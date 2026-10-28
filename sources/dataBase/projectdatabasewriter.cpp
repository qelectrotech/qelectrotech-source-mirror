// SPDX-License-Identifier: GPL-2.0-or-later
/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "projectdatabase.h"

#include "../autoNum/assignvariables.h"
#include "../borderproperties.h"
#include "../itemgroups.h"
#include "../qetproject.h"
#include "../qetversion.h"
#include "../titleblockproperties.h"

#include "../qet.h"

#include <QSqlError>
#include <QElapsedTimer>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtConcurrentMap>
#include <QUuid>

/*
	The project written back from its database alone (QET-SQL-FORMAT S3z1):
	projectDataBase::toDocument() reads nothing but the tables of the
	database file it is given, through a connection of its own, and makes
	the <project> a save would write. What it gives back, opened and saved,
	must equal the project it came from saved: that is the proof that the
	database holds the whole project.
*/

namespace {

using Row = projectDataBase::ProjectSetting;
using Rows = QList<Row>;

/// Every row of @p query, which selects section, ord, name and value
Rows selectRows(QSqlQuery &query)
{
	Rows rows;
	while (query.next())
		rows.append({query.value(0).toString(), query.value(1).toInt(),
					 query.value(2).toString(), query.value(3).toString()});
	return rows;
}

/// The last tag of @p section, without the [n] a repeated item's children
/// carry
QString lastTag(const QString &section)
{
	QString tag = section.mid(section.lastIndexOf(QLatin1Char('/')) + 1);
	const int bracket = tag.indexOf(QLatin1Char('['));
	return bracket < 0 ? tag : tag.left(bracket);
}

/**
	The inverse of projectDataBase::xmlRows(): build under @p root, whose
	section is @p base, the elements @p rows name. A row's section and ord
	say which element it is in: the section of a child of a repeated item
	carries the item's ord ("a/b[2]/c"). The rows of an element whose tag
	is "properties" are <property> children (a DiagramContext), "#show"
	their show attribute; "#text" is an element's text.
*/
void rowsToXml(QDomDocument &document, QDomElement &root, const QString &base, const Rows &rows)
{
	QHash<QPair<QString, int>, QDomElement> nodes;
	nodes.insert({base, 0}, root);
	QHash<QPair<QString, int>, QHash<QString, QDomElement>> properties;

	std::function<QDomElement(const QString &, int)> node = [&](const QString &section, int ord) {
		const auto known = nodes.constFind({section, ord});
		if (known != nodes.constEnd()) return *known;
		const int slash = section.lastIndexOf(QLatin1Char('/'));
		QString parent = section.left(qMax(slash, 0));
		int parent_ord = 0;
		const QRegularExpressionMatch repeated =
				QRegularExpression(QStringLiteral("^(.*)\\[(\\d+)\\]$")).match(parent);
		if (repeated.hasMatch()) {
			parent = repeated.captured(1);
			parent_ord = repeated.captured(2).toInt();
		}
		QDomElement element = document.createElement(section.mid(slash + 1));
		if (slash > 0)
			node(parent, parent_ord).appendChild(element);
		nodes.insert({section, ord}, element);
		return element;
	};

	for (const Row &row : rows) {
		QDomElement element = node(row.section, row.ord);
		if (row.name == QLatin1String("#text")) {
			element.appendChild(document.createTextNode(row.value));
		} else if (lastTag(row.section) == QLatin1String("properties")) {
			auto &named = properties[{row.section, row.ord}];
			if (row.name.endsWith(QLatin1String("#show"))) {
				named.value(row.name.chopped(5)).setAttribute(QStringLiteral("show"), row.value);
				continue;
			}
			QDomElement property = document.createElement(QStringLiteral("property"));
			property.setAttribute(QStringLiteral("name"), row.name);
			property.appendChild(document.createTextNode(row.value));
			element.appendChild(property);
			named.insert(row.name, property);
		} else {
			element.setAttribute(row.name, row.value);
		}
	}
}

/// The rows @p query gives (key, section, ord, name, value), one <@p base>
/// child of @p parent per key, in the order the keys come
void keyedRowsToXml(QDomDocument &document, QDomElement &parent, QSqlQuery &query, const QString &base)
{
		//key, section, ord, name, value, in the order written
	QString current;
	Rows rows;
	auto flush = [&]() {
		if (current.isEmpty()) return;
		QDomElement element = document.createElement(base);
		rowsToXml(document, element, base, rows);
		parent.appendChild(element);
		rows.clear();
	};
	while (query.next()) {
		const QString key = query.value(0).toString();
		if (key != current) {
			flush();
			current = key;
		}
		rows.append({query.value(1).toString(), query.value(2).toInt(),
					 query.value(3).toString(), query.value(4).toString()});
	}
	flush();
}

/// The rows of one key of a table holding (key, section, ord, name, value)
Rows keyRows(const QSqlDatabase &db, const QString &table, const QString &column, const QString &key)
{
	QSqlQuery query(db);
	query.setForwardOnly(true);
	query.prepare(QStringLiteral("SELECT section, ord, name, value FROM %1 WHERE %2 = :key ORDER BY rowid")
				  .arg(table, column));
	query.bindValue(QStringLiteral(":key"), key);
	query.exec();
	return selectRows(query);
}

/// The numbers @p parts, (part, value) in part and ord order, make
autonum::sequentialNumbers sequenceFrom(const QList<std::pair<QString, QString>> &parts_values)
{
	autonum::sequentialNumbers numbers;
	const QHash<QString, QStringList *> parts = {
		{QStringLiteral("unit"), &numbers.unit}, {QStringLiteral("wrap"), &numbers.wrap},
		{QStringLiteral("unit_folio"), &numbers.unit_folio}, {QStringLiteral("ten"), &numbers.ten},
		{QStringLiteral("ten_folio"), &numbers.ten_folio}, {QStringLiteral("hundred"), &numbers.hundred},
		{QStringLiteral("hundred_folio"), &numbers.hundred_folio}, {QStringLiteral("alpha"), &numbers.alpha}};
	for (const auto &[part_name, value] : parts_values)
		if (QStringList *part = parts.value(part_name))
			part->append(value);
	return numbers;
}

autonum::sequentialNumbers sequenceOf(const QSqlDatabase &db, const QString &item)
{
	QSqlQuery query(db);
	query.setForwardOnly(true);
	query.prepare(QStringLiteral("SELECT part, value FROM sequential_number WHERE item_uuid = :uuid ORDER BY part, ord"));
	query.bindValue(QStringLiteral(":uuid"), item);
	query.exec();
	QList<std::pair<QString, QString>> parts_values;
	while (query.next())
		parts_values.append({query.value(0).toString(), query.value(1).toString()});
	return sequenceFrom(parts_values);
}

/// What a folio's title block saves, from its folio_titleblock rows
TitleBlockProperties titleBlockOf(const QSqlDatabase &db, const QString &folio)
{
	TitleBlockProperties properties;
	QSqlQuery query(db);
	query.setForwardOnly(true);
	query.prepare(QStringLiteral("SELECT name, value, show FROM folio_titleblock WHERE diagram_uuid = :uuid ORDER BY ord"));
	query.bindValue(QStringLiteral(":uuid"), folio);
	query.exec();
	while (query.next()) {
		const QString name = query.value(0).toString();
		const QString value = query.value(1).toString();
		if (name == QLatin1String("title")) properties.title = value;
		else if (name == QLatin1String("author")) properties.author = value;
		else if (name == QLatin1String("date")) properties.date = QDate::fromString(value, Qt::ISODate);
		else if (name == QLatin1String("filename")) properties.filename = value;
		else if (name == QLatin1String("plant")) properties.plant = value;
		else if (name == QLatin1String("locmach")) properties.locmach = value;
		else if (name == QLatin1String("indexrev")) properties.indexrev = value;
		else if (name == QLatin1String("version")) properties.version = value;
		else if (name == QLatin1String("folio")) properties.folio = value;
		else if (name == QLatin1String("auto_page_num")) properties.auto_page_num = value;
		else if (name == QLatin1String("template_name")) properties.template_name = value;
		else if (name == QLatin1String("display_at"))
			properties.display_at = value == QLatin1String("right") ? Qt::RightEdge : Qt::BottomEdge;
		else if (name.startsWith(QLatin1String("custom:")))
			properties.context.addValue(name.mid(7), value, query.value(2).toBool());
	}
		//What BorderTitleBlock::localTitleBlock() always gives
	properties.collection = QET::QetCollection::Embedded;
	return properties;
}

/// A folio's counters (Diagram::writeHeader()), from folio_counter
void writeFolioCounters(QDomDocument &document, QDomElement &folio, const QSqlDatabase &db, const QString &uuid)
{
	static const QList<std::pair<QString, QString>> owners = {
		{QStringLiteral("element"), QStringLiteral("elementautonumfoliosequentials")},
		{QStringLiteral("conductor"), QStringLiteral("conductorautonumfoliosequentials")}};
	static const QList<std::tuple<QString, QString, QString, QString>> levels = {
		{QStringLiteral("unitfolio"), QStringLiteral("unitfolioseq"), QStringLiteral("unitfolioseq"), QStringLiteral("sequf_")},
		{QStringLiteral("tenfolio"), QStringLiteral("tenfolioseq"), QStringLiteral("tenfolioseq"), QStringLiteral("seqtf_")},
		{QStringLiteral("hundredfolio"), QStringLiteral("hundredfolioseq"), QStringLiteral("hundredfolioseq"), QStringLiteral("seqhf_")}};
	for (const auto &[owner, owner_tag] : owners) {
		QDomElement list = document.createElement(owner_tag);
		for (const auto &[level, level_suffix, item_tag, prefix] : levels) {
			QSqlQuery query(db);
			query.setForwardOnly(true);
			query.prepare(QStringLiteral("SELECT title, value FROM folio_counter WHERE folio_uuid = :uuid "
										 "AND owner = :owner AND level = :level ORDER BY title, ord"));
			query.bindValue(QStringLiteral(":uuid"), uuid);
			query.bindValue(QStringLiteral(":owner"), owner);
			query.bindValue(QStringLiteral(":level"), level);
			query.exec();
			QDomElement level_element = document.createElement(owner + level_suffix);
			QDomElement item;
			QString title;
			int n = 0;
			while (query.next()) {
				if (item.isNull() || query.value(0).toString() != title) {
					title = query.value(0).toString();
					item = document.createElement(item_tag);
					item.setAttribute(QStringLiteral("title"), title);
					level_element.appendChild(item);
					n = 0;
				}
				item.setAttribute(prefix + QString::number(++n), query.value(1).toString());
			}
			if (level_element.hasChildNodes()) list.appendChild(level_element);
		}
		if (list.hasChildNodes()) folio.appendChild(list);
	}
}

void writeElements(QDomDocument &document, QDomElement &folio, const QSqlDatabase &db, const QString &uuid)
{
	QSqlQuery query(db);
	query.setForwardOnly(true);
	query.prepare(QStringLiteral(
			"SELECT e.uuid, p.type, e.group_uuid, p.x, p.y, p.z, p.rotation, p.horizontal_mirror, p.vertical_mirror "
			"FROM element e JOIN element_placement p ON p.element_uuid = e.uuid "
			"WHERE e.diagram_uuid = :uuid ORDER BY e.rowid"));
	query.bindValue(QStringLiteral(":uuid"), uuid);
	if (!query.exec()) {
		qWarning() << "projectDataBase::toDocument elements:" << query.lastError();
		return;
	}
		//Each table read once for the folio's symbols, not once per symbol
		//(5 080 symbols of f2099: 2.9 s): each symbol's rows in the order
		//its own query gave
	const auto folio_rows = [&db, &uuid](const QString &sql) {
		QSqlQuery rows(db);
		rows.setForwardOnly(true);
		rows.prepare(sql);
		rows.bindValue(QStringLiteral(":uuid"), uuid);
		if (!rows.exec())
			qWarning() << "projectDataBase::toDocument elements:" << rows.lastError();
		return rows;
	};
	QHash<QString, Rows> values, texts;
	for (auto [table, rows] : {std::pair{QStringLiteral("element_value"), &values},
							   std::pair{QStringLiteral("element_text_value"), &texts}}) {
		QSqlQuery q = folio_rows(QStringLiteral("SELECT v.element_uuid, v.section, v.ord, v.name, v.value FROM %1 v "
												"JOIN element e ON e.uuid = v.element_uuid "
												"WHERE e.diagram_uuid = :uuid ORDER BY v.rowid").arg(table));
		while (q.next())
			(*rows)[q.value(0).toString()].append({q.value(1).toString(), q.value(2).toInt(),
												   q.value(3).toString(), q.value(4).toString()});
	}
	QHash<QString, QList<std::pair<QString, QString>>> sequence_parts;
	{
		QSqlQuery q = folio_rows(QStringLiteral("SELECT s.item_uuid, s.part, s.value FROM sequential_number s "
												"JOIN element e ON e.uuid = s.item_uuid "
												"WHERE e.diagram_uuid = :uuid ORDER BY s.item_uuid, s.part, s.ord"));
		while (q.next())
			sequence_parts[q.value(0).toString()].append({q.value(1).toString(), q.value(2).toString()});
	}
	QHash<QString, QList<std::pair<QString, QVariant>>> links_of;
	{
		QSqlQuery q = folio_rows(QStringLiteral("SELECT l.element_uuid, l.linked_uuid, l.group_index FROM link l "
												"JOIN element e ON e.uuid = l.element_uuid "
												"WHERE e.diagram_uuid = :uuid ORDER BY l.element_uuid, l.ord"));
		while (q.next())
			links_of[q.value(0).toString()].append({q.value(1).toString(), q.value(2)});
	}
	QHash<QString, DiagramContext> information_of;
	{
		QSqlQuery q = folio_rows(QStringLiteral("SELECT i.element_uuid, i.name, i.value, i.show FROM element_information i "
												"JOIN element e ON e.uuid = i.element_uuid "
												"WHERE e.diagram_uuid = :uuid ORDER BY i.element_uuid, i.ord"));
		while (q.next())
			information_of[q.value(0).toString()].addValue(q.value(1).toString(), q.value(2).toString(),
															q.value(3).toBool());
	}

	QDomElement list = document.createElement(QStringLiteral("elements"));
	while (query.next()) {
		const QString element_uuid = query.value(0).toString();
		QDomElement element = document.createElement(QStringLiteral("element"));
		element.setAttribute(QStringLiteral("type"), query.value(1).toString());
		element.setAttribute(QStringLiteral("uuid"), element_uuid);
		element.setAttribute(QStringLiteral("x"), QString::number(query.value(3).toDouble()));
		element.setAttribute(QStringLiteral("y"), QString::number(query.value(4).toDouble()));
		element.setAttribute(QStringLiteral("z"), QString::number(query.value(5).toDouble()));
		element.setAttribute(QStringLiteral("orientation"), QString::number(query.value(6).toInt()));
		const bool horizontal = query.value(7).toBool(), vertical = query.value(8).toBool();
		if (horizontal || vertical)
			element.setAttribute(QStringLiteral("mirror"), !vertical ? QStringLiteral("horizontal")
														: !horizontal ? QStringLiteral("vertical")
														: QStringLiteral("both"));
		if (!query.value(2).toString().isEmpty())
			element.setAttribute(QString::fromLatin1(ItemGroups::xml_attribute), query.value(2).toString());

			//Prefix, frozen label, movability, override properties, PLC data
		Rows own_values = values.value(element_uuid);
			//Its children's sections start with their own tag
		for (Row &row : own_values)
			if (row.section != QLatin1String("element")) row.section.prepend(QStringLiteral("element/"));
		rowsToXml(document, element, QStringLiteral("element"), own_values);

		const QDomElement sequence = sequenceFrom(sequence_parts.value(element_uuid)).toXml(document);
		if (sequence.hasChildNodes()) element.appendChild(sequence);
			//Each terminal is placed from the symbol's definition, and the
			//wires name theirs by uuid
		element.appendChild(document.createElement(QStringLiteral("terminals")));
		element.appendChild(document.createElement(QStringLiteral("inputs")));

		QDomElement links_uuids = document.createElement(QStringLiteral("links_uuids"));
		for (const auto &[linked, group_index] : links_of.value(element_uuid)) {
			QDomElement link = document.createElement(QStringLiteral("link_uuid"));
			link.setAttribute(QStringLiteral("uuid"), linked);
			if (!group_index.isNull() && group_index.toInt() >= 0)
				link.setAttribute(QStringLiteral("group_index"), group_index.toInt());
			links_uuids.appendChild(link);
		}
		if (links_uuids.hasChildNodes()) element.appendChild(links_uuids);

		const DiagramContext context = information_of.value(element_uuid);
		QDomElement infos = document.createElement(QStringLiteral("elementInformations"));
		context.toXml(infos, QStringLiteral("elementInformation"));
		if (infos.hasChildNodes()) element.appendChild(infos);

			//Texts and text groups (S3l)
		rowsToXml(document, element, QStringLiteral("element"),
				  [&]() {
					  Rows rows = texts.value(element_uuid);
						  //Their sections start with their own tag
					  for (Row &row : rows) row.section.prepend(QStringLiteral("element/"));
					  return rows;
				  }());
		list.appendChild(element);
	}
	folio.appendChild(list);
}

void writeConductors(QDomDocument &document, QDomElement &folio, const QSqlDatabase &db, const QString &uuid)
{
	QSqlQuery query(db);
	query.setForwardOnly(true);
	query.prepare(QStringLiteral(
			"SELECT c.uuid, c.terminal1_uuid, c.terminal1_element_uuid, c.terminal2_uuid, c.terminal2_element_uuid, "
			"l.x, l.y, l.freeze_label, l.text_x, l.text_y, l.text_rotation "
			"FROM conductor c JOIN conductor_layout l ON l.conductor_uuid = c.uuid "
			"WHERE c.diagram_uuid = :uuid ORDER BY c.rowid"));
	query.bindValue(QStringLiteral(":uuid"), uuid);
	if (!query.exec()) {
		qWarning() << "projectDataBase::toDocument conductors:" << query.lastError();
		return;
	}
	QDomElement list = document.createElement(QStringLiteral("conductors"));
	while (query.next()) {
		const QString wire = query.value(0).toString();
		QDomElement conductor = document.createElement(QStringLiteral("conductor"));
		conductor.setAttribute(QStringLiteral("uuid"), wire);
		conductor.setAttribute(QStringLiteral("x"), QString::number(query.value(5).toDouble()));
		conductor.setAttribute(QStringLiteral("y"), QString::number(query.value(6).toDouble()));
		conductor.setAttribute(QStringLiteral("element1"), query.value(2).toString());
		conductor.setAttribute(QStringLiteral("terminal1"), query.value(1).toString());
		conductor.setAttribute(QStringLiteral("element2"), query.value(4).toString());
		conductor.setAttribute(QStringLiteral("terminal2"), query.value(3).toString());
		conductor.setAttribute(QStringLiteral("freezeLabel"), query.value(7).toBool() ? QStringLiteral("true") : QStringLiteral("false"));

		QSqlQuery path(db);
		path.setForwardOnly(true);
		path.prepare(QStringLiteral("SELECT horizontal, length FROM conductor_path WHERE conductor_uuid = :uuid ORDER BY ord"));
		path.bindValue(QStringLiteral(":uuid"), wire);
		path.exec();
		while (path.next()) {
			QDomElement segment = document.createElement(QStringLiteral("segment"));
			segment.setAttribute(QStringLiteral("orientation"), path.value(0).toBool() ? QStringLiteral("horizontal") : QStringLiteral("vertical"));
			segment.setAttribute(QStringLiteral("length"), QStringLiteral("%1").arg(path.value(1).toDouble()));
			conductor.appendChild(segment);
		}
		conductor.appendChild(sequenceOf(db, wire).toXml(document));

		QSqlQuery properties(db);
		properties.setForwardOnly(true);
		properties.prepare(QStringLiteral("SELECT name, value FROM conductor_properties WHERE conductor_uuid = :uuid"));
		properties.bindValue(QStringLiteral(":uuid"), wire);
		properties.exec();
		while (properties.next())
			conductor.setAttribute(properties.value(0).toString(), properties.value(1).toString());
		if (!query.value(8).isNull()) {
			conductor.setAttribute(QStringLiteral("userx"), QString::number(query.value(8).toDouble()));
			conductor.setAttribute(QStringLiteral("usery"), QString::number(query.value(9).toDouble()));
		}
		if (!query.value(10).isNull())
			conductor.setAttribute(QStringLiteral("rotation"), QString::number(query.value(10).toDouble()));
		list.appendChild(conductor);
	}
	if (list.hasChildNodes()) folio.appendChild(list);
}

/// The symbols and wires a folio keeps as read (kept_item), after its own
void writeKept(QDomDocument &document, QDomElement &folio, const QSqlDatabase &db, const QString &uuid);
void writeDrawnItems(QDomDocument &document, QDomElement &folio, const QSqlDatabase &db, const QString &uuid);

/// A folio's items: its symbols, wires, kept items and drawn items
void writeFolioItems(QDomDocument &document, QDomElement &folio, const QSqlDatabase &db, const QString &uuid)
{
	writeElements(document, folio, db, uuid);
	writeConductors(document, folio, db, uuid);
	writeKept(document, folio, db, uuid);
	writeDrawnItems(document, folio, db, uuid);
}

void writeKept(QDomDocument &document, QDomElement &folio, const QSqlDatabase &db, const QString &uuid)
{
	QSqlQuery query(db);
	query.setForwardOnly(true);
	query.prepare(QStringLiteral("SELECT content FROM kept_item WHERE diagram_uuid = :uuid ORDER BY ord"));
	query.bindValue(QStringLiteral(":uuid"), uuid);
	query.exec();
	while (query.next()) {
		QDomDocument part;
		part.setContent(query.value(0).toString(), QDomDocument::ParseOption::PreserveSpacingOnlyNodes);
		const QDomElement item = part.documentElement();
		const QString list_tag = item.tagName() == QLatin1String("conductor")
				? QStringLiteral("conductors") : QStringLiteral("elements");
		QDomElement list = folio.firstChildElement(list_tag);
		if (list.isNull()) {
			list = document.createElement(list_tag);
				//The wires after the symbols
			if (list_tag == QLatin1String("conductors")) folio.insertAfter(list, folio.firstChildElement(QStringLiteral("elements")));
			else folio.appendChild(list);
		}
		list.appendChild(document.importNode(item, true));
	}
}

/// The shapes, free texts, pictures and tables of a folio, from drawing_item
void writeDrawnItems(QDomDocument &document, QDomElement &folio, const QSqlDatabase &db, const QString &uuid)
{
	static const QHash<QString, QString> lists = {
		{QStringLiteral("input"), QStringLiteral("inputs")}, {QStringLiteral("image"), QStringLiteral("images")},
		{QStringLiteral("shape"), QStringLiteral("shapes")}, {QStringLiteral("graphics_table"), QStringLiteral("tables")}};
	QHash<QString, QDomElement> parents;
	QSqlQuery query(db);
	query.setForwardOnly(true);
		//As the folio stacks them: by height, then first stored first
	query.prepare(QStringLiteral(
			"SELECT d.item_uuid, d.kind, d.section, d.ord, d.name, d.value FROM drawing_item d "
			"LEFT JOIN (SELECT item_uuid, CAST(value AS REAL) AS z FROM drawing_item "
			"           WHERE name = 'z' AND section = kind) h ON h.item_uuid = d.item_uuid "
			"WHERE d.folio_uuid = :uuid ORDER BY COALESCE(h.z, 0), d.seq, d.rowid"));
	query.bindValue(QStringLiteral(":uuid"), uuid);
	query.exec();
	QString current, kind;
	Rows rows;
	auto flush = [&]() {
		if (current.isEmpty()) return;
		QDomElement item = document.createElement(kind);
		Rows own;
		QString blob;
		for (const Row &row : std::as_const(rows))
			if (row.name == QLatin1String("#blob")) blob = row.value;
			else own.append(row);
		if (!blob.isEmpty()) {
			QSqlQuery bytes(db);
			bytes.setForwardOnly(true);
			bytes.prepare(QStringLiteral("SELECT bytes FROM blob WHERE hash = :hash"));
			bytes.bindValue(QStringLiteral(":hash"), blob);
			if (bytes.exec() && bytes.next())
				item.appendChild(document.createTextNode(QString::fromLatin1(bytes.value(0).toByteArray().toBase64())));
		}
		rowsToXml(document, item, kind, own);
		const QString list_tag = lists.value(kind, kind + QLatin1Char('s'));
		if (!parents.contains(list_tag)) {
			parents.insert(list_tag, document.createElement(list_tag));
		}
		parents[list_tag].appendChild(item);
		rows.clear();
	};
	while (query.next()) {
		if (query.value(0).toString() != current) {
			flush();
			current = query.value(0).toString();
			kind = query.value(1).toString();
		}
		rows.append({query.value(2).toString(), query.value(3).toInt(),
					 query.value(4).toString(), query.value(5).toString()});
	}
	flush();
	for (const QString &tag : {QStringLiteral("inputs"), QStringLiteral("images"),
							   QStringLiteral("shapes"), QStringLiteral("tables")})
		if (parents.contains(tag)) folio.appendChild(parents.take(tag));
	for (const QDomElement &other : std::as_const(parents)) folio.appendChild(other);
}

/// The project's own symbols and their categories (S3m), as a <collection>
QDomElement collectionOf(QDomDocument &document, const QSqlDatabase &db)
{
	QDomElement collection = document.createElement(QStringLiteral("collection"));
	QHash<QString, QDomElement> categories;
	categories.insert(QString(), collection);
	auto parentOf = [&](const QString &path) {
		return categories.value(path.left(qMax(path.lastIndexOf(QLatin1Char('/')), 0)), collection);
	};
		//In the collection's order, symbols and categories together; a
		//category's names where they sat among its children (names_ord)
	QHash<QString, std::pair<int, QDomElement>> pending_names;
	auto place = [&](const QString &path, int ord, const QDomElement &item) {
		const QString parent = path.left(qMax(path.lastIndexOf(QLatin1Char('/')), 0));
		QDomElement into = parentOf(path);
		const auto names = pending_names.constFind(parent);
		if (names != pending_names.constEnd() && names->first < ord) {
			into.appendChild(names->second);
			pending_names.erase(names);
		}
		into.appendChild(item);
	};
	struct Entry {
		QString path;
		QString content;
		int ord;
		bool is_category;
		QVariant names_ord;
		QDomDocument parsed;
	};
	QList<Entry> entries;
	QSqlQuery query(db);
	query.setForwardOnly(true);
	query.exec(QStringLiteral("SELECT path, content, ord, is_category, names_ord FROM ("
							  "SELECT path, content, ord, 1 AS is_category, names_ord FROM symbol_category UNION ALL "
							  "SELECT path, content, ord, 0, NULL FROM symbol_def) ORDER BY ord"));
	while (query.next())
		entries.append({query.value(0).toString(), query.value(1).toString(), query.value(2).toInt(),
						query.value(3).toBool(), query.value(4), {}});
		//Each symbol parsed on its own document, all at once: the parsing
		//was 2.3 s of 3.8 s opening f2099's 404 symbols one after another
	QtConcurrent::blockingMap(entries, [](Entry &entry) {
		entry.parsed.setContent(entry.content, QDomDocument::ParseOption::PreserveSpacingOnlyNodes);
	});
	for (Entry &entry : entries) {
		QDomElement item = document.importNode(entry.parsed.documentElement(), true).toElement();
		entry.parsed = QDomDocument();
		if (entry.is_category) {
			const QDomElement names = item.firstChildElement(QStringLiteral("names"));
			if (!names.isNull() && !entry.names_ord.isNull()) {
				item.removeChild(names);
				pending_names.insert(entry.path, {entry.names_ord.toInt(), names});
			}
			categories.insert(entry.path, item);
		}
		place(entry.path, entry.ord, item);
	}
	for (auto it = pending_names.constBegin() ; it != pending_names.constEnd() ; ++it)
		categories.value(it.key()).appendChild(it.value().second);
	return collection;
}

} // namespace

