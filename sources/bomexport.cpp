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

namespace {
QString csvField(QString value)
{
	value.replace(QLatin1Char('"'), QStringLiteral("\"\""));
	return QLatin1Char('"') + value + QLatin1Char('"');
}

QByteArray csvRecord(const QStringList &values)
{
	QStringList escaped;
	for (const auto &value : values) {
		escaped.append(csvField(value));
	}
	return (escaped.join(QLatin1Char(';')) + QLatin1Char('\n')).toUtf8();
}
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

QString BomExport::defaultQuery()
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
	return QStringLiteral("SELECT %1 FROM element_nomenclature_view "
						  "WHERE element_type IN "
						  "('simple', 'master', 'slave', 'terminal') "
						  "ORDER BY diagram_position, position, label")
			.arg(defaultColumns().join(QStringLiteral(", ")));
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
