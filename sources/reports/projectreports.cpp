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
#include "projectreports.h"

#include "../dataBase/projectdatabase.h"
#include "../qetproject.h"

#include <QCoreApplication>
#include <QFile>
#include <QFontMetricsF>
#include <QObject>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QSaveFile>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTextStream>

#include <algorithm>

namespace ProjectReports
{
	namespace
	{
			/// Where the titles of the reports are translated
		struct Text
		{
			Q_DECLARE_TR_FUNCTIONS(ProjectReports)
		};

			/// A device and its pin: =P+L-K1:13
		QString pin(const QString &device, const QString &terminal)
		{
			return QStringLiteral("%1 || CASE WHEN COALESCE(%2, '') <> '' THEN ':' || %2 ELSE '' END")
					.arg(device, terminal);
		}

		QString quoted(QString text)
		{
			text.replace(QLatin1Char('\''), QStringLiteral("''"));
			return QLatin1Char('\'') + text + QLatin1Char('\'');
		}

			/// How a designation starts in @p place: "=P+L-", or "=P+" and "=P-"
		QStringList prefixes(const Place &place)
		{
			QString start;
			if (!place.plant.isEmpty()) start += QLatin1Char('=') + place.plant;
			if (!place.location.isEmpty())
				return {start + QLatin1Char('+') + place.location + QLatin1Char('-')};
			return {start + QLatin1Char('+'), start + QLatin1Char('-')};
		}

		QString csvField(const QString &value)
		{
			if (value.contains(QLatin1Char(';')) || value.contains(QLatin1Char('"'))
					|| value.contains(QLatin1Char('\n'))) {
				QString escaped = value;
				escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
				return QLatin1Char('"') + escaped + QLatin1Char('"');
			}
			return value;
		}
	}

	QString Place::text() const
	{
		QString place;
		if (!plant.isEmpty()) place += QLatin1Char('=') + plant;
		if (!location.isEmpty()) place += QLatin1Char('+') + location;
		return place;
	}

