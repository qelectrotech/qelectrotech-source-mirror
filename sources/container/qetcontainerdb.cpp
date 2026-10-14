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
	//A folio written as it was read (stage D4): its part
const QString KeptPartAttribute = QStringLiteral("qetz-kept-part");
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

	//The tables of a document's engineering data
const QStringList DocTables = {
	QStringLiteral("strip_bridge_terminal"), QStringLiteral("strip_bridge"),
	QStringLiteral("strip_terminal"), QStringLiteral("terminal_strip"),
	QStringLiteral("project_property"), QStringLiteral("folio_property"), QStringLiteral("folio"),
	QStringLiteral("element_info"), QStringLiteral("element"), QStringLiteral("link"),
	QStringLiteral("conductor"), QStringLiteral("meta")};

	//Owns an sqlite3 handle; names its columns as the format of the
	//database read names them
class Database
{
	public:
		~Database() { if (m_db) sqlite3_close(m_db); }
		sqlite3 *m_db = nullptr;
		int m_format = QetContainer::Format;
		bool exec(const QString &sql) {
			return sqlite3_exec(m_db, sql.toUtf8().constData(), nullptr, nullptr, nullptr) == SQLITE_OK;
		}
		QString error() const { return QString::fromUtf8(sqlite3_errmsg(m_db)); }
			//Format 2 (QElectroTech 0.200.1 development builds) keyed its
			//rows apart from the items' uuids: "f1", "e1", "c1" where an item
			//had none of its own, and every item's key in the folio file
		bool v2() const { return m_format == 2; }
		QString folioKey() const { return v2() ? QStringLiteral("id") : QStringLiteral("uuid"); }
		QString folioRef() const { return v2() ? QStringLiteral("folio_id") : QStringLiteral("folio_uuid"); }
		QString itemKey() const { return v2() ? QStringLiteral("key") : QStringLiteral("uuid"); }
		QString elementRef() const { return v2() ? QStringLiteral("element_key") : QStringLiteral("element_uuid"); }
		QString showColumn() const { return v2() ? QStringLiteral("NULL") : QStringLiteral("show"); }
		QStringList elementData() const { return v2() ? ElementDataV2 : ElementData; }
		QStringList conductorData() const { return v2() ? QStringList{QStringLiteral("uuid")} + ConductorData : ConductorData; }
};

	//Owns a prepared statement; binds and reads QVariants (null = SQL NULL)
