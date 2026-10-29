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

#include "../properties/xrefproperties.h"
#include "../titleblockproperties.h"

#include <QDomDocument>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include <algorithm>

/*
	A project opened from a .qetdb starts from the file's own tables
	(QET-SQL-FORMAT S4x, stage X2, QETDB-S4X-SCOPE.md): they are copied into
	this database, and the caches in front of them loaded from them, instead
	of both being filled again from the document read out of the file
	(prefillFromDocument(), populateFromDocument()). stateDump() is what
	shows the two ways give the same database.
*/

namespace {

using Rows = QList<projectDataBase::ProjectSetting>;

	//The (section, ord, name, value) rows of @p table, by their @p key, each
	//key's in the order they were written
QHash<QUuid, Rows> keyedRows(const QSqlDatabase &db, const QString &table, const QString &key)
{
	QHash<QUuid, Rows> rows;
	QSqlQuery query(db);
	query.setForwardOnly(true);
	if (!query.exec(QStringLiteral("SELECT %1, section, ord, name, value FROM %2 ORDER BY rowid").arg(key, table)))
		return rows;
	while (query.next())
		rows[QUuid(query.value(0).toString())].append({query.value(1).toString(), query.value(2).toInt(),
													   query.value(3).toString(), query.value(4).toString()});
	return rows;
}

	//The (name, value, show) rows of @p table as a context each, by their
	//@p key, in ord order
QHash<QUuid, DiagramContext> contexts(const QSqlDatabase &db, const QString &table, const QString &key)
{
	QHash<QUuid, DiagramContext> result;
	QSqlQuery query(db);
	query.setForwardOnly(true);
	if (!query.exec(QStringLiteral("SELECT %1, name, value, show FROM %2 ORDER BY %1, ord").arg(key, table)))
		return result;
	while (query.next())
		result[QUuid(query.value(0).toString())].addValue(query.value(1).toString(), query.value(2).toString(),
														  query.value(3).toBool());
	return result;
}

QString number(qreal value) { return QString::number(value, 'g', 17); }

QString rowsText(const Rows &rows)
{
	QStringList lines;
	for (const auto &row : rows)
		lines << QStringLiteral("%1|%2|%3|%4").arg(row.section).arg(row.ord).arg(row.name, row.value);
	return lines.join(QLatin1Char(';'));
}

QString contextText(const DiagramContext &context)
{
	QStringList keys = context.keys();
	keys.sort();
	QStringList parts;
	for (const QString &key : std::as_const(keys))
		parts << QStringLiteral("%1=%2/%3").arg(key, context.value(key).toString())
				 .arg(context.keyMustShow(key) ? 1 : 0);
	return parts.join(QLatin1Char(';'));
}

	//@p cache's entries as sorted lines, each key then its value as text
template <typename Map, typename Text>
void dumpCache(QStringList &out, const QString &name, const Map &cache, Text text)
{
	QStringList lines;
	for (auto it = cache.constBegin() ; it != cache.constEnd() ; ++it)
		lines << QStringLiteral("%1 %2").arg(it.key().toString(), text(it.value()));
	lines.sort();
	out << QStringLiteral("== cache %1 (%2)").arg(name).arg(lines.size());
	out << lines;
}

} // namespace

