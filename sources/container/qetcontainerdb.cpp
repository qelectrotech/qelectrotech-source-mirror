/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "qetcontainerdb.h"

#include "qetcontainer.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTextStream>
#include <QUuid>
#include <QVariant>

#include <sqlite3.h>

namespace {
const QString KeyAttribute = QStringLiteral("qetz-key");
const QString FolioAttribute = QStringLiteral("qetz-folio");
	//What a moved block leaves where it was. Removing it instead would leave
	//the whitespace before and after it side by side, read back as one text
	//node: the parent would come back a child short.
const QString MarkerTag = QStringLiteral("qetz-db");
const QString MarkerBlock = QStringLiteral("block");

void leaveMarker(QDomElement block)
{
	QDomElement marker = block.ownerDocument().createElement(MarkerTag);
	marker.setAttribute(MarkerBlock, block.tagName());
	block.parentNode().replaceChild(marker, block);
}

	//Put @p block where its marker is in @p parent
bool replaceMarker(QDomElement parent, const QDomElement &block)
{
	for (QDomElement m = parent.firstChildElement(MarkerTag) ; !m.isNull() ; m = m.nextSiblingElement(MarkerTag)) {
		if (m.attribute(MarkerBlock) == block.tagName()) {
			parent.replaceChild(block, m);
			return true;
		}
	}
	return false;
}

	//true if @p parent holds the marker of a block @p tag moved out: where
	//the block goes back, and that it was there at all
bool hasMarker(const QDomElement &parent, const QString &tag)
{
	for (QDomElement m = parent.firstChildElement(MarkerTag) ; !m.isNull() ; m = m.nextSiblingElement(MarkerTag))
		if (m.attribute(MarkerBlock) == tag) return true;
	return false;
}

const QStringList FolioData = {
	QStringLiteral("title"), QStringLiteral("author"), QStringLiteral("date"),
	QStringLiteral("folio"), QStringLiteral("indexrev"), QStringLiteral("filename"),
	QStringLiteral("locmach"), QStringLiteral("plant"), QStringLiteral("auto_page_num")};
	//From format 3 on an item's uuid is its row's key, not a column of its
	//own: it stays in the folio file, where it says which row is the item's.
	//So does a symbol's type, the symbol it is: a folio file read alone
	//still says what is drawn where
const QStringList ElementData = {
	QStringLiteral("prefix"), QStringLiteral("freezeLabel")};
	//Format 2 also moved the type
const QStringList ElementDataV2 = {
	QStringLiteral("uuid"), QStringLiteral("type"), QStringLiteral("prefix"), QStringLiteral("freezeLabel")};
const QStringList ConductorData = {
	QStringLiteral("element1"), QStringLiteral("terminal1"),
	QStringLiteral("element2"), QStringLiteral("terminal2"), QStringLiteral("num"),
	QStringLiteral("formula"), QStringLiteral("function"), QStringLiteral("tension_protocol"),
	QStringLiteral("conductor_color"), QStringLiteral("conductor_section"),
	QStringLiteral("cable"), QStringLiteral("bus"), QStringLiteral("freezeLabel")};

	//Attributes held as 0 or 1 (format 3 on). A value QElectroTech does not
	//write, "yes" say, stays in the folio file as it was.
const QStringList BooleanData = {QStringLiteral("freezeLabel")};

QString columns(const QStringList &names)
{
	QStringList quoted;
	for (const QString &n : names)
		quoted << QStringLiteral("\"%1\" %2").arg(n, BooleanData.contains(n) ? QStringLiteral("INTEGER")
																				 : QStringLiteral("TEXT"));
	return quoted.join(QStringLiteral(", "));
}

	//@p text as attribute @p name is stored, or invalid if it cannot be
QVariant stored(const QString &name, const QString &text)
{
	if (!BooleanData.contains(name)) return text;
	if (text == QLatin1String("true")) return 1;
	if (text == QLatin1String("false")) return 0;
	return {};
}

	//A stored value as attribute @p name writes it
QString attributeText(const QString &name, const QVariant &value)
{
	if (BooleanData.contains(name) && value.typeId() == QMetaType::Int)
		return value.toInt() ? QStringLiteral("true") : QStringLiteral("false");
	return value.toString();
}

QString schema()
{
	return QStringLiteral(
		"CREATE TABLE meta (key TEXT PRIMARY KEY, value TEXT);"
		"CREATE TABLE project_property (ord INTEGER PRIMARY KEY, name TEXT, value TEXT, show INTEGER, attrs TEXT);"
		"CREATE TABLE folio (uuid TEXT PRIMARY KEY, pos INTEGER NOT NULL UNIQUE, %1,"
		" props_attrs TEXT);"
		"CREATE TABLE folio_property (folio_uuid TEXT NOT NULL REFERENCES folio, ord INTEGER NOT NULL,"
		" name TEXT, value TEXT, show INTEGER, attrs TEXT, PRIMARY KEY (folio_uuid, ord));"
		"CREATE TABLE element (uuid TEXT PRIMARY KEY, folio_uuid TEXT NOT NULL REFERENCES folio, %2,"
		" info_attrs TEXT, links_attrs TEXT);"
		"CREATE TABLE element_info (element_uuid TEXT NOT NULL REFERENCES element, ord INTEGER NOT NULL,"
		" name TEXT, value TEXT, show INTEGER, attrs TEXT, PRIMARY KEY (element_uuid, ord));"
		"CREATE TABLE link (element_uuid TEXT NOT NULL REFERENCES element, ord INTEGER NOT NULL,"
		" linked_uuid TEXT, attrs TEXT, PRIMARY KEY (element_uuid, ord));"
		"CREATE TABLE conductor (uuid TEXT PRIMARY KEY, folio_uuid TEXT NOT NULL REFERENCES folio, %3);"
		"CREATE TABLE terminal_strip (uuid TEXT PRIMARY KEY, ord INTEGER NOT NULL UNIQUE,"
		" installation TEXT, location TEXT, name TEXT, comment TEXT, description TEXT);"
		"CREATE TABLE strip_terminal (strip_uuid TEXT NOT NULL REFERENCES terminal_strip,"
		" pos INTEGER NOT NULL, level INTEGER NOT NULL, element_uuid TEXT,"
		" PRIMARY KEY (strip_uuid, pos, level));"
		"CREATE TABLE strip_bridge (uuid TEXT PRIMARY KEY, strip_uuid TEXT NOT NULL REFERENCES terminal_strip,"
		" ord INTEGER NOT NULL, color TEXT);"
		"CREATE TABLE strip_bridge_terminal (bridge_uuid TEXT NOT NULL REFERENCES strip_bridge,"
		" ord INTEGER NOT NULL, element_uuid TEXT, PRIMARY KEY (bridge_uuid, ord));")
			.arg(columns(FolioData), columns(ElementData), columns(ConductorData));
}