/**
	@brief projectDataBase::toDocument
	@return the project the database @p db holds, as a save writes it,
	read from its tables alone
*/
QDomDocument projectDataBase::toDocument(const QSqlDatabase &db, bool deferred_folios)
{
	QDomDocument document;
	QDomElement project = document.createElement(QStringLiteral("project"));
	document.appendChild(project);
	QetVersion::toXmlAttribute(project);

	QSqlQuery query(db);
	query.setForwardOnly(true);
	query.exec(QStringLiteral("SELECT section, ord, name, value FROM project_setting ORDER BY rowid"));
	const Rows settings = selectRows(query);
	Rows newdiagrams;
	QMap<QPair<QString, int>, Rows> sections;   //top-level tag and ord -> rows
	QList<QPair<QString, int>> order;
	for (const Row &row : settings) {
		if (row.section == QLatin1String("project")) {
			project.setAttribute(row.name, row.value);
			continue;
		}
		const QString top = row.section.section(QLatin1Char('/'), 0, 0);
		if (top == QLatin1String("newdiagrams")) {
			newdiagrams.append(row);
			continue;
		}
		const QPair<QString, int> key{top, row.section == top ? row.ord : 0};
		if (!sections.contains(key)) order.append(key);
		sections[key].append(row);
	}

	query.exec(QStringLiteral("SELECT content FROM titleblock_template ORDER BY name"));
	QDomElement templates = document.createElement(QStringLiteral("titleblocktemplates"));
	while (query.next()) {
		QDomDocument part;
		part.setContent(query.value(0).toString(), QDomDocument::ParseOption::PreserveSpacingOnlyNodes);
		templates.appendChild(document.importNode(part.documentElement(), true));
	}
	if (templates.hasChildNodes()) project.appendChild(templates);

		//The project's properties, wire hops, upright texts, wiring rules
		//and the default strip layout, as they were written
	QList<QDomElement> after_folios;
	for (const auto &key : std::as_const(order)) {
		QDomElement element = document.createElement(key.first);
		rowsToXml(document, element, key.first, sections.value(key));
		if (key.first == QLatin1String("terminal_strip_layout")) after_folios.append(element);
		else project.appendChild(element);
	}
	if (!sections.contains({QStringLiteral("properties"), 0}))
		project.appendChild(document.createElement(QStringLiteral("properties")));

	QDomElement new_diagrams = document.createElement(QStringLiteral("newdiagrams"));
	rowsToXml(document, new_diagrams, QStringLiteral("newdiagrams"), newdiagrams);
	QETProject::writeNumberingXml(numberingOf(db), new_diagrams);
	project.appendChild(new_diagrams);

	query.exec(QStringLiteral("SELECT uuid FROM diagram ORDER BY pos"));
	int order_num = 1;
	while (query.next()) {
		const QString uuid = query.value(0).toString();
		QDomElement folio = document.createElement(QStringLiteral("diagram"));
		folio.setAttribute(QStringLiteral("uuid"), uuid);
		titleBlockOf(db, uuid).toXml(folio);

		QSqlQuery border(db);
		border.setForwardOnly(true);
		border.prepare(QStringLiteral("SELECT columns_count, columns_width, display_columns, "
									  "rows_count, rows_height, display_rows FROM folio_border WHERE diagram_uuid = :uuid"));
		border.bindValue(QStringLiteral(":uuid"), uuid);
		if (border.exec() && border.next()) {
			folio.setAttribute(QStringLiteral("cols"), border.value(0).toInt());
			folio.setAttribute(QStringLiteral("colsize"), QStringLiteral("%1").arg(border.value(1).toDouble()));
			folio.setAttribute(QStringLiteral("displaycols"), border.value(2).toBool() ? QStringLiteral("true") : QStringLiteral("false"));
			folio.setAttribute(QStringLiteral("rows"), border.value(3).toInt());
			folio.setAttribute(QStringLiteral("rowsize"), QStringLiteral("%1").arg(border.value(4).toDouble()));
			folio.setAttribute(QStringLiteral("displayrows"), border.value(5).toBool() ? QStringLiteral("true") : QStringLiteral("false"));
		}
			//Default wire, freeze and numbering settings (S3n)
		rowsToXml(document, folio, QStringLiteral("diagram"),
				  keyRows(db, QStringLiteral("folio_value"), QStringLiteral("diagram_uuid"), uuid));
		writeFolioCounters(document, folio, db, uuid);
			//A folio not built at the open gets its items when it is
			//(writeFolioContent(), S4x X3)
		if (deferred_folios)
			folio.setAttribute(QString::fromLatin1(deferred_content), QStringLiteral("1"));
		else
			writeFolioItems(document, folio, db, uuid);
		folio.setAttribute(QStringLiteral("order"), order_num++);
		project.appendChild(folio);
	}

	for (QDomElement &element : after_folios) project.appendChild(element);
	QSqlQuery strips(db);
	strips.setForwardOnly(true);
	strips.exec(QStringLiteral("SELECT strip_uuid, section, ord, name, value FROM terminal_strip_value ORDER BY rowid"));
	QDomElement strip_list = document.createElement(QStringLiteral("terminal_strips"));
	keyedRowsToXml(document, strip_list, strips, QStringLiteral("terminal_strip"));
	if (strip_list.hasChildNodes()) project.appendChild(strip_list);

	QSqlQuery cables(db);
	cables.setForwardOnly(true);
	cables.exec(QStringLiteral("SELECT cable_uuid, section, ord, name, value FROM cable_value ORDER BY rowid"));
	QDomElement cable_list = document.createElement(QStringLiteral("cables"));
	keyedRowsToXml(document, cable_list, cables, QStringLiteral("cable"));
	if (cable_list.hasChildNodes()) project.appendChild(cable_list);

	project.appendChild(collectionOf(document, db));
	return document;
}