/**
	@brief projectDataBase::adoptFile
	Make this database the one the checked .qetdb copy @p file holds: each
	table of this database found in the file with the same columns takes
	the file's rows, then the caches in front of them are loaded from them.
	@return false, the database as it was, if the file's tables are not this
	database's (a .qetdb from another schema): the project is then filled
	from its document, as before
*/
bool projectDataBase::adoptFile(const QString &file, QString *error)
{
		//ATTACH cannot run inside the working copy's transaction
	endTransaction();
	QSqlQuery query(m_data_base);
	bool ok = query.exec(QStringLiteral("ATTACH DATABASE '%1' AS adopted")
						 .arg(QString(file).replace(QLatin1Char('\''), QStringLiteral("''"))));
	QStringList tables;
	if (ok && query.exec(QStringLiteral("SELECT name FROM main.sqlite_master WHERE type = 'table' "
										"AND name NOT LIKE 'sqlite\\_%' ESCAPE '\\' AND name <> 'working_copy'")))
		while (query.next()) tables << query.value(0).toString();
	const auto columns = [&query](const QString &schema, const QString &table) {
		QStringList names;
		if (query.exec(QStringLiteral("PRAGMA %1.table_info(\"%2\")").arg(schema, table)))
			while (query.next()) names << query.value(1).toString();
		return names;
	};
	for (const QString &table : std::as_const(tables)) {
		const QStringList mine = columns(QStringLiteral("main"), table);
		if (mine.isEmpty() || mine != columns(QStringLiteral("adopted"), table)) {
			if (error) *error = QStringLiteral("table %1 differs").arg(table);
			ok = false;
			break;
		}
	}
	if (ok) ok = m_data_base.transaction();
	for (const QString &table : std::as_const(tables)) {
		if (!ok) break;
			//No ORDER BY: a table is read in rowid order anyway, and SQLite
			//copies a table into an empty one of the same columns and
			//indexes page by page only without one (its transfer
			//optimization; a file keeps its indexes): half the time on f2099
		ok = query.exec(QStringLiteral("DELETE FROM main.\"%1\"").arg(table))
				&& query.exec(QStringLiteral("INSERT INTO main.\"%1\" SELECT * FROM adopted.\"%1\"").arg(table));
		if (!ok && error) *error = query.lastError().text();
	}
	if (ok) ok = m_data_base.commit();
	else m_data_base.rollback();
	query.exec(QStringLiteral("DETACH DATABASE adopted"));
	beginTransaction();
	if (!ok) return false;

	loadCaches();
	m_adopted = true;
	m_rows_from_document = true;
	m_holds_links = true;
	m_content_changed = false;
	m_rebuild_all = false;
	m_changed_folios.clear();
	m_rows_in_folio_order = false;
	m_folio_places_changed = false;
	m_folios_with_unjoined_wires.clear();
	return true;
}

/**
	@brief projectDataBase::readElementTexts
	Read the stored texts of the symbol @p element into the cache, if the
	file's were not read at open (loadCaches()) and these not yet: what
	storeElementTexts() compares with, as if all had been read.
*/
void projectDataBase::readElementTexts(const QUuid &element) const
{
	if (!m_element_texts_lazy || element.isNull() || m_element_texts_read.contains(element)) return;
	m_element_texts_read.insert(element);
	if (m_element_texts.contains(element)) return;
	QSqlQuery query(m_data_base);
	query.setForwardOnly(true);
	query.prepare(QStringLiteral("SELECT section, ord, name, value FROM element_text_value "
								 "WHERE element_uuid = :uuid ORDER BY rowid"));
	query.bindValue(QStringLiteral(":uuid"), element.toString());
	if (!query.exec()) return;
	QList<ProjectSetting> rows;
	while (query.next())
		rows.append({query.value(0).toString(), query.value(1).toInt(),
					 query.value(2).toString(), query.value(3).toString()});
	if (!rows.isEmpty()) m_element_texts.insert(element, rows);
}

/**
	@brief projectDataBase::readAllElementTexts
	Read every symbol's stored texts not read yet (readElementTexts()):
	the cache then holds what loading the whole table would have.
*/
void projectDataBase::readAllElementTexts() const
{
	if (!m_element_texts_lazy) return;
	const QHash<QUuid, Rows> stored = keyedRows(m_data_base, QStringLiteral("element_text_value"),
												QStringLiteral("element_uuid"));
	for (auto it = stored.constBegin() ; it != stored.constEnd() ; ++it)
		if (!m_element_texts_read.contains(it.key()) && !m_element_texts.contains(it.key()))
			m_element_texts.insert(it.key(), it.value());
	m_element_texts_lazy = false;
}

