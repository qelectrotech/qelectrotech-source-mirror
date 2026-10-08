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
#ifndef SQLREADONLY_H
#define SQLREADONLY_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

/**
	Running a piece of SQL only if it reads.

	Deliberately its own translation unit, depending on nothing but Qt SQL:
	it is the enforcement point for every query QElectroTech runs against a
	project database, including queries that arrive from outside the
	application (a .qet file's saved report/table query), so it is worth
	being able to test it in isolation -- see
	tests/qttest/tst_sqlreadonly.cpp, which links this file and nothing
	else of QElectroTech.
*/
namespace QETSql {

	QSqlQuery execReadOnly(const QSqlDatabase &db,
			       const QString &query,
			       QString *error = nullptr);
}

#endif // SQLREADONLY_H
