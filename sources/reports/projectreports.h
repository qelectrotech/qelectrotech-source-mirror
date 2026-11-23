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
#ifndef PROJECTREPORTS_H
#define PROJECTREPORTS_H

#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

class QETProject;

/**
	@brief The ProjectReports namespace
	The lists a project is built and wired from: parts, wires, potentials,
	terminals, cables, cable cores, PLC inputs and outputs. Each is a query
	of the project database's public views, so what it shows is the
	drawing's; each can be limited to a place and written to CSV or PDF.
*/
namespace ProjectReports
{
	struct Column
	{
		QString sql;        ///< the expression, in the report's source
		QString title;      ///< what the column is called
	};

	struct Report
	{
		QString id;                  ///< "wires", which does not change
		QString title;               ///< what the list is called
		QString source;              ///< FROM what
		QStringList places;          ///< the columns holding a designation
		bool places_inside = false;  ///< ...among others, as a list of them
		QString order;               ///< ORDER BY what
		bool potentials = false;     ///< reads the TEMP potential views
		QList<Column> columns;
	};

		/// A place of the project: both empty for the whole of it
	struct Place
	{
		QString plant;
		QString location;
		QString text() const;
	};

	struct Table
	{
		QStringList headers;
		QList<QStringList> rows;
		QString error;
	};

	QList<Report> all();
	Report report(const QString &id);
	QList<Place> places(QETProject *project);
	QString locationsSql();
	Table locations(QETProject *project);
	QString sql(const Report &report, const Place &place = Place());
	Table read(QETProject *project, const Report &report, const Place &place = Place());

	bool writeCsv(const Table &table, const QString &path, QString *error = nullptr);
	int writePdf(const Table &table, const QString &title, const QString &subtitle,
				 const QString &path, QString *error = nullptr);
}

#endif // PROJECTREPORTS_H
