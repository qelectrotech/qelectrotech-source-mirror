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
#ifndef WORKINGCOPYRECOVERY_H
#define WORKINGCOPYRECOVERY_H

#include <QDateTime>
#include <QList>
#include <QString>

/**
	The working copies a QElectroTech that did not close normally left
	behind (QET-SQL-FORMAT S3z3, QETDB-S3-SCOPE.md 7.4): a project's
	database, kept in a file while it was open (projectDataBase's working
	copy), to offer at the next start with the recovery files.
*/
namespace WorkingCopyRecovery
{
	struct LeftWorkingCopy
	{
		QString path;           ///< the working copy, a .sqlite file
		QString project_file;   ///< the project it was, empty if never saved
		QDateTime committed;    ///< when its last edit was committed
	};

	QList<LeftWorkingCopy> leftWorkingCopies(const QString &dir);
	void remove(const QString &path);
}

#endif // WORKINGCOPYRECOVERY_H