/**
	@brief projectDataBase::loadCaches
	Fill the caches in front of the tables from the tables, as the stores
	that wrote them would hold them (adoptFile())
*/
void projectDataBase::loadCaches()
{
	m_element_information = contexts(m_data_base, QStringLiteral("element_information"), QStringLiteral("element_uuid"));
	m_element_kind_information = contexts(m_data_base, QStringLiteral("element_kind_information"), QStringLiteral("element_uuid"));
		//Most of the rows (102,000 of f2099's 120,000): each symbol's are
		//read when first asked for, readElementTexts()
	m_element_texts.clear();
	m_element_texts_read.clear();
	m_element_texts_lazy = true;
	m_element_values = keyedRows(m_data_base, QStringLiteral("element_value"), QStringLiteral("element_uuid"));
	m_folio_values = keyedRows(m_data_base, QStringLiteral("folio_value"), QStringLiteral("diagram_uuid"));
	m_cable_values = keyedRows(m_data_base, QStringLiteral("cable_value"), QStringLiteral("cable_uuid"));
	m_terminal_strip_rows = keyedRows(m_data_base, QStringLiteral("terminal_strip_value"), QStringLiteral("strip_uuid"));

	QSqlQuery query(m_data_base);
	query.setForwardOnly(true);
		//The project's own symbols, categories and templates, and its
		//settings: else the first flush after the open writes them all
		//again (0.8 s of saving f2099)
	m_symbol_defs.clear();
	m_symbol_categories.clear();
	m_titleblock_templates.clear();
	m_collection_order.clear();
	for (auto [table, cache] : {std::pair{QStringLiteral("symbol_def"), &m_symbol_defs},
								std::pair{QStringLiteral("symbol_category"), &m_symbol_categories}})
		if (query.exec(QStringLiteral("SELECT path, content, ord FROM %1").arg(table)))
			while (query.next()) {
				cache->insert(query.value(0).toString(), query.value(1).toString());
				if (!query.value(2).isNull()) m_collection_order.insert(query.value(0).toString(), query.value(2).toInt());
			}
	if (query.exec(QStringLiteral("SELECT name, content FROM titleblock_template")))
		while (query.next()) m_titleblock_templates.insert(query.value(0).toString(), query.value(1).toString());
	m_project_settings.clear();
	if (query.exec(QStringLiteral("SELECT section, ord, name, value FROM project_setting ORDER BY rowid")))
		while (query.next())
			m_project_settings.append({query.value(0).toString(), query.value(1).toInt(),
									   query.value(2).toString(), query.value(3).toString()});
	m_kept_items.clear();
	if (query.exec(QStringLiteral("SELECT diagram_uuid, content FROM kept_item ORDER BY diagram_uuid, ord")))
		while (query.next()) m_kept_items[QUuid(query.value(0).toString())].append(query.value(1).toString());

	m_element_placements.clear();
	if (query.exec(QStringLiteral("SELECT element_uuid, type, x, y, rotation, horizontal_mirror, vertical_mirror, z "
								  "FROM element_placement")))
		while (query.next()) {
			ElementPlacement placement;
			placement.type = query.value(1).toString();
			placement.x = query.value(2).toDouble();
			placement.y = query.value(3).toDouble();
			placement.rotation = query.value(4).toDouble();
			placement.horizontal_mirror = query.value(5).toBool();
			placement.vertical_mirror = query.value(6).toBool();
			placement.z = query.value(7).toDouble();
			m_element_placements.insert(QUuid(query.value(0).toString()), placement);
		}

	m_sequences.clear();
	if (query.exec(QStringLiteral("SELECT item_uuid, part, value FROM sequential_number ORDER BY item_uuid, part, ord")))
		while (query.next()) {
			autonum::sequentialNumbers &numbers = m_sequences[QUuid(query.value(0).toString())];
			const QString part = query.value(1).toString();
			QStringList *list = part == QLatin1String("unit") ? &numbers.unit
							  : part == QLatin1String("wrap") ? &numbers.wrap
							  : part == QLatin1String("unit_folio") ? &numbers.unit_folio
							  : part == QLatin1String("ten") ? &numbers.ten
							  : part == QLatin1String("ten_folio") ? &numbers.ten_folio
							  : part == QLatin1String("hundred") ? &numbers.hundred
							  : part == QLatin1String("hundred_folio") ? &numbers.hundred_folio
							  : part == QLatin1String("alpha") ? &numbers.alpha : nullptr;
			if (list) list->append(query.value(2).toString());
		}

	m_folio_titleblocks.clear();
	if (query.exec(QStringLiteral("SELECT diagram_uuid, name, value, show FROM folio_titleblock ORDER BY diagram_uuid, ord")))
		while (query.next()) {
			TitleBlockProperties &properties = m_folio_titleblocks[QUuid(query.value(0).toString())];
			properties.collection = QET::QetCollection::Embedded;
			const QString name = query.value(1).toString();
			const QString value = query.value(2).toString();
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
				properties.context.addValue(name.mid(7), value, query.value(3).toBool());
		}

	m_folio_borders.clear();
	if (query.exec(QStringLiteral("SELECT diagram_uuid, columns_count, columns_width, columns_header_height, "
								  "display_columns, rows_count, rows_height, rows_header_width, display_rows "
								  "FROM folio_border")))
		while (query.next()) {
			BorderProperties border;
			border.columns_count = query.value(1).toInt();
			border.columns_width = query.value(2).toDouble();
			border.columns_header_height = query.value(3).toDouble();
			border.display_columns = query.value(4).toBool();
			border.rows_count = query.value(5).toInt();
			border.rows_height = query.value(6).toDouble();
			border.rows_header_width = query.value(7).toDouble();
			border.display_rows = query.value(8).toBool();
			m_folio_borders.insert(QUuid(query.value(0).toString()), border);
		}

	m_folio_numbers.clear();
	if (query.exec(QStringLiteral("SELECT diagram_uuid, folio_index, folio_total FROM folio_number")))
		while (query.next())
			m_folio_numbers.insert(QUuid(query.value(0).toString()), {query.value(1).toInt(), query.value(2).toInt()});

	m_conductor_attributes.clear();
	if (query.exec(QStringLiteral("SELECT conductor_uuid, name, value FROM conductor_properties")))
		while (query.next())
			m_conductor_attributes[QUuid(query.value(0).toString())].insert(query.value(1).toString(),
																			query.value(2).toString());
	m_conductor_properties.clear();
	{
		QDomDocument document;
		for (auto it = m_conductor_attributes.constBegin() ; it != m_conductor_attributes.constEnd() ; ++it) {
			QDomElement conductor = document.createElement(QStringLiteral("conductor"));
			for (auto a = it.value().constBegin() ; a != it.value().constEnd() ; ++a)
				conductor.setAttribute(a.key(), a.value());
			ConductorProperties properties;
			properties.fromXml(conductor);
			m_conductor_properties.insert(it.key(), properties);
		}
	}
	m_dirty_conductor_properties.clear();

	m_conductor_layouts.clear();
	if (query.exec(QStringLiteral("SELECT conductor_uuid, x, y, freeze_label, text_x, text_y, text_rotation "
								  "FROM conductor_layout")))
		while (query.next()) {
			ConductorLayout layout;
			layout.x = query.value(1).toDouble();
			layout.y = query.value(2).toDouble();
			layout.freeze_label = query.value(3).toBool();
			if (!query.value(4).isNull() && !query.value(5).isNull())
				layout.text_pos = QPointF(query.value(4).toDouble(), query.value(5).toDouble());
			if (!query.value(6).isNull())
				layout.text_rotation = query.value(6).toDouble();
			m_conductor_layouts.insert(QUuid(query.value(0).toString()), layout);
		}

	m_conductor_paths.clear();
	if (query.exec(QStringLiteral("SELECT conductor_uuid, horizontal, length FROM conductor_path ORDER BY conductor_uuid, ord")))
		while (query.next())
			m_conductor_paths[QUuid(query.value(0).toString())].append({query.value(1).toBool(), query.value(2).toDouble()});

	m_drawn_items.clear();
	m_blob_refs.clear();
	m_drawn_seq = 0;
	if (query.exec(QStringLiteral("SELECT item_uuid, folio_uuid, kind, section, ord, name, value, seq "
								  "FROM drawing_item ORDER BY rowid")))
		while (query.next()) {
			DrawnItem &drawn = m_drawn_items[QUuid(query.value(0).toString())];
			drawn.folio = QUuid(query.value(1).toString());
			drawn.kind = query.value(2).toString();
			drawn.seq = query.value(7).toInt();
			drawn.rows.append({query.value(3).toString(), query.value(4).toInt(),
							   query.value(5).toString(), query.value(6).toString()});
			m_drawn_seq = std::max(m_drawn_seq, drawn.seq);
		}
	recountBlobs();

		//What the fill from a document stores for every placed symbol and
		//wire, if only empty
	QSet<QUuid> conductors;
	if (query.exec(QStringLiteral("SELECT uuid FROM conductor")))
		while (query.next()) conductors.insert(QUuid(query.value(0).toString()));
	QSet<QUuid> elements;
	if (query.exec(QStringLiteral("SELECT uuid FROM element")))
		while (query.next()) elements.insert(QUuid(query.value(0).toString()));
	for (auto it = m_element_placements.constBegin() ; it != m_element_placements.constEnd() ; ++it) {
		if (!m_element_information.contains(it.key())) m_element_information.insert(it.key(), DiagramContext());
		if (!m_sequences.contains(it.key())) m_sequences.insert(it.key(), autonum::sequentialNumbers());
	}
	for (const QUuid &element : std::as_const(elements))
		if (!m_element_kind_information.contains(element)) m_element_kind_information.insert(element, DiagramContext());
	for (const QUuid &conductor : std::as_const(conductors))
		if (!m_sequences.contains(conductor)) m_sequences.insert(conductor, autonum::sequentialNumbers());
}

