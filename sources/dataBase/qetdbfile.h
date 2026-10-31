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
		/// What opening a .qetdb found wrong with its pages, and repaired
		/// from the repair data it carries (QETDB-S4C-PLAN.md P3)
	struct Repair
	{
		bool parity = false;  ///< it carries repair data that could be read
		bool edited = false;  ///< another program wrote to it since its save: nothing repaired
		bool header = false;  ///< its header was destroyed and is written again
		bool scanned = false; ///< its repair data was found by reading its pages: SQLite could not read it
		int bad_pages = 0;    ///< pages not as its save wrote them
		int beyond = 0;       ///< groups of pages with more bad ones than repair data
			/// Its content is not what was saved, though SQLite and
			/// QElectroTech's own checks pass: refused, unless the user
			/// chooses to open it with its changes (OutsideEdit::KeepChanges)
		bool content_differs = false;
		bool back_to_save = false;  ///< another program's changes undone, its saved state rebuilt
		bool kept_changes = false;  ///< opened with content that is not what was saved, as the user chose
		bool parity_damaged = false; ///< its repair data itself was damaged, and is left out of the copy
		bool repaired() const { return (bad_pages > 0 || header || parity_damaged) && !beyond; }
	};
		/// What an open does with a file whose content is not what was saved,
		/// another program having written to it (QETDB-S4C-PLAN.md P5)
	enum class OutsideEdit
	{
		Refuse,       ///< refused, with Repair::content_differs set, for the user to choose
		BackToSave,   ///< its pages rebuilt as saved, from its repair data
		KeepChanges   ///< opened as it is, the content check skipped: no one is locked out of a file
	};
	bool read(const QString &path, QDomDocument *document, QString *error,
			  const std::function<bool(const QString &copy)> &with_copy = {}, Repair *repair = nullptr,
			  OutsideEdit outside = OutsideEdit::Refuse, QHash<QString, QByteArray> *table_digests = nullptr);
		/// The .qetdb @p path repaired into @p output, from the repair data it
		/// carries, and sealed with new repair data; @p path is not written.
		/// @p back_to_save: another program's changes undone too
		/// @return false, with @p error, if @p output is not a sound file
	bool repair(const QString &path, const QString &output, Repair *report, QString *error,
				bool back_to_save = false);
		/// History of the saves, kept in the file when switched on
		/// (QETDB-S6-SCOPE.md): on or off, and each revision as a line
	bool setHistory(const QString &path, bool on, QString *error);
	QStringList history(const QString &path, QString *error);
	QStringList historyDiff(const QString &path, qint64 from, qint64 to, QString *error);
		/// The last save that changed the symbol, wire or folio @p uuid,
		/// as "ord  time  who@host  added|changed"; empty if none did
	QString lastChange(const QString &path, const QString &uuid, QString *error);
	bool exportDelivery(const QString &path, const QString &output, QString *error);
		/// Only the last @p saves revisions kept, 0 for all (S6-6)
	bool setHistoryLimit(const QString &path, int saves, QString *error);
	int historyLimit(const QString &path);
	bool writeRevision(const QString &path, qint64 ord, const QString &output, QString *error,
					   bool *same_digest = nullptr);
		/// The checks a file passes before it is opened: not damaged,
		/// ours, a format this QElectroTech reads
	bool isReadable(const QString &path, QString *error);
}

#endif // QETDBFILE_H