	/**
		@return every report, in the order they are offered
	*/
	QList<Report> all()
	{
		const QString number_order = QStringLiteral(
				"CASE WHEN wire_number GLOB '[0-9]*' THEN 0 ELSE 1 END, "
				"CAST(wire_number AS INTEGER), wire_number");

		QList<Report> list;
		{
			Report r;
			r.id = QStringLiteral("parts");
			r.title = Text::tr("Parts list");
			r.source = QStringLiteral("qet_device_list");
			r.places = {QStringLiteral("structure_id")};
			r.order = QStringLiteral("structure_id");
			r.columns = {{QStringLiteral("structure_id"), Text::tr("Device")},
						 {QStringLiteral("description"), Text::tr("Description")},
						 {QStringLiteral("model"), Text::tr("Model")},
						 {QStringLiteral("articles"), Text::tr("Part number")},
						 {QStringLiteral("symbols"), Text::tr("Symbols")},
						 {QStringLiteral("folio_number"), Text::tr("Sheet")},
						 {QStringLiteral("position"), Text::tr("Position")}};
			list << r;
		}
		{
			Report r;
			r.id = QStringLiteral("wires");
			r.title = Text::tr("Wire list");
			r.source = QStringLiteral("qet_wires");
			r.places = {QStringLiteral("from_structure_id"), QStringLiteral("to_structure_id")};
			r.order = QStringLiteral("folio_number, ") + number_order;
			r.columns = {{QStringLiteral("wire_number"), Text::tr("Wire")},
						 {QStringLiteral("section"), Text::tr("Section")},
						 {QStringLiteral("colour"), Text::tr("Colour")},
						 {pin(QStringLiteral("from_structure_id"), QStringLiteral("from_terminal")), Text::tr("From")},
						 {pin(QStringLiteral("to_structure_id"), QStringLiteral("to_terminal")), Text::tr("To")},
						 {QStringLiteral("cable"), Text::tr("Cable")},
						 {QStringLiteral("folio_number"), Text::tr("Sheet")}};
			list << r;
		}
		{
				//The devices' pins, not the folio reports which only carry
				//the potential across folios: their labels are its names
			Report r;
			r.id = QStringLiteral("potentials");
			r.title = Text::tr("Potential list");
			r.potentials = true;
			r.source = QStringLiteral(
				"(WITH ends AS ("
					"SELECT m.potential_id AS id, w.from_device_uuid AS device, %1 AS pin "
					"FROM qet_wires w JOIN temp.potential_member m ON m.wire_uuid = w.wire_uuid "
					"UNION SELECT m.potential_id, w.to_device_uuid, %2 "
					"FROM qet_wires w JOIN temp.potential_member m ON m.wire_uuid = w.wire_uuid), "
				"pins AS (SELECT id, group_concat(pin, ', ') AS pins, COUNT(*) AS count FROM "
					"(SELECT e.id, e.pin FROM ends e JOIN element d ON d.uuid = e.device "
					"WHERE d.type NOT IN ('next_report', 'previous_report') ORDER BY e.id, e.pin) "
					"GROUP BY id) "
				"SELECT v.wire_numbers, v.names, v.wires, v.first_folio_number, v.folios, "
				"p.pins, COALESCE(p.count, 0) AS count "
				"FROM temp.potential_view v LEFT JOIN pins p ON p.id = v.potential_id)")
					.arg(pin(QStringLiteral("w.from_structure_id"), QStringLiteral("w.from_terminal")),
						 pin(QStringLiteral("w.to_structure_id"), QStringLiteral("w.to_terminal")));
			r.places = {QStringLiteral("pins")};
			r.places_inside = true;
			r.order = QStringLiteral("first_folio_number, wire_numbers");
			r.columns = {{QStringLiteral("wire_numbers"), Text::tr("Wire numbers")},
						 {QStringLiteral("names"), Text::tr("Names")},
						 {QStringLiteral("count"), Text::tr("Pins")},
						 {QStringLiteral("pins"), Text::tr("Connected")},
						 {QStringLiteral("first_folio_number"), Text::tr("First sheet")},
						 {QStringLiteral("folios"), Text::tr("Sheets")}};
			list << r;
		}
		{
			Report r;
			r.id = QStringLiteral("terminals");
			r.title = Text::tr("Terminal list");
			r.source = QStringLiteral("qet_terminals");
			r.places = {QStringLiteral("strip_id")};
			r.order = QStringLiteral("strip_id, strip_uuid, position, level");
			r.columns = {{QStringLiteral("strip_id"), Text::tr("Strip")},
						 {QStringLiteral("position + 1"), Text::tr("Position")},
						 {QStringLiteral("label"), Text::tr("Terminal")},
						 {QStringLiteral("wires"), Text::tr("Wires")},
						 {QStringLiteral("connected_to"), Text::tr("Connected")},
						 {QStringLiteral("folio_number"), Text::tr("Sheet")}};
			list << r;
		}
		{
			Report r;
			r.id = QStringLiteral("cables");
			r.title = Text::tr("Cable list");
			r.source = QStringLiteral("qet_cables");
			r.places = {QStringLiteral("structure_id")};
			r.order = QStringLiteral("structure_id");
			r.columns = {{QStringLiteral("structure_id"), Text::tr("Cable")},
						 {QStringLiteral("type"), Text::tr("Type")},
						 {QStringLiteral("cores"), Text::tr("Cores")},
						 {QStringLiteral("wired_cores"), Text::tr("Wired")},
						 {QStringLiteral("length"), Text::tr("Length")},
						 {QStringLiteral("folio_number"), Text::tr("Sheet")}};
			list << r;
		}
		{
			Report r;
			r.id = QStringLiteral("cores");
			r.title = Text::tr("Cable core list");
			r.source = QStringLiteral("qet_cable_cores");
			r.places = {QStringLiteral("structure_id")};
			r.order = QStringLiteral("structure_id, cable_uuid, position");
			r.columns = {{QStringLiteral("structure_id"), Text::tr("Cable")},
						 {QStringLiteral("core"), Text::tr("Core")},
						 {QStringLiteral("wire_number"), Text::tr("Wire")},
						 {QStringLiteral("section"), Text::tr("Section")},
						 {pin(QStringLiteral("from_structure_id"), QStringLiteral("from_terminal")), Text::tr("From")},
						 {pin(QStringLiteral("to_structure_id"), QStringLiteral("to_terminal")), Text::tr("To")},
						 {QStringLiteral("folio_number"), Text::tr("Sheet")}};
			list << r;
		}
		{
			Report r;
			r.id = QStringLiteral("plc");
			r.title = Text::tr("PLC input and output list");
			r.source = QStringLiteral("qet_plc_io");
			r.places = {QStringLiteral("structure_id")};
			r.order = QStringLiteral("structure_id, plc_uuid, entry");
			r.columns = {{QStringLiteral("structure_id"), Text::tr("PLC")},
						 {QStringLiteral("article"), Text::tr("Part number")},
						 {QStringLiteral("entry"), Text::tr("Entry")},
						 {QStringLiteral("address"), Text::tr("Address")},
						 {QStringLiteral("type"), Text::tr("Type")},
						 {QStringLiteral("function"), Text::tr("Function")},
						 {QStringLiteral("folio_number"), Text::tr("Sheet")},
						 {QStringLiteral("position"), Text::tr("Position")}};
			list << r;
		}
		return list;
	}