	//Owns an sqlite3 handle
class Database
{
	public:
		~Database() { if (m_db) sqlite3_close(m_db); }
		sqlite3 *m_db = nullptr;
		bool exec(const QString &sql) {
			return sqlite3_exec(m_db, sql.toUtf8().constData(), nullptr, nullptr, nullptr) == SQLITE_OK;
		}
		QString error() const { return QString::fromUtf8(sqlite3_errmsg(m_db)); }
};

	//Owns a prepared statement; binds and reads QVariants (null = SQL NULL)
class Statement
{
	public:
		Statement(sqlite3 *db, const QString &sql) {
			sqlite3_prepare_v2(db, sql.toUtf8().constData(), -1, &m_stmt, nullptr);
		}
		~Statement() { sqlite3_finalize(m_stmt); }
		bool ok() const { return m_stmt; }
		void bind(const QVariantList &values) {
			sqlite3_reset(m_stmt);
			sqlite3_clear_bindings(m_stmt);
			for (int i = 0 ; i < values.size() ; ++i) {
				const QVariant &v = values.at(i);
				if (v.isNull()) {
					sqlite3_bind_null(m_stmt, i + 1);
				} else if (v.typeId() == QMetaType::Int) {
					sqlite3_bind_int(m_stmt, i + 1, v.toInt());
				} else {
					const QByteArray utf8 = v.toString().toUtf8();
					sqlite3_bind_text(m_stmt, i + 1, utf8.constData(), utf8.size(), SQLITE_TRANSIENT);
				}
			}
		}
		bool run(const QVariantList &values) {
			bind(values);
			return sqlite3_step(m_stmt) == SQLITE_DONE;
		}
		bool next() { return sqlite3_step(m_stmt) == SQLITE_ROW; }
		QVariant value(int column) const {
			switch (sqlite3_column_type(m_stmt, column)) {
				case SQLITE_NULL: return QVariant();
				case SQLITE_INTEGER: return QVariant(sqlite3_column_int(m_stmt, column));
				default: return QVariant(QString::fromUtf8(
							reinterpret_cast<const char *>(sqlite3_column_text(m_stmt, column)),
							sqlite3_column_bytes(m_stmt, column)));
			}
		}
		sqlite3_stmt *m_stmt = nullptr;
};

void setError(QString *error, const QString &text) { if (error) *error = text; }

QString placeholders(int n)
{
	QStringList q;
	for (int i = 0 ; i < n ; ++i) q << QStringLiteral("?");
	return q.join(QLatin1Char(','));
}