/**
	@brief projectDataBase::writeDocument
	Write to @p project_file the project the database file
	@p database_file holds (toDocument()), opened through a connection of
	its own.
	@return true on success; else false, and why in @p error
*/
/**
	@brief projectDataBase::writeFolioContent
	Write into @p folio, a folio element toDocument() left without its items
	(deferred_content), its symbols, wires, kept items and drawn items, as
	this database holds them now
*/
void projectDataBase::writeFolioContent(QDomElement &folio) const
{
	if (!folio.hasAttribute(QString::fromLatin1(deferred_content))) return;
	folio.removeAttribute(QString::fromLatin1(deferred_content));
	QDomDocument document = folio.ownerDocument();
	writeFolioItems(document, folio, m_data_base, folio.attribute(QStringLiteral("uuid")));
}

bool projectDataBase::writeDocument(const QString &database_file, const QString &project_file, QString *error)
{
	QDomDocument document;
	return readDocument(database_file, &document, error)
			&& QET::writeXmlFile(document, project_file, error);
}

/**
	@brief projectDataBase::readDocument
	@p document becomes the project the database file @p database_file
	holds (toDocument()), read through a connection of its own.
	@return true on success; else false, and why in @p error
*/
bool projectDataBase::readDocument(const QString &database_file, QDomDocument *document, QString *error,
								   bool deferred_folios)
{
	const QString connection = QStringLiteral("qet_project_writer_") + QUuid::createUuid().toString();
	bool ok = false;
	{
		QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
		db.setDatabaseName(database_file);
		db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
		if (!db.open()) {
			if (error) *error = db.lastError().text();
		} else {
			*document = toDocument(db, deferred_folios);
			ok = true;
		}
		db.close();
	}
	QSqlDatabase::removeDatabase(connection);
	return ok;
}

