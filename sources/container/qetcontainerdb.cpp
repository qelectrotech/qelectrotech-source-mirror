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

#include <QRegularExpression>

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QUuid>
#include <QVariant>

#include <sqlite3.h>

namespace {
const QString KeyAttribute = QStringLiteral("qetz-key");
	//A folio written as it was read (stage D4): its part
const QString KeptPartAttribute = QStringLiteral("qetz-kept-part");
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

const QStringList FolioData = {
	QStringLiteral("title"), QStringLiteral("author"), QStringLiteral("date"),
	QStringLiteral("folio"), QStringLiteral("indexrev"), QStringLiteral("filename"),
	QStringLiteral("locmach"), QStringLiteral("plant"), QStringLiteral("auto_page_num")};
const QStringList ElementData = {
	QStringLiteral("uuid"), QStringLiteral("type"), QStringLiteral("prefix"),
	QStringLiteral("freezeLabel")};
const QStringList ConductorData = {
	QStringLiteral("uuid"), QStringLiteral("element1"), QStringLiteral("terminal1"),
	QStringLiteral("element2"), QStringLiteral("terminal2"), QStringLiteral("num"),
	QStringLiteral("formula"), QStringLiteral("function"), QStringLiteral("tension_protocol"),
	QStringLiteral("conductor_color"), QStringLiteral("conductor_section"),
	QStringLiteral("cable"), QStringLiteral("bus"), QStringLiteral("freezeLabel")};

QString columns(const QStringList &names)
{
	QStringList quoted;
	for (const QString &n : names) quoted << QStringLiteral("\"%1\" TEXT").arg(n);
	return quoted.join(QStringLiteral(", "));
}

QString schema()
{
	return QStringLiteral(
		"CREATE TABLE meta (key TEXT PRIMARY KEY, value TEXT);"
		"CREATE TABLE project_property (ord INTEGER PRIMARY KEY, name TEXT, value TEXT, attrs TEXT);"
		"CREATE TABLE folio (id TEXT PRIMARY KEY, pos INTEGER NOT NULL UNIQUE, %1,"
		" props_at INTEGER, props_attrs TEXT);"
		"CREATE TABLE folio_property (folio_id TEXT NOT NULL REFERENCES folio, ord INTEGER NOT NULL,"
		" name TEXT, value TEXT, attrs TEXT, PRIMARY KEY (folio_id, ord));"
		"CREATE TABLE element (key TEXT PRIMARY KEY, folio_id TEXT NOT NULL REFERENCES folio, %2,"
		" info_at INTEGER, info_attrs TEXT, links_at INTEGER, links_attrs TEXT);"
		"CREATE TABLE element_info (element_key TEXT NOT NULL REFERENCES element, ord INTEGER NOT NULL,"
		" name TEXT, value TEXT, attrs TEXT, PRIMARY KEY (element_key, ord));"
		"CREATE TABLE link (element_key TEXT NOT NULL REFERENCES element, ord INTEGER NOT NULL,"
		" linked_uuid TEXT, attrs TEXT, PRIMARY KEY (element_key, ord));"
		"CREATE TABLE conductor (key TEXT PRIMARY KEY, folio_id TEXT NOT NULL REFERENCES folio, %3);"
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

	//The tables of a document's engineering data. From format 3 on they are
	//named doc_<name>, beside the project database's own tables in the same
	//file (stage D3)
const QStringList DocTables = {
	QStringLiteral("strip_bridge_terminal"), QStringLiteral("strip_bridge"),
	QStringLiteral("strip_terminal"), QStringLiteral("terminal_strip"),
	QStringLiteral("project_property"), QStringLiteral("folio_property"), QStringLiteral("folio"),
	QStringLiteral("element_info"), QStringLiteral("element"), QStringLiteral("link"),
	QStringLiteral("conductor"), QStringLiteral("meta")};

	//Owns an sqlite3 handle; its SQL names the document's tables by their
	//format 2 names, given their prefix here
class Database
{
	public:
		~Database() { if (m_db) sqlite3_close(m_db); }
		sqlite3 *m_db = nullptr;
		QString m_prefix;
		QString sql(const QString &text) const {
			if (m_prefix.isEmpty()) return text;
			static const QRegularExpression names(QStringLiteral("(?<![\"\\w])(%1)(?![\\w\"])")
												  .arg(DocTables.join(QLatin1Char('|'))));
			QString out = text;
			return out.replace(names, m_prefix + QStringLiteral("\\1"));
		}
		QString table(const QString &name) const { return m_prefix + name; }
		bool exec(const QString &text) {
			return sqlite3_exec(m_db, sql(text).toUtf8().constData(), nullptr, nullptr, nullptr) == SQLITE_OK;
		}
		QString error() const { return QString::fromUtf8(sqlite3_errmsg(m_db)); }
};

	//Owns a prepared statement; binds and reads QVariants (null = SQL NULL)
class Statement
{
	public:
		Statement(const Database &db, const QString &sql) {
			sqlite3_prepare_v2(db.m_db, db.sql(sql).toUtf8().constData(), -1, &m_stmt, nullptr);
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

	//The attributes of @p e but @p skip, and its tag, as JSON
QVariant attributesJson(const QDomElement &e, const QString &skip = QString(), bool with_tag = false)
{
	QJsonObject o;
	if (with_tag) o.insert(QStringLiteral("tag"), e.tagName());
	const QDomNamedNodeMap map = e.attributes();
	for (int i = 0 ; i < map.count() ; ++i) {
		const QDomAttr a = map.item(i).toAttr();
		if (a.name() != skip) o.insert(a.name(), a.value());
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

int childIndex(const QDomNode &child)
{
	int i = 0;
	for (QDomNode n = child.parentNode().firstChild() ; !n.isNull() ; n = n.nextSibling(), ++i)
		if (n == child) return i;
	return -1;
}


QVariantList take(QDomElement e, const QStringList &names)
{
	QVariantList values;
	for (const QString &n : names) {
		values << (e.hasAttribute(n) ? QVariant(e.attribute(n)) : QVariant());
		e.removeAttribute(n);
	}
	return values;
}

void put(QDomElement e, const QStringList &names, const QVariantList &values, int from)
{
	for (int i = 0 ; i < names.size() ; ++i)
		if (!values.at(from + i).isNull()) e.setAttribute(names.at(i), values.at(from + i).toString());
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

	//uuid when present and unique among @p items, else prefix + count
QStringList keys(const QList<QDomElement> &items, const QString &prefix)
{
	QHash<QString, int> seen;
	for (const QDomElement &e : items)
		if (e.hasAttribute(QStringLiteral("uuid"))) ++seen[e.attribute(QStringLiteral("uuid"))];
	QStringList out;
	int made_up = 0;
	for (const QDomElement &e : items) {
		const QString uuid = e.attribute(QStringLiteral("uuid"));
		out << ((!uuid.isEmpty() && seen.value(uuid) == 1)
				? uuid : prefix + QString::number(++made_up));
	}
	return out;
}
}

namespace {
	//@p database opened read only in @p db, its tables checked to be plain
	//tables: a file is untrusted
bool openDatabase(Database &db, const QByteArray &database, QString *error)
{
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
		//Format 3 names the document's tables doc_<name>, format 2 not
	{
		Statement doc(db, QStringLiteral("SELECT count(*) FROM sqlite_master WHERE name = 'doc_meta'"));
		if (doc.next() && doc.value(0).toInt() > 0) db.m_prefix = QStringLiteral("doc_");
	}
	for (const char *table : {"meta", "project_property", "folio", "folio_property",
							  "element", "element_info", "link", "conductor", "terminal_strip",
							  "strip_terminal", "strip_bridge", "strip_bridge_terminal"}) {
		Statement kind(db, QStringLiteral("SELECT type FROM sqlite_master WHERE name = ?"));
		kind.bind({db.table(QString::fromLatin1(table))});
		if (!kind.next() || kind.value(0).toString() != QLatin1String("table")) {
			setError(error, QStringLiteral("the project database has no table %1").arg(QLatin1String(table)));
			return false;
		}
	}
	return true;
}

QDomElement entryElement(QDomDocument &document, const QVariant &name_attr, const QString &name,
						 const QVariant &value, const QVariant &attrs, const QString &default_tag)
{
	const QJsonObject o = QJsonDocument::fromJson(attrs.toString().toUtf8()).object();
	QDomElement e = document.createElement(o.value(QStringLiteral("tag")).toString(default_tag));
	if (!name_attr.isNull()) e.setAttribute(name, name_attr.toString());
	setAttributesJson(e, attrs);
	if (!value.isNull()) e.appendChild(document.createTextNode(value.toString()));
	return e;
}

	//The engineering data of the folio @p d (its qetz-folio id) back from @p db
bool restoreDiagram(Database &db, QDomElement d, QString *error)
{
	QDomDocument document = d.ownerDocument();
	auto entry = [&document](const QVariant &name_attr, const QString &name, const QVariant &value,
							 const QVariant &attrs, const QString &default_tag) {
		return entryElement(document, name_attr, name, value, attrs, default_tag);
	};
	const QString id = d.attribute(FolioAttribute);
	if (id.isEmpty()) return true;
	d.removeAttribute(FolioAttribute);
	Statement f(db, QStringLiteral("SELECT %1, props_at, props_attrs FROM folio WHERE id = ?")
				.arg(QStringLiteral("\"") + FolioData.join(QStringLiteral("\", \"")) + QStringLiteral("\"")));
	f.bind({id});
	if (!f.next()) { setError(error, QStringLiteral("folio %1 is not in the database").arg(id)); return false; }
	QVariantList values;
	for (int i = 0 ; i < FolioData.size() + 2 ; ++i) values << f.value(i);
	put(d, FolioData, values, 0);
	if (!values.at(FolioData.size()).isNull()) {
		QDomElement props = document.createElement(QStringLiteral("properties"));
		setAttributesJson(props, values.at(FolioData.size() + 1));
		Statement p(db, QStringLiteral("SELECT name, value, attrs FROM folio_property WHERE folio_id = ? ORDER BY ord"));
		p.bind({id});
		while (p.next())
			props.appendChild(entry(p.value(0), QStringLiteral("name"), p.value(1), p.value(2),
									QStringLiteral("property")));
		restoreWhitespace(props, values.at(FolioData.size() + 1));
		if (!replaceMarker(d, props)) {
			setError(error, QStringLiteral("folio %1's properties have no place").arg(id));
			return false;
		}
	}

	for (const QDomElement &list : children(d, QStringLiteral("elements"))) {
		for (QDomElement e : children(list, QStringLiteral("element"))) {
			if (!e.hasAttribute(KeyAttribute)) continue;
			const QString key = e.attribute(KeyAttribute);
			e.removeAttribute(KeyAttribute);
			Statement s(db, QStringLiteral("SELECT %1, info_at, info_attrs, links_at, links_attrs FROM element WHERE key = ?")
						.arg(QStringLiteral("\"") + ElementData.join(QStringLiteral("\", \"")) + QStringLiteral("\"")));
			s.bind({key});
			if (!s.next()) { setError(error, QStringLiteral("symbol %1 is not in the database").arg(key)); return false; }
			QVariantList v;
			for (int i = 0 ; i < ElementData.size() + 4 ; ++i) v << s.value(i);
			put(e, ElementData, v, 0);
			const int n = ElementData.size();
			QDomElement info, links;
			if (!v.at(n).isNull()) {
				info = document.createElement(QStringLiteral("elementInformations"));
				setAttributesJson(info, v.at(n + 1));
				Statement i(db, QStringLiteral("SELECT name, value, attrs FROM element_info WHERE element_key = ? ORDER BY ord"));
				i.bind({key});
				while (i.next())
					info.appendChild(entry(i.value(0), QStringLiteral("name"), i.value(1), i.value(2),
										   QStringLiteral("elementInformation")));
				restoreWhitespace(info, v.at(n + 1));
			}
			if (!v.at(n + 2).isNull()) {
				links = document.createElement(QStringLiteral("links_uuids"));
				setAttributesJson(links, v.at(n + 3));
				Statement l(db, QStringLiteral("SELECT linked_uuid, attrs FROM link WHERE element_key = ? ORDER BY ord"));
				l.bind({key});
				while (l.next())
					links.appendChild(entry(l.value(0), QStringLiteral("uuid"), QVariant(), l.value(1),
											QStringLiteral("link_uuid")));
				restoreWhitespace(links, v.at(n + 3));
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
			if (!c.hasAttribute(KeyAttribute)) continue;
			const QString key = c.attribute(KeyAttribute);
			c.removeAttribute(KeyAttribute);
			Statement s(db, QStringLiteral("SELECT %1 FROM conductor WHERE key = ?")
						.arg(QStringLiteral("\"") + ConductorData.join(QStringLiteral("\", \"")) + QStringLiteral("\"")));
			s.bind({key});
			if (!s.next()) { setError(error, QStringLiteral("wire %1 is not in the database").arg(key)); return false; }
			QVariantList v;
			for (int i = 0 ; i < ConductorData.size() ; ++i) v << s.value(i);
			put(c, ConductorData, v, 0);
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
QByteArray QetContainerDb::extract(QDomElement project, QString *error, const QByteArray &index,
								   const QByteArray &kept_from)
{
	Database db;
	db.m_prefix = QStringLiteral("doc_");
	if (sqlite3_open(":memory:", &db.m_db) != SQLITE_OK) {
		setError(error, QStringLiteral("cannot create the project database"));
		return {};
	}
		//The project database's own tables first, when given: one file
		//holds both (stage D3)
	if (!index.isEmpty()) {
		unsigned char *copy = static_cast<unsigned char *>(sqlite3_malloc64(sqlite3_uint64(index.size())));
		if (!copy) { setError(error, QStringLiteral("out of memory")); return {}; }
		memcpy(copy, index.constData(), size_t(index.size()));
		if (sqlite3_deserialize(db.m_db, "main", copy, index.size(), index.size(),
								SQLITE_DESERIALIZE_FREEONCLOSE | SQLITE_DESERIALIZE_RESIZEABLE) != SQLITE_OK) {
			setError(error, QStringLiteral("cannot read the project database's index"));
			return {};
		}
	}
	if (!db.exec(schema())) {
		setError(error, QStringLiteral("cannot create the project database"));
		return {};
	}
		//The database of the .qetz this project was read from, for the folios
		//kept as they were (stage D4)
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
			kind.bind({db.table(table)});
			if (!kind.next() || kind.value(0).toString() != QLatin1String("table")) {
				setError(error, QStringLiteral("the kept folios' database has no table %1").arg(table));
				return {};
			}
		}
	}
	db.exec(QStringLiteral("BEGIN"));

	Statement meta(db, QStringLiteral("INSERT INTO meta VALUES (?,?)"));
	Statement project_property(db, QStringLiteral("INSERT INTO project_property VALUES (?,?,?,?)"));
	Statement folio(db, QStringLiteral("INSERT INTO folio VALUES (?,?,%1,?,?)").arg(placeholders(FolioData.size())));
	Statement folio_property(db, QStringLiteral("INSERT INTO folio_property VALUES (?,?,?,?,?)"));
	Statement element(db, QStringLiteral("INSERT INTO element VALUES (?,?,%1,?,?,?,?)").arg(placeholders(ElementData.size())));
	Statement element_info(db, QStringLiteral("INSERT INTO element_info VALUES (?,?,?,?,?)"));
	Statement link(db, QStringLiteral("INSERT INTO link VALUES (?,?,?,?)"));
	Statement conductor(db, QStringLiteral("INSERT INTO conductor VALUES (?,?,%1)").arg(placeholders(ConductorData.size())));
	bool ok = meta.ok() && project_property.ok() && folio.ok() && folio_property.ok()
			&& element.ok() && element_info.ok() && link.ok() && conductor.ok();

	ok = ok && meta.run({QStringLiteral("format"), QStringLiteral("3")});

		//<properties> of a block, as rows of @p rows (with @p prefix values)
	auto moveProperties = [&](QDomElement block, Statement &rows, const QVariantList &prefix) {
		int ord = 0;
		for (QDomElement p : children(block, QStringLiteral("property"))) {
			QVariant value;
			simpleValue(p, &value);
			ok = ok && rows.run(QVariantList(prefix) << ord++ << (p.hasAttribute(QStringLiteral("name"))
													   ? QVariant(p.attribute(QStringLiteral("name"))) : QVariant())
							   << value << attributesJson(p, QStringLiteral("name"), true));
		}
	};

		//Project properties
	const QDomElement project_props = project.firstChildElement(QStringLiteral("properties"));
	if (!project_props.isNull() && listBlock(project_props, QStringLiteral("property"))) {
		ok = ok && meta.run({QStringLiteral("props_at"), QString::number(childIndex(project_props))});
		ok = ok && meta.run({QStringLiteral("props_attrs"), blockJson(project_props)});
		int ord = 0;
		for (QDomElement p : children(project_props, QStringLiteral("property"))) {
			QVariant value;
			simpleValue(p, &value);
			ok = ok && project_property.run({ord++, p.hasAttribute(QStringLiteral("name"))
												? QVariant(p.attribute(QStringLiteral("name"))) : QVariant(),
											 value, attributesJson(p, QStringLiteral("name"), true)});
		}
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
	const QStringList element_keys = keys(all_elements, QStringLiteral("e"));
	const QStringList conductor_keys = keys(all_conductors, QStringLiteral("c"));
	int e_index = 0, c_index = 0;

		//A folio's id is its uuid when every folio has one of its own, so
		//that a folio kept as it was keeps its rows (format 3, D4)
	bool uuid_ids = true;
	{
		QSet<QString> seen;
		for (const QDomElement &d : diagrams) {
			const QString uuid = d.attribute(QStringLiteral("uuid"));
			if (QUuid(uuid).isNull() || seen.contains(uuid)) { uuid_ids = false; break; }
			seen.insert(uuid);
		}
	}
	if (uuid_ids) {
		ok = ok && meta.run({QStringLiteral("folio_ids"), QStringLiteral("uuid")});
	}

	int pos = 0;
	for (QDomElement d : diagrams)
	{
		++pos;
		const QString id = uuid_ids ? d.attribute(QStringLiteral("uuid")) : QStringLiteral("f%1").arg(pos);
			//Kept as it was: its rows from the database it was read from
		if (d.hasAttribute(KeptPartAttribute)) {
			if (kept_from.isEmpty() || !uuid_ids) {
				setError(error, QStringLiteral("a folio kept as it was, without its database"));
				return {};
			}
			Statement copy(db, QStringLiteral("INSERT INTO folio SELECT id, ?, %1, props_at, props_attrs "
											  "FROM old.folio WHERE id = ?")
						   .arg(QStringLiteral("\"") + FolioData.join(QStringLiteral("\", \"")) + QStringLiteral("\"")));
			ok = ok && copy.run({pos, id}) && sqlite3_changes(db.m_db) == 1;
			for (const QString &sql : {
					 QStringLiteral("INSERT INTO folio_property SELECT * FROM old.folio_property WHERE folio_id = ? ORDER BY rowid"),
					 QStringLiteral("INSERT INTO element SELECT * FROM old.element WHERE folio_id = ? ORDER BY rowid"),
					 QStringLiteral("INSERT INTO element_info SELECT * FROM old.element_info WHERE element_key IN "
									"(SELECT key FROM old.element WHERE folio_id = ?) ORDER BY rowid"),
					 QStringLiteral("INSERT INTO link SELECT * FROM old.link WHERE element_key IN "
									"(SELECT key FROM old.element WHERE folio_id = ?) ORDER BY rowid"),
					 QStringLiteral("INSERT INTO conductor SELECT * FROM old.conductor WHERE folio_id = ? ORDER BY rowid")}) {
				Statement rows(db, sql);
				ok = ok && rows.run({id});
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
		row << (move_props ? QVariant(childIndex(props)) : QVariant())
			<< (move_props ? blockJson(props) : QVariant());
		ok = ok && folio.run(row);
		if (move_props) {
			moveProperties(props, folio_property, {id});
			leaveMarker(props);
		}
		d.setAttribute(FolioAttribute, id);

		for (const QDomElement &list : children(d, QStringLiteral("elements"))) {
			for (QDomElement e : children(list, QStringLiteral("element"))) {
				const QString key = element_keys.at(e_index++);
				QVariantList erow{key, id};
				erow += take(e, ElementData);
				QDomElement info = e.firstChildElement(QStringLiteral("elementInformations"));
				QDomElement links = e.firstChildElement(QStringLiteral("links_uuids"));
				if (!info.isNull() && !listBlock(info, QStringLiteral("elementInformation"))) info = QDomElement();
				if (!links.isNull() && !listBlock(links, QStringLiteral("link_uuid"))) links = QDomElement();
				const int info_at = info.isNull() ? -1 : childIndex(info);
				const int links_at = links.isNull() ? -1 : childIndex(links);
				erow << (info_at < 0 ? QVariant() : QVariant(info_at))
					 << (info_at < 0 ? QVariant() : blockJson(info))
					 << (links_at < 0 ? QVariant() : QVariant(links_at))
					 << (links_at < 0 ? QVariant() : blockJson(links));
				ok = ok && element.run(erow);
				if (info_at >= 0) {
					int ord = 0;
					for (QDomElement i : children(info, QStringLiteral("elementInformation"))) {
						QVariant value;
						simpleValue(i, &value);
						ok = ok && element_info.run({key, ord++, i.hasAttribute(QStringLiteral("name"))
														? QVariant(i.attribute(QStringLiteral("name"))) : QVariant(),
													 value, attributesJson(i, QStringLiteral("name"), true)});
					}
				}
				if (links_at >= 0) {
					int ord = 0;
					for (QDomElement l : children(links, QStringLiteral("link_uuid")))
						ok = ok && link.run({key, ord++, l.hasAttribute(QStringLiteral("uuid"))
												? QVariant(l.attribute(QStringLiteral("uuid"))) : QVariant(),
											 attributesJson(l, QStringLiteral("uuid"), true)});
				}
				if (info_at >= 0) leaveMarker(info);
				if (links_at >= 0) leaveMarker(links);
				e.setAttribute(KeyAttribute, key);
			}
		}
		for (const QDomElement &list : children(d, QStringLiteral("conductors"))) {
			for (QDomElement c : children(list, QStringLiteral("conductor"))) {
				const QString key = conductor_keys.at(c_index++);
				QVariantList crow{key, id};
				crow += take(c, ConductorData);
				ok = ok && conductor.run(crow);
				c.setAttribute(KeyAttribute, key);
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

	auto entry = [&document](const QVariant &name_attr, const QString &name, const QVariant &value,
							 const QVariant &attrs, const QString &default_tag) {
		const QJsonObject o = QJsonDocument::fromJson(attrs.toString().toUtf8()).object();
		QDomElement e = document.createElement(o.value(QStringLiteral("tag")).toString(default_tag));
		if (!name_attr.isNull()) e.setAttribute(name, name_attr.toString());
		setAttributesJson(e, attrs);
		if (!value.isNull()) e.appendChild(document.createTextNode(value.toString()));
		return e;
	};

		//Project properties
	QHash<QString, QVariant> meta;
	{
		Statement s(db, QStringLiteral("SELECT key, value FROM meta"));
		while (s.next()) meta.insert(s.value(0).toString(), s.value(1));
	}
	if (meta.contains(QStringLiteral("props_at"))) {
		QDomElement props = document.createElement(QStringLiteral("properties"));
		setAttributesJson(props, meta.value(QStringLiteral("props_attrs")));
		Statement s(db, QStringLiteral("SELECT name, value, attrs FROM project_property ORDER BY ord"));
		while (s.next())
			props.appendChild(entry(s.value(0), QStringLiteral("name"), s.value(1), s.value(2),
									QStringLiteral("property")));
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

	for (QDomElement d : children(project, QStringLiteral("diagram")))
		if (!restoreDiagram(db, d, error))
			return false;
	return true;
}

/**
	@brief QetContainerDb::restoreFolio
	restore() for the one folio @p diagram, read from its own part (a folio
	built after its project was opened from a .qetz, stage D2)
*/
bool QetContainerDb::restoreFolio(QDomElement diagram, const QByteArray &database, QString *error)
{
	Database db;
	return openDatabase(db, database, error) && restoreDiagram(db, diagram, error);
}

/**
	@brief QetContainerDb::foliosByUuid
	@return true if @p database, a format 3 project.sqlite, names its
	folios by their uuid: a folio written as it was read keeps its rows
	under it (stage D4)
*/
bool QetContainerDb::foliosByUuid(const QByteArray &database)
{
	Database db;
	if (!openDatabase(db, database, nullptr) || db.m_prefix.isEmpty())
		return false;
	Statement s(db, QStringLiteral("SELECT value FROM meta WHERE key = 'folio_ids'"));
	return s.next() && s.value(0).toString() == QLatin1String("uuid");
}