/**
	@brief projectDataBase::foliosWithCableWires
	@return the folios with a wire that is a core of a cable, from the
	wires' rows: what QETProject::cableLabelFolios() otherwise reads from
	every folio not built, which for a .qetdb opened from its tables would
	write out each one's items (S4x X3)
*/
QSet<QUuid> projectDataBase::foliosWithCableWires() const
{
	QSet<QUuid> folios;
	QSqlQuery query(m_data_base);
	query.setForwardOnly(true);
	if (query.exec(QStringLiteral("SELECT DISTINCT c.diagram_uuid FROM conductor c "
								  "JOIN conductor_properties p ON p.conductor_uuid = c.uuid "
								  "WHERE p.name = 'cable_uuid' AND p.value <> ''")))
		while (query.next()) folios.insert(QUuid(query.value(0).toString()));
	return folios;
}

/**
	@brief projectDataBase::stateDump
	Every table, its rows sorted, and every cache loadCaches() fills, its
	entries sorted, as text: two ways of opening a project give the same
	database when they give the same dump (QET_DUMP_DB_STATE).
*/
QString projectDataBase::stateDump() const
{
		//The cache as loading the whole table would have left it
	readAllElementTexts();
	QStringList out;
	QSqlQuery query(m_data_base);
	query.setForwardOnly(true);
	QStringList tables;
	if (query.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type = 'table' "
								  "AND name NOT LIKE 'sqlite\\_%' ESCAPE '\\' AND name <> 'working_copy' ORDER BY name")))
		while (query.next()) tables << query.value(0).toString();
	for (const QString &table : std::as_const(tables)) {
		QStringList lines;
		if (query.exec(QStringLiteral("SELECT * FROM \"%1\"").arg(table))) {
			const int count = query.record().count();
			while (query.next()) {
				QStringList cells;
				for (int i = 0 ; i < count ; ++i) {
					const QVariant v = query.value(i);
					cells << (v.isNull() ? QStringLiteral("NULL")
							  : table == QLatin1String("drawing_item") && query.record().fieldName(i) == QLatin1String("seq")
							  ? QStringLiteral("seq") : v.toString());
				}
				lines << cells.join(QLatin1Char('|'));
			}
		}
		lines.sort();
		out << QStringLiteral("== table %1 (%2)").arg(table).arg(lines.size());
		out << lines;
	}

	dumpCache(out, QStringLiteral("element_information"), m_element_information, contextText);
	dumpCache(out, QStringLiteral("element_kind_information"), m_element_kind_information, contextText);
	dumpCache(out, QStringLiteral("element_texts"), m_element_texts, rowsText);
	dumpCache(out, QStringLiteral("element_values"), m_element_values, rowsText);
	dumpCache(out, QStringLiteral("folio_values"), m_folio_values, rowsText);
	dumpCache(out, QStringLiteral("cable_values"), m_cable_values, rowsText);
	dumpCache(out, QStringLiteral("element_placements"), m_element_placements, [](const ElementPlacement &p) {
		return QStringLiteral("%1 %2 %3 %4 %5 %6 %7").arg(p.type, number(p.x), number(p.y), number(p.rotation))
				.arg(p.horizontal_mirror).arg(p.vertical_mirror).arg(number(p.z));
	});
	dumpCache(out, QStringLiteral("sequences"), m_sequences, [](const autonum::sequentialNumbers &n) {
		return QStringList{n.unit.join(','), n.wrap.join(','), n.unit_folio.join(','), n.ten.join(','),
						   n.ten_folio.join(','), n.hundred.join(','), n.hundred_folio.join(','),
						   n.alpha.join(',')}.join(QLatin1Char('|'));
	});
	dumpCache(out, QStringLiteral("folio_titleblocks"), m_folio_titleblocks, [](const TitleBlockProperties &p) {
		QDomDocument document;
		QDomElement e = document.createElement(QStringLiteral("t"));
		p.toXml(e);
		document.appendChild(e);
		return QStringLiteral("%1 version=%2 collection=%3").arg(document.toString(-1), p.version)
				.arg(int(p.collection));
	});
	dumpCache(out, QStringLiteral("folio_borders"), m_folio_borders, [](const BorderProperties &b) {
		return QStringLiteral("%1 %2 %3 %4 %5 %6 %7 %8").arg(b.columns_count).arg(number(b.columns_width))
				.arg(number(b.columns_header_height)).arg(b.display_columns).arg(b.rows_count)
				.arg(number(b.rows_height)).arg(number(b.rows_header_width)).arg(b.display_rows);
	});
	dumpCache(out, QStringLiteral("folio_numbers"), m_folio_numbers, [](const QPair<int, int> &n) {
		return QStringLiteral("%1/%2").arg(n.first).arg(n.second);
	});
	dumpCache(out, QStringLiteral("conductor_properties"), m_conductor_properties, [](const ConductorProperties &p) {
		QStringList attributes;
		for (const auto &a : p.attributes()) attributes << a.first + QLatin1Char('=') + a.second;
		attributes.sort();
		return attributes.join(QLatin1Char(';'));
	});
	dumpCache(out, QStringLiteral("conductor_attributes"), m_conductor_attributes, [](const QHash<QString, QString> &h) {
		QStringList attributes;
		for (auto it = h.constBegin() ; it != h.constEnd() ; ++it) attributes << it.key() + QLatin1Char('=') + it.value();
		attributes.sort();
		return attributes.join(QLatin1Char(';'));
	});
	dumpCache(out, QStringLiteral("conductor_layouts"), m_conductor_layouts, [](const ConductorLayout &l) {
		return QStringLiteral("%1 %2 %3 %4 %5").arg(number(l.x), number(l.y)).arg(l.freeze_label)
				.arg(l.text_pos ? number(l.text_pos->x()) + QLatin1Char(',') + number(l.text_pos->y()) : QStringLiteral("-"),
					 l.text_rotation ? number(*l.text_rotation) : QStringLiteral("-"));
	});
	dumpCache(out, QStringLiteral("conductor_paths"), m_conductor_paths, [](const ConductorPath &path) {
		QStringList segments;
		for (const auto &s : path) segments << QStringLiteral("%1:%2").arg(s.horizontal).arg(number(s.length));
		return segments.join(QLatin1Char(','));
	});
	dumpCache(out, QStringLiteral("drawn_items"), m_drawn_items, [](const DrawnItem &d) {
		return QStringLiteral("%1 %2 %3").arg(d.folio.toString(), d.kind, rowsText(d.rows));
	});
	QStringList blobs;
	for (auto it = m_blob_refs.constBegin() ; it != m_blob_refs.constEnd() ; ++it)
		blobs << QStringLiteral("%1 %2").arg(it.key()).arg(it.value());
	blobs.sort();
	out << QStringLiteral("== cache blob_refs (%1)").arg(blobs.size()) << blobs;
	out << QStringLiteral("== state holds_links=%1 rows_from_document=%2 unjoined=%3 dirty_conductors=%4")
		   .arg(m_holds_links).arg(m_rows_from_document).arg(m_folios_with_unjoined_wires.size())
		   .arg(m_dirty_conductor_properties.size());
	return out.join(QLatin1Char('\n'));
}