class Statement
{
	public:
		Statement(const Database &db, const QString &sql) {
			sqlite3_prepare_v2(db.m_db, sql.toUtf8().constData(), -1, &m_stmt, nullptr);
		}
		~Statement() { sqlite3_finalize(m_stmt); }
		bool ok() const { return m_stmt; }
			//A statement that could not be prepared (the database is not
			//one, a table is missing) binds nothing and steps to no row:
			//SQLite's own calls are not all safe on a null statement
		void bind(const QVariantList &values) {
			if (!m_stmt) return;
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
			if (!m_stmt) return false;
			bind(values);
			return sqlite3_step(m_stmt) == SQLITE_DONE;
		}
		bool next() { return m_stmt && sqlite3_step(m_stmt) == SQLITE_ROW; }
		QVariant value(int column) const {
			if (!m_stmt) return QVariant();
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
	//@p items has and @p reserved does not hold, so the uuid the folio file
	//already holds says which row is the item's (@p own true). Else (F100:
	//symbols copied with their uuid) a uuid made from the item itself --
	//the same on every save, never from where the item is -- kept in the
	//folio file as qetz-key. @p reserved: the keys of rows copied from
	//another database (a folio kept as it was)
QStringList keys(const QList<QDomElement> &items, QList<bool> *own,
				 const QSet<QString> &reserved = QSet<QString>())
{
	static const QUuid key_namespace(QStringLiteral("{3f0c6c1e-5b8e-4b8a-9d55-6b2f5f0f9a41}"));
	QHash<QString, int> seen;
	for (const QDomElement &e : items)
		if (!QUuid(e.attribute(QStringLiteral("uuid"))).isNull()) ++seen[e.attribute(QStringLiteral("uuid"))];
	QSet<QString> taken(seen.keyBegin(), seen.keyEnd());
	taken.unite(reserved);
	QHash<QString, int> twins;
	QStringList out;
	for (const QDomElement &e : items) {
		const QString uuid = e.attribute(QStringLiteral("uuid"));
		if (seen.value(uuid) == 1 && !reserved.contains(uuid)) {
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

	//Every table of @p db written again without its keys and indexes, in
	//the order of its rows, and the file compacted: the file a .qetz holds
	//is read whole, its lookups indexed once read (openDatabase()), and keys
	//and indexes of text uuids were half of it (stage D3b)
bool plainTables(sqlite3 *db)
{
	QStringList tables;
	{
		sqlite3_stmt *list = nullptr;
		sqlite3_prepare_v2(db, "SELECT name FROM main.sqlite_master WHERE type = 'table' "
							   "AND name NOT LIKE 'sqlite_%' ORDER BY name", -1, &list, nullptr);
		while (list && sqlite3_step(list) == SQLITE_ROW)
			tables << QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(list, 0)));
		sqlite3_finalize(list);
	}
	auto run = [db](const QString &sql) {
		return sqlite3_exec(db, sql.toUtf8().constData(), nullptr, nullptr, nullptr) == SQLITE_OK;
	};
	bool ok = run(QStringLiteral("BEGIN"));
	for (const QString &table : std::as_const(tables)) {
		const QString plain = table + QStringLiteral("__plain");
			//The same columns and types (QETZ-FORMAT.md), no key or constraint
		QStringList columns;
		{
			sqlite3_stmt *info = nullptr;
			sqlite3_prepare_v2(db, QStringLiteral("SELECT name, type FROM pragma_table_info('%1') ORDER BY cid")
							   .arg(table).toUtf8().constData(), -1, &info, nullptr);
			while (info && sqlite3_step(info) == SQLITE_ROW)
				columns << QStringLiteral("\"%1\" %2").arg(
							   QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(info, 0))),
							   QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(info, 1))));
			sqlite3_finalize(info);
		}
		ok = ok && !columns.isEmpty()
				&& run(QStringLiteral("CREATE TABLE \"%1\" (%2)").arg(plain, columns.join(QStringLiteral(", "))))
				&& run(QStringLiteral("INSERT INTO \"%1\" SELECT * FROM \"%2\" ORDER BY rowid").arg(plain, table))
				&& run(QStringLiteral("DROP TABLE \"%1\"").arg(table))
				&& run(QStringLiteral("ALTER TABLE \"%1\" RENAME TO \"%2\"").arg(plain, table));
	}
	ok = ok && run(QStringLiteral("COMMIT"));
	return ok && run(QStringLiteral("VACUUM"));
}

	//@p database opened in @p db, its tables checked to be plain tables and
	//its format one this reads: a file is untrusted