	//The attributes of @p e but @p skip as JSON, and its tag if it is not
	//@p usual_tag
QVariant attributesJson(const QDomElement &e, const QStringList &skip, const QString &usual_tag)
{
	QJsonObject o;
	if (e.tagName() != usual_tag) o.insert(QStringLiteral("tag"), e.tagName());
	const QDomNamedNodeMap map = e.attributes();
	for (int i = 0 ; i < map.count() ; ++i) {
		const QDomAttr a = map.item(i).toAttr();
		if (!skip.contains(a.name())) o.insert(a.name(), a.value());
	}
	if (o.isEmpty()) return QVariant();
	return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

void setAttributesJson(QDomElement e, const QVariant &json)
{
	if (json.isNull()) return;
	const QJsonObject o = QJsonDocument::fromJson(json.toString().toUtf8()).object();
	for (auto it = o.begin() ; it != o.end() ; ++it)
		if (it.key() != QLatin1String("tag") && it.key() != QLatin1String("qetz-ws"))
			e.setAttribute(it.key(), it.value().toString());
}

	//A list block's attributes, and the whitespace between its entries if
	//it has any (a document read from a file does; a save has none):
	//"qetz-ws" holds, for each entry and after the last, the text before it.
QVariant blockJson(const QDomElement &block)
{
	QJsonObject o;
	const QDomNamedNodeMap map = block.attributes();
	for (int i = 0 ; i < map.count() ; ++i) {
		const QDomAttr a = map.item(i).toAttr();
		o.insert(a.name(), a.value());
	}
	QJsonArray ws;
	QString pending;
	bool any = false;
	for (QDomNode n = block.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
		if (n.isText()) { pending += n.nodeValue(); any = true; }
		else { ws.append(pending); pending.clear(); }
	}
	ws.append(pending);
	if (any) o.insert(QStringLiteral("qetz-ws"), ws);
	if (o.isEmpty()) return QVariant();
	return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

	//The whitespace blockJson() kept, back between the entries of @p block
void restoreWhitespace(QDomElement block, const QVariant &json)
{
	if (json.isNull()) return;
	const QJsonArray ws = QJsonDocument::fromJson(json.toString().toUtf8()).object()
			.value(QStringLiteral("qetz-ws")).toArray();
	if (ws.isEmpty()) return;
	QDomDocument document = block.ownerDocument();
	QList<QDomNode> entries;
	for (QDomNode n = block.firstChild() ; !n.isNull() ; n = n.nextSibling()) entries << n;
	for (int i = 0 ; i < entries.size() && i < ws.size() ; ++i)
		if (!ws.at(i).toString().isEmpty())
			block.insertBefore(document.createTextNode(ws.at(i).toString()), entries.at(i));
	if (ws.size() > entries.size() && !ws.last().toString().isEmpty())
		block.appendChild(document.createTextNode(ws.last().toString()));
}

	//The value of a list entry: its only child, a text node, or none
bool simpleValue(const QDomElement &e, QVariant *value)
{
	const QDomNodeList kids = e.childNodes();
	if (kids.size() == 0) { *value = QVariant(); return true; }
	if (kids.size() == 1 && kids.at(0).isText() && !kids.at(0).isCDATASection()) {
		*value = kids.at(0).nodeValue();
		return true;
	}
	return false;
}

	//A list block -- <properties>, <elementInformations>, <links_uuids> --
	//this can move: entries named @p entry with simple values, and
	//whitespace between them that a save would write anyway.
bool listBlock(const QDomElement &block, const QString &entry)
{
	for (QDomNode n = block.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
		if (n.isElement()) {
			QVariant v;
			if (n.toElement().tagName() != entry || !simpleValue(n.toElement(), &v)) return false;
		} else if (!(n.isText() && !n.isCDATASection() && n.nodeValue().trimmed().isEmpty())) {
			return false;
		}
	}
	return true;
}

	//The attributes @p names of @p e, taken out of it; one that cannot be
	//stored stays, its value null
QVariantList take(QDomElement e, const QStringList &names)
{
	QVariantList values;
	for (const QString &n : names) {
		const QVariant v = e.hasAttribute(n) ? stored(n, e.attribute(n)) : QVariant();
		values << (v.isValid() ? v : QVariant());
		if (v.isValid()) e.removeAttribute(n);
	}
	return values;
}

void put(QDomElement e, const QStringList &names, const QVariantList &values, int from)
{
	for (int i = 0 ; i < names.size() ; ++i)
		if (!values.at(from + i).isNull())
			e.setAttribute(names.at(i), attributeText(names.at(i), values.at(from + i)));
}

	//Same tags, attributes, text and child order
bool sameTree(const QDomNode &a, const QDomNode &b)
{
	if (a.nodeType() != b.nodeType() || a.nodeName() != b.nodeName()) return false;
	if (!a.isElement() && a.nodeValue() != b.nodeValue()) return false;
	if (a.isElement()) {
		const QDomNamedNodeMap am = a.attributes(), bm = b.attributes();
		if (am.count() != bm.count()) return false;
		for (int i = 0 ; i < am.count() ; ++i) {
			const QDomAttr x = am.item(i).toAttr();
			if (!bm.contains(x.name()) || bm.namedItem(x.name()).nodeValue() != x.value()) return false;
		}
	}
	QDomNode ca = a.firstChild(), cb = b.firstChild();
	for ( ; !ca.isNull() && !cb.isNull() ; ca = ca.nextSibling(), cb = cb.nextSibling())
		if (!sameTree(ca, cb)) return false;
	return ca.isNull() && cb.isNull();
}

bool hasWhitespaceText(const QDomNode &node)
{
	for (QDomNode n = node.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
		if (n.isText() && n.nodeValue().trimmed().isEmpty()) return true;
		if (hasWhitespaceText(n)) return true;
	}
	return false;
}

const QStringList StripData = {
	QStringLiteral("installation"), QStringLiteral("location"), QStringLiteral("name"),
	QStringLiteral("comment"), QStringLiteral("description")};

struct StripRows
{
	QList<QVariantList> strips, terminals, bridges, bridge_terminals;
};

	//<terminal_strips> as TerminalStrip::toXml() writes it, from @p rows
QDomElement stripsXml(QDomDocument &document, const StripRows &rows)
{
	QDomElement root = document.createElement(QStringLiteral("terminal_strips"));
	for (const QVariantList &st : rows.strips) {
		const QString uuid = st.at(0).toString();
		QDomElement strip = root.appendChild(document.createElement(QStringLiteral("terminal_strip"))).toElement();
		QDomElement data = strip.appendChild(document.createElement(QStringLiteral("terminal_strip_data"))).toElement();
		data.setAttribute(QStringLiteral("uuid"), uuid);
		QDomElement infos = data.appendChild(document.createElement(QStringLiteral("informations"))).toElement();
		for (int i = 0 ; i < StripData.size() ; ++i) {
			const QVariant v = st.at(2 + i);
			if (v.isNull()) continue;
			QDomElement info = infos.appendChild(document.createElement(QStringLiteral("information"))).toElement();
			info.setAttribute(QStringLiteral("name"), StripData.at(i));
			info.appendChild(document.createTextNode(v.toString()));
		}
		QDomElement layout = strip.appendChild(document.createElement(QStringLiteral("layout"))).toElement();
		QDomElement physical;
		int last_pos = -1;
		for (const QVariantList &t : rows.terminals) {
			if (t.at(0).toString() != uuid) continue;
			if (t.at(1).toInt() != last_pos) {
				physical = layout.appendChild(document.createElement(QStringLiteral("physical_terminal"))).toElement();
				last_pos = t.at(1).toInt();
			}
			QDomElement real = physical.appendChild(document.createElement(QStringLiteral("real_terminal"))).toElement();
			if (!t.at(3).isNull()) real.setAttribute(QStringLiteral("element_uuid"), t.at(3).toString());
		}
		for (const QVariantList &b : rows.bridges) {
			if (b.at(1).toString() != uuid) continue;
			QDomElement bridge = strip.appendChild(document.createElement(QStringLiteral("terminal_strip_bridge"))).toElement();
			bridge.setAttribute(QStringLiteral("uuid"), b.at(0).toString());
			if (!b.at(3).isNull()) bridge.setAttribute(QStringLiteral("color"), b.at(3).toString());
			QDomElement reals = bridge.appendChild(document.createElement(QStringLiteral("real_terminals"))).toElement();
			for (const QVariantList &bt : rows.bridge_terminals) {
				if (bt.at(0).toString() != b.at(0).toString()) continue;
				QDomElement real = reals.appendChild(document.createElement(QStringLiteral("real_terminal"))).toElement();
				if (!bt.at(2).isNull()) real.setAttribute(QStringLiteral("uuid"), bt.at(2).toString());
			}
		}
	}
	return root;
}

	//The rows of @p block, if rebuilding them gives @p block back exactly
bool stripRows(const QDomElement &block, StripRows *rows)
{
	if (block.attributes().count() || hasWhitespaceText(block)) return false;
	QSet<QString> strip_uuids, bridge_uuids;
	int ord = 0;
	for (QDomElement strip = block.firstChildElement() ; !strip.isNull() ; strip = strip.nextSiblingElement()) {
		const QDomElement data = strip.firstChildElement(QStringLiteral("terminal_strip_data"));
		const QString uuid = data.attribute(QStringLiteral("uuid"));
		if (uuid.isEmpty() || strip_uuids.contains(uuid)) return false;
		strip_uuids.insert(uuid);
		QHash<QString, QString> info;
		for (QDomElement i = data.firstChildElement(QStringLiteral("informations")).firstChildElement() ;
			 !i.isNull() ; i = i.nextSiblingElement())
			info.insert(i.attribute(QStringLiteral("name")), i.text());
		QVariantList row{uuid, ord++};
		for (const QString &n : StripData) row << (info.contains(n) ? QVariant(info.value(n)) : QVariant());
		rows->strips << row;
		int pos = 0;
		for (QDomElement phy = strip.firstChildElement(QStringLiteral("layout")).firstChildElement() ;
			 !phy.isNull() ; phy = phy.nextSiblingElement(), ++pos) {
			int level = 0;
			for (QDomElement real = phy.firstChildElement() ; !real.isNull() ; real = real.nextSiblingElement())
				rows->terminals << QVariantList{uuid, pos, level++, real.hasAttribute(QStringLiteral("element_uuid"))
												? QVariant(real.attribute(QStringLiteral("element_uuid"))) : QVariant()};
		}
		int b_ord = 0;
		for (QDomElement br = strip.firstChildElement(QStringLiteral("terminal_strip_bridge")) ;
			 !br.isNull() ; br = br.nextSiblingElement(QStringLiteral("terminal_strip_bridge"))) {
			const QString buuid = br.attribute(QStringLiteral("uuid"));
			if (buuid.isEmpty() || bridge_uuids.contains(buuid)) return false;
			bridge_uuids.insert(buuid);
			rows->bridges << QVariantList{buuid, uuid, b_ord++, br.hasAttribute(QStringLiteral("color"))
											? QVariant(br.attribute(QStringLiteral("color"))) : QVariant()};
			int k = 0;
			for (QDomElement real = br.firstChildElement(QStringLiteral("real_terminals")).firstChildElement() ;
				 !real.isNull() ; real = real.nextSiblingElement())
				rows->bridge_terminals << QVariantList{buuid, k++, real.hasAttribute(QStringLiteral("uuid"))
														? QVariant(real.attribute(QStringLiteral("uuid"))) : QVariant()};
		}
	}
	QDomDocument scratch;
	return sameTree(block, stripsXml(scratch, *rows));
}

QList<QDomElement> children(const QDomElement &parent, const QString &tag)
{
	QList<QDomElement> list;
	for (QDomElement e = parent.firstChildElement(tag) ; !e.isNull() ; e = e.nextSiblingElement(tag))
		list << e;
	return list;
}

	//A list entry -- a <property>, an <elementInformation> -- as its row:
	//its name, value, show flag (0 or 1, else left in the JSON) and any
	//other attribute as JSON, its tag too if it is not @p usual_tag
QVariantList entryRow(const QDomElement &e, const QString &usual_tag)
{
	QVariant value;
	simpleValue(e, &value);
	const QString show = e.attribute(QStringLiteral("show"));
	const bool show_stored = show == QLatin1String("0") || show == QLatin1String("1");
	QStringList kept{QStringLiteral("name")};
	if (show_stored) kept << QStringLiteral("show");
	return {e.hasAttribute(QStringLiteral("name")) ? QVariant(e.attribute(QStringLiteral("name"))) : QVariant(),
			value, show_stored ? QVariant(show.toInt()) : QVariant(), attributesJson(e, kept, usual_tag)};
}

	//The key of each of @p items: its uuid, when it has one no other of
	//@p items has, so the uuid the folio file already holds says which row
	//is the item's (@p own true). Else (F100: symbols copied with their
	//uuid) a uuid made from the item itself -- the same on every save,
	//never from where the item is -- kept in the folio file as qetz-key.
QStringList keys(const QList<QDomElement> &items, QList<bool> *own)
{
	static const QUuid key_namespace(QStringLiteral("{3f0c6c1e-5b8e-4b8a-9d55-6b2f5f0f9a41}"));
	QHash<QString, int> seen;
	for (const QDomElement &e : items)
		if (!QUuid(e.attribute(QStringLiteral("uuid"))).isNull()) ++seen[e.attribute(QStringLiteral("uuid"))];
	QSet<QString> taken(seen.keyBegin(), seen.keyEnd());
	QHash<QString, int> twins;
	QStringList out;
	for (const QDomElement &e : items) {
		const QString uuid = e.attribute(QStringLiteral("uuid"));
		if (seen.value(uuid) == 1) {
			out << uuid;
			*own << true;
			continue;
		}
		QString text;
		QTextStream stream(&text);
		e.save(stream, -1);
		stream.flush();
			//Items alike to the byte: how many came before tells them apart
		const int twin = twins[text]++;
		QString key;
		for (int salt = 0 ; key.isEmpty() || taken.contains(key) ; ++salt)
			key = QUuid::createUuidV5(key_namespace, QStringLiteral("%1\n%2\n%3").arg(text).arg(twin).arg(salt))
					.toString();
		taken.insert(key);
		out << key;
		*own << false;
	}
	return out;
}
}

/**
	@brief QetContainerDb::extract
	Move the engineering data of @p project (the <project> element, its
	folios still in it) into a new database.
	@return the database file, or an empty array with @p error set
*/
QByteArray QetContainerDb::extract(QDomElement project, QString *error)
{
	Database db;
	if (sqlite3_open(":memory:", &db.m_db) != SQLITE_OK || !db.exec(schema())) {
		setError(error, QStringLiteral("cannot create the project database"));
		return {};
	}
	db.exec(QStringLiteral("BEGIN"));

	Statement meta(db.m_db, QStringLiteral("INSERT INTO meta VALUES (?,?)"));
	Statement project_property(db.m_db, QStringLiteral("INSERT INTO project_property VALUES (?,?,?,?,?)"));
	Statement folio(db.m_db, QStringLiteral("INSERT INTO folio VALUES (?,?,%1,?)").arg(placeholders(FolioData.size())));
	Statement folio_property(db.m_db, QStringLiteral("INSERT INTO folio_property VALUES (?,?,?,?,?,?)"));
	Statement element(db.m_db, QStringLiteral("INSERT INTO element VALUES (?,?,%1,?,?)").arg(placeholders(ElementData.size())));
	Statement element_info(db.m_db, QStringLiteral("INSERT INTO element_info VALUES (?,?,?,?,?,?)"));
	Statement link(db.m_db, QStringLiteral("INSERT INTO link VALUES (?,?,?,?)"));
	Statement conductor(db.m_db, QStringLiteral("INSERT INTO conductor VALUES (?,?,%1)").arg(placeholders(ConductorData.size())));
	bool ok = meta.ok() && project_property.ok() && folio.ok() && folio_property.ok()
			&& element.ok() && element_info.ok() && link.ok() && conductor.ok();

	ok = ok && meta.run({QStringLiteral("format"), QString::number(QetContainer::Format)})
			&& db.exec(QStringLiteral("PRAGMA user_version = %1").arg(QetContainer::Format));

		//<properties> of a block, as rows of @p rows (with @p prefix values)
	auto moveProperties = [&](QDomElement block, Statement &rows, const QVariantList &prefix) {
		int ord = 0;
		for (QDomElement p : children(block, QStringLiteral("property")))
			ok = ok && rows.run(QVariantList(prefix) << ord++ << entryRow(p, QStringLiteral("property")));
	};

		//Project properties
	const QDomElement project_props = project.firstChildElement(QStringLiteral("properties"));
	if (!project_props.isNull() && listBlock(project_props, QStringLiteral("property"))) {
		ok = ok && meta.run({QStringLiteral("props_attrs"), blockJson(project_props)});
		moveProperties(project_props, project_property, {});
		leaveMarker(project_props);
	}

	const QList<QDomElement> diagrams = children(project, QStringLiteral("diagram"));
	QList<QDomElement> all_elements, all_conductors;
	for (const QDomElement &d : diagrams) {
		for (const QDomElement &list : children(d, QStringLiteral("elements")))
			all_elements += children(list, QStringLiteral("element"));
		for (const QDomElement &list : children(d, QStringLiteral("conductors")))
			all_conductors += children(list, QStringLiteral("conductor"));
	}
	QList<bool> folio_own, element_own, conductor_own;
	const QStringList folio_keys = keys(diagrams, &folio_own);
	const QStringList element_keys = keys(all_elements, &element_own);
	const QStringList conductor_keys = keys(all_conductors, &conductor_own);
	int e_index = 0, c_index = 0;

	int pos = 0;
	for (QDomElement d : diagrams)
	{
		const QString id = folio_keys.at(pos);
		const bool own_id = folio_own.at(pos);
		++pos;
		QVariantList row{id, pos};
		row += take(d, FolioData);
		QDomElement props = d.firstChildElement(QStringLiteral("properties"));
		const bool move_props = !props.isNull() && listBlock(props, QStringLiteral("property"));
		row << (move_props ? blockJson(props) : QVariant());
		ok = ok && folio.run(row);
		if (move_props) {
			moveProperties(props, folio_property, {id});
			leaveMarker(props);
		}
		if (!own_id) d.setAttribute(FolioAttribute, id);

		for (const QDomElement &list : children(d, QStringLiteral("elements"))) {
			for (QDomElement e : children(list, QStringLiteral("element"))) {
				const QString key = element_keys.at(e_index);
				const bool own_key = element_own.at(e_index++);
				QVariantList erow{key, id};
				erow += take(e, ElementData);
				QDomElement info = e.firstChildElement(QStringLiteral("elementInformations"));
				QDomElement links = e.firstChildElement(QStringLiteral("links_uuids"));
				if (!info.isNull() && !listBlock(info, QStringLiteral("elementInformation"))) info = QDomElement();
				if (!links.isNull() && !listBlock(links, QStringLiteral("link_uuid"))) links = QDomElement();
				erow << (info.isNull() ? QVariant() : blockJson(info))
					 << (links.isNull() ? QVariant() : blockJson(links));
				ok = ok && element.run(erow);
				if (!info.isNull()) {
					int ord = 0;
					for (QDomElement i : children(info, QStringLiteral("elementInformation")))
						ok = ok && element_info.run(QVariantList{key, ord++}
													<< entryRow(i, QStringLiteral("elementInformation")));
				}
				if (!links.isNull()) {
					int ord = 0;
					for (QDomElement l : children(links, QStringLiteral("link_uuid")))
						ok = ok && link.run({key, ord++, l.hasAttribute(QStringLiteral("uuid"))
												? QVariant(l.attribute(QStringLiteral("uuid"))) : QVariant(),
											 attributesJson(l, {QStringLiteral("uuid")}, QStringLiteral("link_uuid"))});
				}
				if (!info.isNull()) leaveMarker(info);
				if (!links.isNull()) leaveMarker(links);
				if (!own_key) e.setAttribute(KeyAttribute, key);
			}
		}
		for (const QDomElement &list : children(d, QStringLiteral("conductors"))) {
			for (QDomElement c : children(list, QStringLiteral("conductor"))) {
				const QString key = conductor_keys.at(c_index);
				const bool own_key = conductor_own.at(c_index++);
				QVariantList crow{key, id};
				crow += take(c, ConductorData);
				ok = ok && conductor.run(crow);
				if (!own_key) c.setAttribute(KeyAttribute, key);
			}
		}
	}

		//Terminal strips, when their rows rebuild them exactly
	const QDomElement strips = project.firstChildElement(QStringLiteral("terminal_strips"));
	StripRows strip_rows;
	if (!strips.isNull() && stripRows(strips, &strip_rows)) {
		Statement st(db.m_db, QStringLiteral("INSERT INTO terminal_strip VALUES (?,?,?,?,?,?,?)"));
		Statement tt(db.m_db, QStringLiteral("INSERT INTO strip_terminal VALUES (?,?,?,?)"));
		Statement bt(db.m_db, QStringLiteral("INSERT INTO strip_bridge VALUES (?,?,?,?)"));
		Statement btt(db.m_db, QStringLiteral("INSERT INTO strip_bridge_terminal VALUES (?,?,?)"));
		for (const QVariantList &r : strip_rows.strips) ok = ok && st.run(r);
		for (const QVariantList &r : strip_rows.terminals) ok = ok && tt.run(r);
		for (const QVariantList &r : strip_rows.bridges) ok = ok && bt.run(r);
		for (const QVariantList &r : strip_rows.bridge_terminals) ok = ok && btt.run(r);
		ok = ok && meta.run({QStringLiteral("terminal_strips"), QStringLiteral("1")});
		leaveMarker(strips);
	}

	if (!ok || !db.exec(QStringLiteral("COMMIT"))) {
		setError(error, QStringLiteral("cannot fill the project database: %1").arg(db.error()));
		return {};
	}

	sqlite3_int64 size = 0;
	unsigned char *bytes = sqlite3_serialize(db.m_db, "main", &size, 0);
	if (!bytes) {
		setError(error, QStringLiteral("cannot write the project database"));
		return {};
	}
	const QByteArray file(reinterpret_cast<const char *>(bytes), qsizetype(size));
	sqlite3_free(bytes);
	return file;
}

/**
	@brief QetContainerDb::restore
	Put the engineering data in @p database back into @p project (the
	<project> element, its folios joined back already).
*/
bool QetContainerDb::restore(QDomElement project, const QByteArray &database, QString *error)
{
	QDomDocument document = project.ownerDocument();
	Database db;
	if (sqlite3_open(":memory:", &db.m_db) != SQLITE_OK) {
		setError(error, QStringLiteral("cannot open the project database"));
		return false;
	}
	unsigned char *copy = static_cast<unsigned char *>(sqlite3_malloc64(sqlite3_uint64(database.size())));
	if (!copy) { setError(error, QStringLiteral("out of memory")); return false; }
	memcpy(copy, database.constData(), size_t(database.size()));
	if (sqlite3_deserialize(db.m_db, "main", copy, database.size(), database.size(),
							SQLITE_DESERIALIZE_FREEONCLOSE | SQLITE_DESERIALIZE_READONLY) != SQLITE_OK) {
		setError(error, QStringLiteral("the project database cannot be read"));
		return false;
	}
		//A file is untrusted: nothing in its schema may run, and every name
		//read must be a plain table.
	db.exec(QStringLiteral("PRAGMA trusted_schema = OFF"));
	db.exec(QStringLiteral("PRAGMA query_only = ON"));
	for (const char *table : {"meta", "project_property", "folio", "folio_property",
							  "element", "element_info", "link", "conductor", "terminal_strip",
							  "strip_terminal", "strip_bridge", "strip_bridge_terminal"}) {
		Statement kind(db.m_db, QStringLiteral("SELECT type FROM sqlite_master WHERE name = ?"));
		kind.bind({QString::fromLatin1(table)});
		if (!kind.next() || kind.value(0).toString() != QLatin1String("table")) {
			setError(error, QStringLiteral("the project database has no table %1").arg(QLatin1String(table)));
			return false;
		}
	}

	auto entry = [&document](const QVariant &name_attr, const QString &name, const QVariant &value,
							 const QVariant &attrs, const QString &default_tag, const QVariant &show = QVariant()) {
		const QJsonObject o = QJsonDocument::fromJson(attrs.toString().toUtf8()).object();
		QDomElement e = document.createElement(o.value(QStringLiteral("tag")).toString(default_tag));
		if (!name_attr.isNull()) e.setAttribute(name, name_attr.toString());
		if (!show.isNull()) e.setAttribute(QStringLiteral("show"), show.toString());
		setAttributesJson(e, attrs);
		if (!value.isNull()) e.appendChild(document.createTextNode(value.toString()));
		return e;
	};

		//Project properties
	QHash<QString, QVariant> meta;
	{
		Statement s(db.m_db, QStringLiteral("SELECT key, value FROM meta"));
		while (s.next()) meta.insert(s.value(0).toString(), s.value(1));
	}
		//Format 2 (QElectroTech 0.200.1 development builds) keyed its rows
		//apart from the items' uuids: "f1", "e1", "c1" where an item had
		//none of its own, and every item's key in the folio file
	const int format = meta.value(QStringLiteral("format")).toString().toInt();
		//A later format is read when the manifest's min-reader allows it
		//(QetContainer::join()): its writer kept what this reads
	if (format < 2) {
		setError(error, QStringLiteral("the project database is of format %1, this reads format 2 on")
				 .arg(meta.value(QStringLiteral("format")).toString()));
		return false;
	}
	const bool v2 = format == 2;
	const QString folio_key = v2 ? QStringLiteral("id") : QStringLiteral("uuid");
	const QString folio_ref = v2 ? QStringLiteral("folio_id") : QStringLiteral("folio_uuid");
	const QString item_key = v2 ? QStringLiteral("key") : QStringLiteral("uuid");
	const QString element_ref = v2 ? QStringLiteral("element_key") : QStringLiteral("element_uuid");
	const QString show_column = v2 ? QStringLiteral("NULL") : QStringLiteral("show");
	const QStringList element_data = v2 ? ElementDataV2 : ElementData;
	const QStringList conductor_data = v2 ? QStringList{QStringLiteral("uuid")} + ConductorData : ConductorData;
	auto quoted = [](const QStringList &names) {
		return QStringLiteral("\"") + names.join(QStringLiteral("\", \"")) + QStringLiteral("\"");
	};
		//An item's key: the one it was given, else (format 3) its uuid
	auto keyOf = [v2](QDomElement e, const QString &attribute) {
		if (e.hasAttribute(attribute)) {
			const QString key = e.attribute(attribute);
			e.removeAttribute(attribute);
			return key;
		}
		return v2 ? QString() : e.attribute(QStringLiteral("uuid"));
	};
		//A moved block goes back where its marker is: the marker says it
		//was there
	if (hasMarker(project, QStringLiteral("properties"))) {
		QDomElement props = document.createElement(QStringLiteral("properties"));
		setAttributesJson(props, meta.value(QStringLiteral("props_attrs")));
		Statement s(db.m_db, QStringLiteral("SELECT name, value, attrs, %1 FROM project_property ORDER BY ord")
					.arg(show_column));
		while (s.next())
			props.appendChild(entry(s.value(0), QStringLiteral("name"), s.value(1), s.value(2),
									QStringLiteral("property"), s.value(3)));
		restoreWhitespace(props, meta.value(QStringLiteral("props_attrs")));
		if (!replaceMarker(project, props)) {
			setError(error, QStringLiteral("the project's properties have no place"));
			return false;
		}
	}

	if (meta.contains(QStringLiteral("terminal_strips"))) {
		StripRows rows;
		auto read = [&db](const QString &sql, int columns, QList<QVariantList> *out) {
			Statement s(db.m_db, sql);
			while (s.next()) {
				QVariantList r;
				for (int i = 0 ; i < columns ; ++i) r << s.value(i);
				*out << r;
			}
		};
		read(QStringLiteral("SELECT * FROM terminal_strip ORDER BY ord"), 7, &rows.strips);
		read(QStringLiteral("SELECT * FROM strip_terminal ORDER BY strip_uuid, pos, level"), 4, &rows.terminals);
		read(QStringLiteral("SELECT * FROM strip_bridge ORDER BY ord"), 4, &rows.bridges);
		read(QStringLiteral("SELECT * FROM strip_bridge_terminal ORDER BY bridge_uuid, ord"), 3, &rows.bridge_terminals);
		if (!replaceMarker(project, stripsXml(document, rows))) {
			setError(error, QStringLiteral("the terminal strips have no place"));
			return false;
		}
	}

		//Every row must be an item's, once, on the folio the item is on:
		//nothing enforces it in the file, which is untrusted
	QSet<QString> folios_used, elements_used, conductors_used;
	auto use = [error](QSet<QString> &used, const QString &key, const QString &what) {
		if (used.contains(key)) {
			setError(error, QStringLiteral("%1 %2 is in the folios twice").arg(what, key));
			return false;
		}
		used.insert(key);
		return true;
	};
	for (QDomElement d : children(project, QStringLiteral("diagram")))
	{
		const QString id = keyOf(d, FolioAttribute);
		if (id.isEmpty()) continue;
		if (!use(folios_used, id, QStringLiteral("folio"))) return false;
		Statement f(db.m_db, QStringLiteral("SELECT %1, props_attrs FROM folio WHERE %2 = ?")
					.arg(quoted(FolioData), folio_key));
		f.bind({id});
		if (!f.next()) { setError(error, QStringLiteral("folio %1 is not in the database").arg(id)); return false; }
		QVariantList values;
		for (int i = 0 ; i < FolioData.size() + 1 ; ++i) values << f.value(i);
		put(d, FolioData, values, 0);
		if (hasMarker(d, QStringLiteral("properties"))) {
			QDomElement props = document.createElement(QStringLiteral("properties"));
			setAttributesJson(props, values.at(FolioData.size()));
			Statement p(db.m_db, QStringLiteral("SELECT name, value, attrs, %1 FROM folio_property WHERE %2 = ? ORDER BY ord")
						.arg(show_column, folio_ref));
			p.bind({id});
			while (p.next())
				props.appendChild(entry(p.value(0), QStringLiteral("name"), p.value(1), p.value(2),
										QStringLiteral("property"), p.value(3)));
			restoreWhitespace(props, values.at(FolioData.size()));
			if (!replaceMarker(d, props)) {
				setError(error, QStringLiteral("folio %1's properties have no place").arg(id));
				return false;
			}
		}

		for (const QDomElement &list : children(d, QStringLiteral("elements"))) {
			for (QDomElement e : children(list, QStringLiteral("element"))) {
				const QString key = keyOf(e, KeyAttribute);
				if (key.isEmpty()) continue;
				if (!use(elements_used, key, QStringLiteral("symbol"))) return false;
				Statement s(db.m_db, QStringLiteral("SELECT %1, info_attrs, links_attrs, %2 FROM element WHERE %3 = ?")
							.arg(quoted(element_data), folio_ref, item_key));
				s.bind({key});
				if (!s.next()) { setError(error, QStringLiteral("symbol %1 is not in the database").arg(key)); return false; }
				if (s.value(element_data.size() + 2).toString() != id) {
					setError(error, QStringLiteral("symbol %1 is on folio %2, the database says another").arg(key, id));
					return false;
				}
				QVariantList v;
				for (int i = 0 ; i < element_data.size() + 2 ; ++i) v << s.value(i);
				put(e, element_data, v, 0);
				const int n = element_data.size();
				QDomElement info, links;
				if (hasMarker(e, QStringLiteral("elementInformations"))) {
					info = document.createElement(QStringLiteral("elementInformations"));
					setAttributesJson(info, v.at(n));
					Statement i(db.m_db, QStringLiteral("SELECT name, value, attrs, %1 FROM element_info WHERE %2 = ? ORDER BY ord")
								.arg(show_column, element_ref));
					i.bind({key});
					while (i.next())
						info.appendChild(entry(i.value(0), QStringLiteral("name"), i.value(1), i.value(2),
											   QStringLiteral("elementInformation"), i.value(3)));
					restoreWhitespace(info, v.at(n));
				}
				if (hasMarker(e, QStringLiteral("links_uuids"))) {
					links = document.createElement(QStringLiteral("links_uuids"));
					setAttributesJson(links, v.at(n + 1));
					Statement l(db.m_db, QStringLiteral("SELECT linked_uuid, attrs FROM link WHERE %1 = ? ORDER BY ord").arg(element_ref));
					l.bind({key});
					while (l.next())
						links.appendChild(entry(l.value(0), QStringLiteral("uuid"), QVariant(), l.value(1),
												QStringLiteral("link_uuid")));
					restoreWhitespace(links, v.at(n + 1));
				}
				if ((!info.isNull() && !replaceMarker(e, info))
					|| (!links.isNull() && !replaceMarker(e, links))) {
					setError(error, QStringLiteral("symbol %1's information has no place").arg(key));
					return false;
				}
			}
		}
		for (const QDomElement &list : children(d, QStringLiteral("conductors"))) {
			for (QDomElement c : children(list, QStringLiteral("conductor"))) {
				const QString key = keyOf(c, KeyAttribute);
				if (key.isEmpty()) continue;
				if (!use(conductors_used, key, QStringLiteral("wire"))) return false;
				Statement s(db.m_db, QStringLiteral("SELECT %1, %2 FROM conductor WHERE %3 = ?")
							.arg(quoted(conductor_data), folio_ref, item_key));
				s.bind({key});
				if (!s.next()) { setError(error, QStringLiteral("wire %1 is not in the database").arg(key)); return false; }
				if (s.value(conductor_data.size()).toString() != id) {
					setError(error, QStringLiteral("wire %1 is on folio %2, the database says another").arg(key, id));
					return false;
				}
				QVariantList v;
				for (int i = 0 ; i < conductor_data.size() ; ++i) v << s.value(i);
				put(c, conductor_data, v, 0);
			}
		}
	}

		//No row left over: each table's rows, counted, are the items read,
		//and every entry belongs to a row
	auto count = [&db](const QString &sql) {
		Statement s(db.m_db, sql);
		return s.next() ? s.value(0).toInt() : -1;
	};
	const struct { QString sql; int expected; QString what; } checks[] = {
		{QStringLiteral("SELECT count(*) FROM folio"), int(folios_used.size()), QStringLiteral("folio rows")},
		{QStringLiteral("SELECT count(*) FROM element"), int(elements_used.size()), QStringLiteral("symbol rows")},
		{QStringLiteral("SELECT count(*) FROM conductor"), int(conductors_used.size()), QStringLiteral("wire rows")},
		{QStringLiteral("SELECT count(*) FROM folio_property WHERE %1 NOT IN (SELECT %2 FROM folio)")
				.arg(folio_ref, folio_key), 0, QStringLiteral("folio property rows of no folio")},
		{QStringLiteral("SELECT count(*) FROM element_info WHERE %1 NOT IN (SELECT %2 FROM element)")
				.arg(element_ref, item_key), 0, QStringLiteral("information rows of no symbol")},
		{QStringLiteral("SELECT count(*) FROM link WHERE %1 NOT IN (SELECT %2 FROM element)")
				.arg(element_ref, item_key), 0, QStringLiteral("link rows of no symbol")}};
	for (const auto &check : checks) {
		const int found = count(check.sql);
		if (found != check.expected) {
			setError(error, QStringLiteral("the project database does not match its folios: %1 %2, %3 expected")
					 .arg(found).arg(check.what).arg(check.expected));
			return false;
		}
	}
	return true;
}
