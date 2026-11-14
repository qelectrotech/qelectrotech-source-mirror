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
#include "bomexport.h"

#include <QSaveFile>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

#include <utility>

namespace {
QString csvField(QString value)
{
	value.replace(QLatin1Char('"'), QStringLiteral("\"\""));
	return QLatin1Char('"') + value + QLatin1Char('"');
}

}

QByteArray BomExport::csvRecord(const QStringList &values)
{
	QStringList escaped;
	for (const auto &value : values) {
		escaped.append(csvField(value));
	}
	return (escaped.join(QLatin1Char(';')) + QLatin1Char('\n')).toUtf8();
}

QStringList BomExport::defaultColumns()
{
	return {
		QStringLiteral("label"),
		QStringLiteral("designation"),
		QStringLiteral("manufacturer"),
		QStringLiteral("manufacturer_reference"),
		QStringLiteral("model"),
		QStringLiteral("description"),
		QStringLiteral("category"),
		QStringLiteral("quantity"),
		QStringLiteral("voltage_rating"),
		QStringLiteral("current_rating"),
		QStringLiteral("folio"),
		QStringLiteral("notes")
	};
}

/**
	@brief BomExport::junctionFilter
	@return a condition true for every row that is not a junction: a
	terminal-type element with no label and no part data. Some users draw
	the dots and bends where wires branch as their own terminal-type
	symbols, and each one became an empty row of the parts list (#1178).
	A terminal block with a label or a part number is kept.
*/
QString BomExport::junctionFilter()
{
	return QStringLiteral("NOT (element_type = 'terminal' "
						  "AND IFNULL(label, '') = '' "
						  "AND IFNULL(designation, '') = '' "
						  "AND IFNULL(manufacturer, '') = '' "
						  "AND IFNULL(manufacturer_reference, '') = '')");
}

/**
	@brief BomExport::defaultQuery
	@param include_slaves : false leaves out the contact blocks (slave
	elements), which otherwise appear as rows of their own
	@param include_junctions : false leaves out the junctions, see
	junctionFilter()
	@return the query of the parts list
*/
QString BomExport::defaultQuery(bool include_slaves, bool include_junctions)
{
		//Slaves and terminals are included because both are routinely
		//separately orderable hardware. A circuit breaker can carry ten or
		//twenty auxiliary blocks, each with its own order code, and a
		//terminal block is a purchased part in its own right. Neither shares
		//a line with its master: the query is ungrouped, one row per element,
		//so each appears as the distinct item it is.
		//
		//Anything that should not be ordered is kept out by setting
		//exclude_from_bom on the element, which the view already honours --
		//a relay's own auxiliary contact, say.
		//
		//Thumbnails are deliberately left out for now even though ten of
		//them in the shipped examples carry manufacturer and reference data
		//(the assembly-plan mounting-plate symbols), because that has not
		//been asked for and is a separate question. The folio report arrows
		//and the conductor definition stay out because they are not hardware.
		//
		//See discussion #847.
	const QString types = include_slaves
			? QStringLiteral("'simple', 'master', 'slave', 'terminal'")
			: QStringLiteral("'simple', 'master', 'terminal'");
	const QString junctions = include_junctions
			? QString()
			: QStringLiteral(" AND ") + junctionFilter();
	return QStringLiteral("SELECT %1 FROM element_nomenclature_view "
						  "WHERE element_type IN (%2)%3 "
						  "ORDER BY diagram_position, position, label")
			.arg(defaultColumns().join(QStringLiteral(", ")), types, junctions);
}

