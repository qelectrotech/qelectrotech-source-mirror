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
#ifndef EPDZIMPORT_H
#define EPDZIMPORT_H

#include <QString>
#include <QStringList>

/**
	@brief Reads an EPLAN export archive (.epdz, as EPLAN eVIEW and the
	"export for viewing" write it) into a QElectroTech project with the
	pages as pictures: the part of "File > Import EPLAN project" that
	needs no Python. With the import program
	(tools/eplan_import/eplan_zw1/epdz_nach_qet.py) the same archive
	becomes an editable project instead.

	The import gives:
	- one folio per page, in the order of the manifest, with its title,
	  page number and structure (= plant, + location) in the title block
	  and the page drawn as a picture filling the folio;
	- beside the .qet: <name>_devices.csv (the functions with their device
	  tag 20001, article 20100 and designation 20193),
	  <name>_connections.csv (source, target, potential, colour, section,
	  length of every connection) and the folder <name>_3D with the .e3d
	  installation spaces.
	See EpdzManifest for how the archive is read.
*/
namespace EpdzImport
{
	struct Result {
		bool ok = false;
		QString error;
		QStringList notes;     ///< what was read, for the log of the dialog
		int pages = 0, functions = 0, connections = 0, spaces = 0;
	};
	/// unpacks @p epdz and writes the project @p target_qet
	Result run(const QString &epdz, const QString &target_qet);
	/// the same for an archive already unpacked into @p folder;
	/// @p name is the project title when the manifest names none
	Result runUnpacked(const QString &folder, const QString &name, const QString &target_qet);
}

#endif // EPDZIMPORT_H