bool openDatabase(Database &db, const QByteArray &database, QString *error)
{
	if (sqlite3_open(":memory:", &db.m_db) != SQLITE_OK) {
		setError(error, QStringLiteral("cannot open the project database"));
		return false;
	}
	unsigned char *copy = static_cast<unsigned char *>(sqlite3_malloc64(sqlite3_uint64(database.size())));
	if (!copy) { setError(error, QStringLiteral("out of memory")); return false; }
	memcpy(copy, database.constData(), size_t(database.size()));
		//A copy of its own, writable: its lookups are indexed below
	if (sqlite3_deserialize(db.m_db, "main", copy, database.size(), database.size(),
							SQLITE_DESERIALIZE_FREEONCLOSE | SQLITE_DESERIALIZE_RESIZEABLE) != SQLITE_OK) {
		setError(error, QStringLiteral("the project database cannot be read"));
		return false;
	}
		//A file is untrusted: nothing in its schema may run, and every name
		//read must be a plain table.
	db.exec(QStringLiteral("PRAGMA trusted_schema = OFF"));
	for (const QString &table : DocTables) {
		Statement kind(db, QStringLiteral("SELECT type FROM sqlite_master WHERE name = ?"));
		kind.bind({table});
		if (!kind.next() || kind.value(0).toString() != QLatin1String("table")) {
			setError(error, QStringLiteral("the project database has no table %1").arg(table));
			return false;
		}
	}
	{
		Statement format(db, QStringLiteral("SELECT value FROM meta WHERE key = 'format'"));
		const QString text = format.next() ? format.value(0).toString() : QString();
		db.m_format = text.toInt();
			//A later format is read when the manifest's min-reader allows it
			//(QetContainer::join()): its writer kept what this reads
		if (db.m_format < 2) {
			setError(error, QStringLiteral("the project database is of format %1, this reads format 2 on")
					 .arg(text));
			return false;
		}
	}
		//What the folios are read by: a file holds no index (plainTables())
	const QStringList indexes = {
		QStringLiteral("folio(%1)").arg(db.folioKey()), QStringLiteral("folio_property(%1)").arg(db.folioRef()),
		QStringLiteral("element(%1)").arg(db.itemKey()), QStringLiteral("element(%1)").arg(db.folioRef()),
		QStringLiteral("element_info(%1)").arg(db.elementRef()), QStringLiteral("link(%1)").arg(db.elementRef()),
		QStringLiteral("conductor(%1)").arg(db.itemKey()), QStringLiteral("meta(key)")};
	for (const QString &on : indexes) {
		const QString name = QStringLiteral("qetz_read_") + QString(on).replace(QLatin1Char('('), QLatin1Char('_')).remove(QLatin1Char(')'));
		if (!db.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS %1 ON %2").arg(name, on))) {
			setError(error, QStringLiteral("the project database cannot be read: %1").arg(db.error()));
			return false;
		}
	}
	db.exec(QStringLiteral("PRAGMA query_only = ON"));
	return true;
}

QDomElement entryElement(QDomDocument &document, const QVariant &name_attr, const QString &name,
						 const QVariant &value, const QVariant &attrs, const QString &default_tag,
						 const QVariant &show = QVariant())
{
	const QJsonObject o = QJsonDocument::fromJson(attrs.toString().toUtf8()).object();
	QDomElement e = document.createElement(o.value(QStringLiteral("tag")).toString(default_tag));
	if (!name_attr.isNull()) e.setAttribute(name, name_attr.toString());
	if (!show.isNull()) e.setAttribute(QStringLiteral("show"), show.toString());
	setAttributesJson(e, attrs);
	if (!value.isNull()) e.appendChild(document.createTextNode(value.toString()));
	return e;
}

	//An item's key: the one it was given, else (format 3 on) its uuid
QString keyOf(const Database &db, QDomElement e, const QString &attribute)
{
	if (e.hasAttribute(attribute)) {
		const QString key = e.attribute(attribute);
		e.removeAttribute(attribute);
		return key;
	}
	return db.v2() ? QString() : e.attribute(QStringLiteral("uuid"));
}

QString quoted(const QStringList &names)
{
	return QStringLiteral("\"") + names.join(QStringLiteral("\", \"")) + QStringLiteral("\"");
}

	//The keys of the rows restore() read, each once: every row must be an
	//item's, on the folio the item is on -- nothing in the file, which is
	//untrusted, enforces it
struct UsedRows
{
	QSet<QString> folios, elements, conductors;
	bool use(QSet<QString> &used, const QString &key, const QString &what, QString *error) {
		if (used.contains(key)) {
			setError(error, QStringLiteral("%1 %2 is in the folios twice").arg(what, key));
			return false;
		}
		used.insert(key);
		return true;
	}
};

	//The engineering data of the folio @p d back from @p db
