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
#ifndef BOMEXPORT_H
#define BOMEXPORT_H

#include <QByteArray>
#include <QString>
#include <QStringList>

class QSqlQuery;

namespace BomExport
{
	QStringList defaultColumns();
	QString defaultQuery();
	QByteArray toCsv(QSqlQuery &query, const QStringList &headers,
					 bool include_headers = true, int *row_count = nullptr);
	bool writeCsv(const QString &file_path, const QByteArray &csv,
				  QString *error = nullptr);
}

#endif // BOMEXPORT_H
