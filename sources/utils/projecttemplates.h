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
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef PROJECTTEMPLATES_H
#define PROJECTTEMPLATES_H

#include <QList>
#include <QString>
#include <QStringList>

/**
	@brief The ProjectTemplates namespace
	Project templates (#852, item B13 of #1878). A template is an ordinary
	.qet file in a templates folder: its folios, title block templates,
	numbering rules, default properties and drawing standard are the ones a
	new project starts with. There is no template file format of its own.
	A template is read as XML and never opened as a project.
*/
namespace ProjectTemplates
{
		/// A template found in a folder
	struct Entry {
		QString name;   ///< file name without the extension
		QString path;   ///< absolute path of the .qet file
		QString origin; ///< the label of the folder it was found in
	};

		/// A folder to look in and what to call the templates found in it
	struct Folder {
		QString label;
		QString path;
	};

		/// The .qet files of @p folders, each folder by name, the folders in order
	QList<Entry> list(const QList<Folder> &folders);

		/**
			Writes to @p out_path the copy of the template @p template_path a
			new project is made from: no title (a saved project is named by
			its file) and a project uuid of its own, since the uuid says
			which project a file belongs to.
			@return false, with @p error, for a file that is not a .qet project
		*/
	bool prepareCopy(const QString &template_path, const QString &out_path, QString *error);
}

#endif