	Report report(const QString &id)
	{
		for (const Report &r : all())
			if (r.id == id) return r;
		return Report();
	}

	/**
		@return the plants and the locations of the project, a plant before
		its locations
	*/
	QList<Place> places(QETProject *project)
	{
		QList<Place> list;
		if (!project || !project->dataBase()) return list;
		project->dataBase()->updateDB();
		QSqlQuery query = project->dataBase()->newQuery(QStringLiteral(
				"SELECT DISTINCT plant, location FROM qet_locations "
				"ORDER BY plant, location"));
		QString last_plant;
		bool first = true;
		while (query.next()) {
			const QString plant = query.value(0).toString().trimmed();
			const QString location = query.value(1).toString().trimmed();
			if ((first || plant != last_plant) && !plant.isEmpty())
				list << Place{plant, QString()};
			first = false;
			last_plant = plant;
			if (!location.isEmpty())
				list << Place{plant, location};
		}
		return list;
	}

	/**
		@return the query of @p report, limited to @p place
	*/
	QString sql(const Report &report, const Place &place)
	{
		QStringList columns;
		for (int i = 0 ; i < report.columns.size() ; ++i)
			columns << QStringLiteral("(%1) AS c%2").arg(report.columns.at(i).sql).arg(i);
		QString text = QStringLiteral("SELECT %1 FROM %2").arg(columns.join(QStringLiteral(", ")), report.source);
		if (!place.plant.isEmpty() || !place.location.isEmpty()) {
			QStringList tests;
			const QStringList starts = prefixes(place);
			for (const QString &column : report.places) {
				for (const QString &start : starts) {
					if (report.places_inside)
						tests << QStringLiteral("instr(%1, %2) > 0").arg(column, quoted(start));
					else
						tests << QStringLiteral("substr(%1, 1, %2) = %3")
								 .arg(column).arg(start.size()).arg(quoted(start));
				}
			}
			text += QStringLiteral(" WHERE ") + tests.join(QStringLiteral(" OR "));
		}
		if (!report.order.isEmpty())
			text += QStringLiteral(" ORDER BY ") + report.order;
		return text;
	}

	/**
		@return @p report as it stands now, limited to @p place
	*/
	Table read(QETProject *project, const Report &report, const Place &place)
	{
		Table table;
		for (const Column &column : report.columns)
			table.headers << column.title;
		if (!project || !project->dataBase()) {
			table.error = QObject::tr("The project has no database.");
			return table;
		}
		project->dataBase()->updateDB();
		if (report.potentials)
			projectDataBase::refreshPotentialMembersOf(project->dataBase()->database());
		QSqlQuery query = project->dataBase()->newQuery(sql(report, place), &table.error);
		if (!table.error.isEmpty()) return table;
		while (query.next()) {
			QStringList row;
			for (int i = 0 ; i < report.columns.size() ; ++i)
				row << query.value(i).toString();
			table.rows << row;
		}
		return table;
	}