bool restoreDiagram(Database &db, QDomElement d, UsedRows &used, QString *error)
{
	QDomDocument document = d.ownerDocument();
	auto entry = [&document](const QVariant &name_attr, const QString &name, const QVariant &value,
							 const QVariant &attrs, const QString &default_tag, const QVariant &show = QVariant()) {
		return entryElement(document, name_attr, name, value, attrs, default_tag, show);
	};
	const QString id = keyOf(db, d, FolioAttribute);
	if (id.isEmpty()) return true;
	if (!used.use(used.folios, id, QStringLiteral("folio"), error)) return false;
	Statement f(db, QStringLiteral("SELECT %1, props_attrs FROM folio WHERE %2 = ?")
				.arg(quoted(FolioData), db.folioKey()));
	f.bind({id});
	if (!f.next()) { setError(error, QStringLiteral("folio %1 is not in the database").arg(id)); return false; }
	QVariantList values;
	for (int i = 0 ; i < FolioData.size() + 1 ; ++i) values << f.value(i);
	put(d, FolioData, values, 0);
		//A moved block goes back where its marker is: the marker says it
		//was there
	if (hasMarker(d, QStringLiteral("properties"))) {
		QDomElement props = document.createElement(QStringLiteral("properties"));
		setAttributesJson(props, values.at(FolioData.size()));
		Statement p(db, QStringLiteral("SELECT name, value, attrs, %1 FROM folio_property WHERE %2 = ? ORDER BY ord")
					.arg(db.showColumn(), db.folioRef()));
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

	const QStringList element_data = db.elementData();
	const QStringList conductor_data = db.conductorData();
	for (const QDomElement &list : children(d, QStringLiteral("elements"))) {
		for (QDomElement e : children(list, QStringLiteral("element"))) {
			const QString key = keyOf(db, e, KeyAttribute);
			if (key.isEmpty()) continue;
			if (!used.use(used.elements, key, QStringLiteral("symbol"), error)) return false;
			Statement s(db, QStringLiteral("SELECT %1, info_attrs, links_attrs, %2 FROM element WHERE %3 = ?")
						.arg(quoted(element_data), db.folioRef(), db.itemKey()));
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
				Statement i(db, QStringLiteral("SELECT name, value, attrs, %1 FROM element_info WHERE %2 = ? ORDER BY ord")
							.arg(db.showColumn(), db.elementRef()));
				i.bind({key});
				while (i.next())
					info.appendChild(entry(i.value(0), QStringLiteral("name"), i.value(1), i.value(2),
										   QStringLiteral("elementInformation"), i.value(3)));
				restoreWhitespace(info, v.at(n));
			}
			if (hasMarker(e, QStringLiteral("links_uuids"))) {
				links = document.createElement(QStringLiteral("links_uuids"));
				setAttributesJson(links, v.at(n + 1));
				Statement l(db, QStringLiteral("SELECT linked_uuid, attrs FROM link WHERE %1 = ? ORDER BY ord")
							.arg(db.elementRef()));
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
			const QString key = keyOf(db, c, KeyAttribute);
			if (key.isEmpty()) continue;
			if (!used.use(used.conductors, key, QStringLiteral("wire"), error)) return false;
			Statement s(db, QStringLiteral("SELECT %1, %2 FROM conductor WHERE %3 = ?")
						.arg(quoted(conductor_data), db.folioRef(), db.itemKey()));
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
	return true;
}

	//No row of @p db left over once restoreDiagram() read every folio:
	//each table's rows, counted, are the items read (@p used), and every
	//entry belongs to a row
bool noRowLeft(const Database &db, const UsedRows &used, QString *error)
{
	auto count = [&db](const QString &sql) {
		Statement s(db, sql);
		return s.next() ? s.value(0).toInt() : -1;
	};
	const struct { QString sql; int expected; QString what; } checks[] = {
		{QStringLiteral("SELECT count(*) FROM folio"), int(used.folios.size()), QStringLiteral("folio rows")},
		{QStringLiteral("SELECT count(*) FROM element"), int(used.elements.size()), QStringLiteral("symbol rows")},
		{QStringLiteral("SELECT count(*) FROM conductor"), int(used.conductors.size()), QStringLiteral("wire rows")},
		{QStringLiteral("SELECT count(*) FROM folio_property WHERE %1 NOT IN (SELECT %2 FROM folio)")
				.arg(db.folioRef(), db.folioKey()), 0, QStringLiteral("folio property rows of no folio")},
		{QStringLiteral("SELECT count(*) FROM element_info WHERE %1 NOT IN (SELECT %2 FROM element)")
				.arg(db.elementRef(), db.itemKey()), 0, QStringLiteral("information rows of no symbol")},
		{QStringLiteral("SELECT count(*) FROM link WHERE %1 NOT IN (SELECT %2 FROM element)")
				.arg(db.elementRef(), db.itemKey()), 0, QStringLiteral("link rows of no symbol")}};
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
}

/**
	@brief QetContainerDb::extract
	Move the engineering data of @p project (the <project> element, its
	folios still in it) into a new database.
	@return the database file, or an empty array with @p error set
*/
QByteArray QetContainerDb::extract(QDomElement project, QString *error, const QByteArray &kept_from)
{
	Database db;
	if (sqlite3_open(":memory:", &db.m_db) != SQLITE_OK || !db.exec(schema())) {
		setError(error, QStringLiteral("cannot create the project database"));
		return {};
	}
		//The database of the .qetz this project was read from, for the folios
		//kept as they were (stage D4): of this format (currentFormat())
	if (!kept_from.isEmpty()) {
		unsigned char *copy = static_cast<unsigned char *>(sqlite3_malloc64(sqlite3_uint64(kept_from.size())));
		if (!copy) { setError(error, QStringLiteral("out of memory")); return {}; }
		memcpy(copy, kept_from.constData(), size_t(kept_from.size()));
		if (!db.exec(QStringLiteral("ATTACH DATABASE ':memory:' AS old"))
				|| sqlite3_deserialize(db.m_db, "old", copy, kept_from.size(), kept_from.size(),
									   SQLITE_DESERIALIZE_FREEONCLOSE | SQLITE_DESERIALIZE_READONLY) != SQLITE_OK) {
			setError(error, QStringLiteral("cannot read the database the kept folios come from"));
			return {};
		}
		db.exec(QStringLiteral("PRAGMA trusted_schema = OFF"));
		for (const QString &table : DocTables) {
			Statement kind(db, QStringLiteral("SELECT type FROM old.sqlite_master WHERE name = ?"));
			kind.bind({table});
			if (!kind.next() || kind.value(0).toString() != QLatin1String("table")) {
				setError(error, QStringLiteral("the kept folios' database has no table %1").arg(table));
				return {};
			}
		}
	}
	db.exec(QStringLiteral("BEGIN"));

	Statement meta(db, QStringLiteral("INSERT INTO meta VALUES (?,?)"));
	Statement project_property(db, QStringLiteral("INSERT INTO project_property VALUES (?,?,?,?,?)"));
	Statement folio(db, QStringLiteral("INSERT INTO folio VALUES (?,?,%1,?)").arg(placeholders(FolioData.size())));
	Statement folio_property(db, QStringLiteral("INSERT INTO folio_property VALUES (?,?,?,?,?,?)"));
	Statement element(db, QStringLiteral("INSERT INTO element VALUES (?,?,%1,?,?)").arg(placeholders(ElementData.size())));
	Statement element_info(db, QStringLiteral("INSERT INTO element_info VALUES (?,?,?,?,?,?)"));
	Statement link(db, QStringLiteral("INSERT INTO link VALUES (?,?,?,?)"));
	Statement conductor(db, QStringLiteral("INSERT INTO conductor VALUES (?,?,%1)").arg(placeholders(ConductorData.size())));
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
		//The <elementInformation> entries of @p block, as the rows of the
		//symbol @p key
	auto moveInformation = [&](const QDomElement &block, const QString &key) {
		int ord = 0;
		for (QDomElement i : children(block, QStringLiteral("elementInformation")))
			ok = ok && element_info.run(QVariantList{key, ord++} << entryRow(i, QStringLiteral("elementInformation")));
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
		//The keys of the rows the folios kept as they were bring, which no
		//other item may have
	QSet<QString> kept_elements, kept_conductors;
	for (const QDomElement &d : diagrams) {
		if (!d.hasAttribute(KeptPartAttribute)) continue;
		Statement e(db, QStringLiteral("SELECT uuid FROM old.element WHERE folio_uuid = ?"));
		e.bind({d.attribute(QStringLiteral("uuid"))});
		while (e.next()) kept_elements.insert(e.value(0).toString());
		Statement c(db, QStringLiteral("SELECT uuid FROM old.conductor WHERE folio_uuid = ?"));
		c.bind({d.attribute(QStringLiteral("uuid"))});
		while (c.next()) kept_conductors.insert(c.value(0).toString());
	}
	QList<bool> folio_own, element_own, conductor_own;
	const QStringList folio_keys = keys(diagrams, &folio_own);
	const QStringList element_keys = keys(all_elements, &element_own, kept_elements);
	const QStringList conductor_keys = keys(all_conductors, &conductor_own, kept_conductors);
	int e_index = 0, c_index = 0;

	int pos = 0;
	for (QDomElement d : diagrams)
	{
		const QString id = folio_keys.at(pos);
		const bool own_id = folio_own.at(pos);
		++pos;
			//Kept as it was: its rows from the database it was read from,
			//under its uuid
		if (d.hasAttribute(KeptPartAttribute)) {
			if (kept_from.isEmpty() || !own_id) {
				setError(error, QStringLiteral("a folio kept as it was, without its database"));
				return {};
			}
			Statement copy(db, QStringLiteral("INSERT INTO folio SELECT uuid, ?, %1, props_attrs "
											  "FROM old.folio WHERE uuid = ?").arg(quoted(FolioData)));
			ok = ok && copy.run({pos, id}) && sqlite3_changes(db.m_db) == 1;
			for (const QString &sql : {
					 QStringLiteral("INSERT INTO folio_property SELECT * FROM old.folio_property WHERE folio_uuid = ? ORDER BY rowid"),
					 QStringLiteral("INSERT INTO element SELECT * FROM old.element WHERE folio_uuid = ? ORDER BY rowid"),
					 QStringLiteral("INSERT INTO element_info SELECT * FROM old.element_info WHERE element_uuid IN "
									"(SELECT uuid FROM old.element WHERE folio_uuid = ?) ORDER BY rowid"),
					 QStringLiteral("INSERT INTO link SELECT * FROM old.link WHERE element_uuid IN "
									"(SELECT uuid FROM old.element WHERE folio_uuid = ?) ORDER BY rowid"),
					 QStringLiteral("INSERT INTO conductor SELECT * FROM old.conductor WHERE folio_uuid = ? ORDER BY rowid")}) {
				Statement rows(db, sql);
				ok = ok && rows.run({id});
			}
				//Its symbols whose information changed since (QETProject::toXml()):
				//their information rows written as a symbol's block writes them
			for (const QDomElement &edited : children(d, QStringLiteral("qetz-edited-element"))) {
					//Its part has the place they go back to (QetContainer::split())
				Statement find(db, QStringLiteral("SELECT uuid FROM element WHERE folio_uuid = ? AND uuid = ?"));
				find.bind({id, edited.attribute(QStringLiteral("uuid"))});
				if (!find.next()) {
					setError(error, QStringLiteral("a symbol changed on folio %1 kept as it was is not in its database").arg(id));
					return {};
				}
				const QString key = find.value(0).toString();
				Statement drop(db, QStringLiteral("DELETE FROM element_info WHERE element_uuid = ?"));
				ok = ok && drop.run({key});
				moveInformation(edited.firstChildElement(QStringLiteral("elementInformations")), key);
			}
				//Its wires whose properties changed since: their columns
			for (const QDomElement &edited : children(d, QStringLiteral("qetz-edited-conductor"))) {
				QStringList set;
				QVariantList values;
				for (const QString &column : ConductorData) {
					if (column.startsWith(QLatin1String("element")) || column.startsWith(QLatin1String("terminal"))
							|| BooleanData.contains(column) || !edited.hasAttribute(column))
						continue;
					set << QStringLiteral("\"%1\" = ?").arg(column);
					values << edited.attribute(column);
				}
				values << id << edited.attribute(QStringLiteral("uuid"));
				Statement update(db, QStringLiteral("UPDATE conductor SET %1 WHERE folio_uuid = ? AND uuid = ?")
								 .arg(set.join(QStringLiteral(", "))));
				ok = ok && !set.isEmpty() && update.run(values) && sqlite3_changes(db.m_db) == 1;
			}
			if (!ok) {
				setError(error, QStringLiteral("folio %1 kept as it was is not in its database").arg(id));
				return {};
			}
			continue;
		}
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
				if (!info.isNull())
					moveInformation(info, key);
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
		Statement st(db, QStringLiteral("INSERT INTO terminal_strip VALUES (?,?,?,?,?,?,?)"));
		Statement tt(db, QStringLiteral("INSERT INTO strip_terminal VALUES (?,?,?,?)"));
		Statement bt(db, QStringLiteral("INSERT INTO strip_bridge VALUES (?,?,?,?)"));
		Statement btt(db, QStringLiteral("INSERT INTO strip_bridge_terminal VALUES (?,?,?)"));
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
	if (!kept_from.isEmpty()) {
		sqlite3_exec(db.m_db, "DETACH DATABASE old", nullptr, nullptr, nullptr);
	}
	if (!plainTables(db.m_db)) {
		setError(error, QStringLiteral("cannot write the project database: %1").arg(db.error()));
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
	if (!openDatabase(db, database, error))
		return false;

		//Project properties
	QHash<QString, QVariant> meta;
	{
		Statement s(db, QStringLiteral("SELECT key, value FROM meta"));
		while (s.next()) meta.insert(s.value(0).toString(), s.value(1));
	}
	if (hasMarker(project, QStringLiteral("properties"))) {
		QDomElement props = document.createElement(QStringLiteral("properties"));
		setAttributesJson(props, meta.value(QStringLiteral("props_attrs")));
		Statement s(db, QStringLiteral("SELECT name, value, attrs, %1 FROM project_property ORDER BY ord")
					.arg(db.showColumn()));
		while (s.next())
			props.appendChild(entryElement(document, s.value(0), QStringLiteral("name"), s.value(1), s.value(2),
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
			Statement s(db, sql);
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

	UsedRows used;
	for (QDomElement d : children(project, QStringLiteral("diagram")))
		if (!restoreDiagram(db, d, used, error))
			return false;
	return noRowLeft(db, used, error);
}

/**
	@brief QetContainerDb::currentFormat
	@return true if @p database, a project.sqlite, is of the format this
	writes: a folio written as it was read keeps its rows, copied as they
	are (stage D4)
*/
bool QetContainerDb::currentFormat(const QByteArray &database)
{
	Database db;
	if (!openDatabase(db, database, nullptr) || db.m_format != QetContainer::Format)
		return false;
		//Its rows are copied as they are: the same columns as this writes
	const QList<std::pair<QString, int>> columns = {
		{QStringLiteral("folio"), int(FolioData.size()) + 3},
		{QStringLiteral("element"), int(ElementData.size()) + 4},
		{QStringLiteral("conductor"), int(ConductorData.size()) + 2},
		{QStringLiteral("folio_property"), 6}, {QStringLiteral("element_info"), 6}};
	for (const auto &table : columns) {
		Statement s(db, QStringLiteral("SELECT count(*) FROM pragma_table_info('%1')").arg(table.first));
		if (!s.next() || s.value(0).toInt() != table.second)
			return false;
	}
	return true;
}