/**
	@brief BomExport::groupedQuery
	The parts list the way it is ordered: one line per part, however many
	elements use it. Parts are the same when manufacturer, part number and
	unit are. An element with no part number is never merged with another,
	since nothing says they are the same part.
	Elements with no part number, designation or description are not
	parts and stay out, which also keeps out junctions and folio arrows.
	Columns: number (optional), quantity, name, part_number, references
	(optional), manufacturer, unit.
	Idea from PR #1650 by victorienea.

	With options.costing, four more columns: unit_price, total_price,
	labour_time, total_labour, and a last line named options.total_label
	adding up the two totals for the project (#1878 A4). Each element
	counts with its own price and time, times its quantity when
	quantities are added up, so two elements of one part priced
	differently still give the right total; the unit price and time are
	shown only when every element of the part gives the same one. An
	element with no price adds nothing, and an auxiliary block never has
	one (#1883). Written with at most four decimals, like the fields
	themselves.
	@param options
	@return the SQL, to run on element_nomenclature_view
*/
QString BomExport::groupedQuery(const GroupedOptions &options)
{
		//The main part and each auxiliary block, as the same set of columns.
		//The row number tells two elements with no part number apart.
		//A price or a time is a number or nothing: an empty field must not
		//count as 0, or a part nobody priced would look free. Nor must text
		//that is no plain number ("€12", "1,234.50"): SQLite's CAST would
		//read it as 0 or 1.234, so it counts as no price.
	const auto number = [](const QString &field) {
		const QString value = QStringLiteral("REPLACE(TRIM(%1), ',', '.')").arg(field);
		return QStringLiteral(
			"CASE WHEN %1 GLOB '*[0-9]*' AND %1 NOT GLOB '*[^0-9.]*' "
			"AND %1 NOT GLOB '*.*.*' THEN CAST(%1 AS REAL) END").arg(value);
	};
		//Only read when asked for, so a list without costing runs on a
		//database which has no price columns.
	const QString costing_columns = options.costing
			? QStringLiteral("%1 AS price, %2 AS labour, ")
				.arg(number(QStringLiteral("unit_price")),
					 number(QStringLiteral("labour_time")))
			: QString();
	const auto part = [&](const QString &suffix) {
		return QStringLiteral(
			"SELECT TRIM(COALESCE(manufacturer%1, '')) AS manufacturer, "
			"TRIM(COALESCE(manufacturer_reference%1, '')) AS article, "
			"COALESCE(NULLIF(TRIM(description%1), ''), designation%1, '') AS name, "
			"label, TRIM(COALESCE(unity%1, '')) AS unit, "
			"CAST(REPLACE(COALESCE(NULLIF(TRIM(quantity%1), ''), '1'), ',', '.') AS REAL) AS qty, "
			"%2"
			"'row:' || CAST(ROW_NUMBER() OVER () AS TEXT) || ':%1' AS item_key "
			"FROM element_nomenclature_view "
			"WHERE TRIM(COALESCE(manufacturer_reference%1, '')) <> '' "
			"OR TRIM(COALESCE(designation%1, '')) <> '' "
			"OR TRIM(COALESCE(description%1, '')) <> ''")
				.arg(suffix,
						//An auxiliary block has no price or time of its own
					 (options.costing && !suffix.isEmpty())
						 ? QStringLiteral("NULL AS price, NULL AS labour, ")
						 : costing_columns);
	};
	QString source = part(QString());
	if (options.auxiliaries)
	{
		for (int i = 1 ; i <= 4 ; ++i) {
			source += QStringLiteral(" UNION ALL ")
					+ part(QStringLiteral("_auxiliary%1").arg(i));
		}
	}

		//How many of an element one line counts: its quantity field, or
		//one when the rows are counted rather than added up
	const QString times = options.sum_quantity ? QStringLiteral("qty * ")
											   : QString();
		//The same unit value on every element of the part, or nothing
	const auto same = [](const QString &field) {
		return QStringLiteral(
			"CASE WHEN COUNT(%1) = COUNT(*) AND MIN(%1) = MAX(%1) "
			"THEN ROUND(MIN(%1), 4) END").arg(field);
	};
	const QString grouped_costing = options.costing
			? QStringLiteral(", %1 AS unit_price, ROUND(SUM(%3price), 4) AS total_price, "
							 "%2 AS labour_time, ROUND(SUM(%3labour), 4) AS total_labour")
				.arg(same(QStringLiteral("price")), same(QStringLiteral("labour")), times)
			: QString();

	const QString grouped = QStringLiteral(
		"SELECT manufacturer, article, MIN(name) AS name, unit, %1 AS qty, "
		"GROUP_CONCAT(DISTINCT label) AS refs%3 FROM (%2) "
		"GROUP BY manufacturer, article, unit, "
		"CASE WHEN TRIM(article) = '' THEN COALESCE(label, '') || item_key ELSE '' END")
			.arg(options.sum_quantity ? QStringLiteral("SUM(qty)") : QStringLiteral("COUNT(*)"),
				 source, grouped_costing);

	QStringList columns;
	if (options.numbering) {
		columns << QStringLiteral("ROW_NUMBER() OVER (ORDER BY manufacturer, article, name, refs) AS number");
	}
	columns << QStringLiteral("qty AS quantity")
			<< QStringLiteral("name")
			<< QStringLiteral("article AS part_number");
	if (options.references) {
		columns << QStringLiteral("refs AS \"references\"");
	}
	columns << QStringLiteral("manufacturer") << QStringLiteral("unit");

	if (!options.costing) {
		return QStringLiteral("SELECT %1 FROM (%2) ORDER BY manufacturer, article, name, refs")
				.arg(columns.join(QStringLiteral(", ")), grouped);
	}

		//Written out as a plain decimal, at most four places and no
		//trailing zeros: read back as a double, 0.0001 would print as
		//"1e-04" in the file.
	const auto decimal = [](const QString &value) {
		return QStringLiteral(
			"CASE WHEN %1 IS NULL THEN NULL "
			"ELSE RTRIM(RTRIM(PRINTF('%.4f', %1), '0'), '.') END").arg(value);
	};
	columns << decimal(QStringLiteral("unit_price")) + QStringLiteral(" AS unit_price")
			<< decimal(QStringLiteral("total_price")) + QStringLiteral(" AS total_price")
			<< decimal(QStringLiteral("labour_time")) + QStringLiteral(" AS labour_time")
			<< decimal(QStringLiteral("total_labour")) + QStringLiteral(" AS total_labour");
	const QString parts = QStringLiteral(
		"SELECT %1, refs AS sort_refs, 0 AS last_line FROM (%2)")
			.arg(columns.join(QStringLiteral(", ")), grouped);

		//The last line: the same columns, empty but for its name and the
		//two totals. TOTAL() gives 0 rather than nothing when no part has
		//a price, which is the honest sum of no prices.
	QStringList total;
	if (options.numbering) total << QStringLiteral("NULL");
	total << QStringLiteral("NULL")
		  << QStringLiteral("'%1'").arg(QString(options.total_label)
											.replace(QLatin1Char('\''), QStringLiteral("''")))
		  << QStringLiteral("NULL");
	if (options.references) total << QStringLiteral("NULL");
	total << QStringLiteral("NULL") << QStringLiteral("NULL")
		  << QStringLiteral("NULL") << decimal(QStringLiteral("TOTAL(total_price)"))
		  << QStringLiteral("NULL") << decimal(QStringLiteral("TOTAL(total_labour)"))
		  << QStringLiteral("NULL") << QStringLiteral("1");
	const QString sum = QStringLiteral("SELECT %1 FROM (%2)")
			.arg(total.join(QStringLiteral(", ")), grouped);

		//The columns again, by name, to leave the sort key out
	QStringList names;
	for (const QString &column : std::as_const(columns)) {
		names << column.section(QStringLiteral(" AS "), -1);
	}
	return QStringLiteral(
		"SELECT %1 FROM (%2 UNION ALL %3) ORDER BY last_line, manufacturer, part_number, name, sort_refs")
			.arg(names.join(QStringLiteral(", ")), parts, sum);
}

QByteArray BomExport::toCsv(QSqlQuery &query, const QStringList &headers,
							bool include_headers, int *row_count)
{
	QByteArray csv("\xEF\xBB\xBF");
	if (include_headers) {
		csv += csvRecord(headers);
	}
	int rows = 0;
	while (query.next())
	{
		QStringList values;
		for (int i = 0; i < query.record().count(); ++i) {
			values.append(query.value(i).toString());
		}
		csv += csvRecord(values);
		++rows;
	}
	if (row_count) {
		*row_count = rows;
	}
	return csv;
}

bool BomExport::writeCsv(const QString &file_path, const QByteArray &csv,
						 QString *error)
{
	if (error) {
		error->clear();
	}
	QSaveFile file(file_path);
	if (!file.open(QIODevice::WriteOnly) || file.write(csv) != csv.size())
	{
		if (error) {
			*error = file.errorString();
		}
		file.cancelWriting();
		return false;
	}
	if (!file.commit())
	{
		if (error) {
			*error = file.errorString();
		}
		return false;
	}
	return true;
}
