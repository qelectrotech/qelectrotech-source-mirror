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
#ifndef EPDZMANIFEST_H
#define EPDZMANIFEST_H

#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

/**
	@brief What the manifest (manifest.db) of an unpacked EPLAN export
	archive (.epdz) says: the pages with their title block values and
	drawing, the functions, the connections and the 3D installation
	spaces.

	An .epdz is a 7-Zip archive; manifest.db is a SQLite database whose
	"package" table lists every object (page, function, merged connection,
	installation space), "property" holds the EPLAN properties of each
	package by their EPLAN property number, and "item" maps a package to
	the files of the archive (the page's SVG, the space's .e3d). The
	table layout follows the format notes of ecad-view
	(https://github.com/Canary-ECAD/ecad-view, docs/formats.md, MIT); the
	code is our own.

	Kept apart from EpdzImport, which builds the project, so that the
	reading can be tested on a small database made in the test.
*/
namespace EpdzManifest
{
	struct Page {
		qint64 id = 0;
		QString name, title, folio, plant, location;
		QString svg;	///< the page's drawing in the unpacked folder, empty if none
	};
	struct Device {
		QString function, designation, article, description;
	};
	struct Connection {
		QString source, target, potential, colour, section, length;
	};
	struct Manifest {
		QString project_name;
		QList<Page> pages;		///< in the order of the manifest
		QList<Device> devices;
		QList<Connection> connections;
		QList<QPair<QString, QString>> spaces;	///< name, .e3d file
	};

	/// reads the manifest of the archive unpacked into @p folder; false,
	/// with @p error set, when there is none or it cannot be read
	bool read(const QString &folder, Manifest &manifest, QString &error);

	/// the value without the prefix EPLAN writes in front ("==EES" -> "EES")
	QString bare(QString value);

	/// a CSV cell for a ';' separated file: quoted when it holds the
	/// separator or a quote, line breaks replaced by spaces
	QString csvCell(QString text);
}

#endif // EPDZMANIFEST_H