/**
	@brief projectDataBase::writeProjectFromDatabase
	Bring every table up to date, copy the database to a file of its own,
	and write @p project_file from that copy alone (writeDocument()): what
	the database would give back of this project.
*/
bool projectDataBase::writeProjectFromDatabase(const QString &project_file, QString *error)
{
	QTemporaryDir dir;
	if (!dir.isValid()) {
		if (error) *error = QStringLiteral("no temporary folder");
		return false;
	}
	const QString copy = dir.filePath(QStringLiteral("project.sqlite"));
	return copyTo(copy, error) && writeDocument(copy, project_file, error);
}

/**
	@brief projectDataBase::copyTo
	Bring every table up to date and copy the database to the new file
	@p file (VACUUM INTO), as it then holds the whole project.
*/
bool projectDataBase::copyTo(const QString &file, QString *error)
{
	QElapsedTimer timer;
	timer.start();
	updateDB();
		//The rebuild above leaves some queues for the next read
	flushDrawingItems();
	flushLinks();
	flushElementPositions();
	flushElementTexts();
	flushCollections();
	flushFolioValues();
	flushCables();
	flushConductorProperties();
	const qint64 update_ms = timer.elapsed();

		//VACUUM INTO cannot run inside the working copy's transaction
	endTransaction();
	QSqlQuery query(m_data_base);
	query.setForwardOnly(true);
	const bool copied = query.exec(QStringLiteral("VACUUM INTO '%1'").arg(QString(file).replace(QLatin1Char('\''), QStringLiteral("''"))));
	beginTransaction();
	if (qEnvironmentVariableIntValue("QET_QETDB_SAVE_TIMES") == 1)
		qInfo().noquote() << "projectDataBase::copyTo (ms): update and flush" << update_ms
						  << ", VACUUM INTO" << timer.elapsed() - update_ms;
	if (!copied && error) *error = query.lastError().text();
	return copied;
}