	/**
		@brief writeCsv
		Semicolon separated, as the project's other lists.
	*/
	bool writeCsv(const Table &table, const QString &path, QString *error)
	{
		QSaveFile file(path);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			if (error) *error = file.errorString();
			return false;
		}
		QTextStream out(&file);
		const auto line = [&out](const QStringList &fields) {
			QStringList cells;
			for (const QString &field : fields) cells << csvField(field);
			out << cells.join(QLatin1Char(';')) << '\n';
		};
		line(table.headers);
		for (const QStringList &row : table.rows) line(row);
		out.flush();
		if (!file.commit()) {
			if (error) *error = file.errorString();
			return false;
		}
		return true;
	}

	/**
		@brief writePdf
		The list on landscape A4 pages: its title and the column titles on
		each page, long cells wrapped, the page number at the foot.
		@return the number of pages, 0 on failure
	*/
	int writePdf(const Table &table, const QString &title, const QString &subtitle,
				 const QString &path, QString *error)
	{
		QPdfWriter writer(path);
		writer.setTitle(title);
		writer.setResolution(300);
		writer.setPageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Landscape,
										 QMarginsF(10, 10, 10, 10), QPageLayout::Millimeter));
		QPainter painter;
		if (!painter.begin(&writer)) {
			if (error) *error = QObject::tr("Cannot write the file.");
			return 0;
		}
		QFont font = painter.font();
		font.setPointSizeF(8);
		QFont bold = font;
		bold.setBold(true);
		QFont heading = font;
		heading.setPointSizeF(12);
		heading.setBold(true);
		const QFontMetricsF metrics(font, &writer);
		const QFontMetricsF bold_metrics(bold, &writer);
		const QRectF page(QPointF(0, 0), QSizeF(writer.width(), writer.height()));
		const qreal gap = metrics.averageCharWidth();
		const int columns = int(table.headers.size());
		if (!columns) {
			painter.end();
			return 0;
		}

			//Each column as wide as its text, a long one not more than a
			//third of the page; what is over or left is shared out
		QList<qreal> widths(columns, 0);
		for (int c = 0 ; c < columns ; ++c)
			widths[c] = bold_metrics.horizontalAdvance(table.headers.at(c));
		for (const QStringList &row : table.rows)
			for (int c = 0 ; c < columns ; ++c)
				widths[c] = std::max(widths.at(c), std::min(metrics.horizontalAdvance(row.at(c)),
															page.width() / 3));
		qreal total = 0;
		for (qreal &width : widths) {
			width += 2 * gap;
			total += width;
		}
		const qreal scale = page.width() / total;
		for (qreal &width : widths) width *= scale;

		const int flags = Qt::TextWordWrap | Qt::TextWrapAnywhere | Qt::AlignLeft | Qt::AlignTop;
		const auto heightOf = [&](const QStringList &row, const QFontMetricsF &m) {
			qreal height = m.height();
			for (int c = 0 ; c < columns ; ++c)
				height = std::max(height, m.boundingRect(QRectF(0, 0, widths.at(c) - 2 * gap, 1e7),
														 flags, row.at(c)).height());
			return height + gap / 2;
		};

			//Which rows go on which page, before anything is drawn: each
			//page says how many there are
		const qreal title_height = QFontMetricsF(heading, &writer).height() * 1.4
				+ (subtitle.isEmpty() ? 0 : metrics.height() * 1.4);
		const qreal header_height = heightOf(table.headers, bold_metrics);
		const qreal foot_height = metrics.height() * 1.4;
		const qreal body = page.height() - title_height - header_height - foot_height;
		QList<qreal> heights;
		heights.reserve(table.rows.size());
		QList<int> first_of_page{0};
		qreal used = 0;
		for (int r = 0 ; r < table.rows.size() ; ++r) {
			const qreal height = std::min(heightOf(table.rows.at(r), metrics), body);
			heights << height;
			if (used + height > body) {
				first_of_page << r;
				used = 0;
			}
			used += height;
		}

		const auto drawRow = [&](const QStringList &row, qreal y, qreal height) {
			qreal x = 0;
			for (int c = 0 ; c < columns ; ++c) {
				painter.drawText(QRectF(x + gap, y, widths.at(c) - 2 * gap, height), flags, row.at(c));
				x += widths.at(c);
			}
		};
		const int pages = int(first_of_page.size());
		for (int p = 0 ; p < pages ; ++p) {
			if (p) writer.newPage();
			painter.setFont(heading);
			qreal y = 0;
			painter.drawText(QRectF(0, y, page.width(), title_height), Qt::AlignLeft | Qt::AlignTop, title);
			y += QFontMetricsF(heading, &writer).height() * 1.4;
			painter.setFont(font);
			if (!subtitle.isEmpty())
				painter.drawText(QRectF(0, y, page.width(), metrics.height() * 1.4),
								 Qt::AlignLeft | Qt::AlignTop, subtitle);
			y = title_height;
			painter.setFont(bold);
			drawRow(table.headers, y, header_height);
			y += header_height;
			painter.drawLine(QPointF(0, y), QPointF(page.width(), y));
			painter.setFont(font);
			const int end = p + 1 < pages ? first_of_page.at(p + 1) : int(table.rows.size());
			for (int r = first_of_page.at(p) ; r < end ; ++r) {
				drawRow(table.rows.at(r), y, heights.at(r));
				y += heights.at(r);
			}
			painter.drawText(QRectF(0, page.height() - foot_height, page.width(), foot_height),
							 Qt::AlignRight | Qt::AlignBottom,
							 QObject::tr("Page %1 of %2", "project report").arg(p + 1).arg(pages));
		}
		painter.end();
		return pages;
	}
}
