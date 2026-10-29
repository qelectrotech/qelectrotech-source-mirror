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
#ifndef QETDBFILE_H
#define QETDBFILE_H

#include <QByteArray>
#include <QDomDocument>
#include <QString>
#include <QStringList>

#include <functional>

class projectDataBase;

/**
	A project saved as one SQLite file, .qetdb (QET-SQL-FORMAT S4a,
	QETDB-S4-SCOPE.md): the project database as it holds the whole project,
	with what says which program wrote it. Experimental.
*/
namespace QetDbFile
{
		/// PRAGMA application_id of a .qetdb: "QETD"
	constexpr int applicationId = 0x51455444;
		/// The format this QElectroTech writes, and the newest it reads
	constexpr int format = 1;

	bool isDatabasePath(const QString &path);
	bool isDatabase(const QByteArray &head);
		/// @p sha1, if given, gets the SHA-1 of the file written (what
		/// QETProject remembers to see it changed on disk), made from it
		/// before it replaces the project, while it is checked and flushed
		/// @p previous, if given, is the file this save follows when it is
		/// not @p path (Save As, a copy): the history it keeps goes on in
		/// the new file (QETDB-S6-SCOPE.md R4)
	bool save(projectDataBase &database, const QString &path, QString *error, QByteArray *sha1 = nullptr,
			  const QString &previous = QString());
	bool read(const QString &path, QDomDocument *document, QString *error,
			  const std::function<bool(const QString &copy)> &with_copy = {});
		/// History of the saves, kept in the file when switched on
		/// (QETDB-S6-SCOPE.md): on or off, and each revision as a line
	bool setHistory(const QString &path, bool on, QString *error);
	QStringList history(const QString &path, QString *error);
}

#endif // QETDBFILE_H
