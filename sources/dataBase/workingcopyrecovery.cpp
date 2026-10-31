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
#include "workingcopyrecovery.h"

#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QUuid>

#include <algorithm>

namespace WorkingCopyRecovery
{
	/**
		@brief leftWorkingCopies
		The working copies in @p dir that no running QElectroTech holds
		(their lock is free) and that have edits their project's file
		lacks, newest first. The others there are deleted: those left by a
		project closed while QElectroTech stopped, or still opening (not
		ready), or saved since its last edit, hold nothing to recover, and
		one that cannot be read cannot be recovered.
	*/
	QList<LeftWorkingCopy> leftWorkingCopies(const QString &dir)
	{
		QList<LeftWorkingCopy> left;
		const QFileInfoList files = QDir(dir).entryInfoList({QStringLiteral("*.sqlite")}, QDir::Files);
		for (const QFileInfo &file : files)
		{
			const QString path = file.absoluteFilePath();
			QLockFile lock(path.chopped(int(qstrlen(".sqlite"))) + QStringLiteral(".lock"));
				//Held: the project is open in a QElectroTech running now
			if (!lock.tryLock(0)) {
				continue;
			}
			LeftWorkingCopy copy;
			copy.path = path;
			bool to_recover = false;
			const QString connection = QStringLiteral("qet_left_working_copy_")
					+ QUuid::createUuid().toString();
			{
				QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
				db.setDatabaseName(path);
				if (db.open())
				{
					QSqlQuery query(db);
					if (query.exec(QStringLiteral("SELECT project_file, committed, modified, ready "
												  "FROM working_copy")) && query.next())
					{
						copy.project_file = query.value(0).toString();
						copy.committed = QDateTime::fromString(query.value(1).toString(), Qt::ISODateWithMs);
						to_recover = query.value(2).toInt() == 1 && query.value(3).toInt() == 1;
					}
				}
				db.close();
			}
			QSqlDatabase::removeDatabase(connection);
			lock.unlock();
			if (to_recover) {
				left << copy;
			} else {
				remove(path);
			}
		}
		std::sort(left.begin(), left.end(), [](const LeftWorkingCopy &a, const LeftWorkingCopy &b) {
			return a.committed > b.committed;
		});
		return left;
	}

	/**
		@brief remove
		Delete the working copy @p path and the files SQLite and its lock
		keep beside it
	*/
	void remove(const QString &path)
	{
		for (const char *suffix : {"", "-wal", "-shm", "-journal"})
			QFile::remove(path + QLatin1String(suffix));
		QFile::remove(path.chopped(int(qstrlen(".sqlite"))) + QStringLiteral(".lock"));
	}
}
