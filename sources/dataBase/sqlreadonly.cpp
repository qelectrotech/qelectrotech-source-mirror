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
#include "sqlreadonly.h"

#include <QCoreApplication>
#include <QSqlError>

namespace QETSql {

/**
	@brief QETSql::execReadOnly
	Run @p query on @p db with SQLite's query_only pragma set, so that
	SQLite itself refuses anything that would write.

	Why SQLite is asked rather than the text inspected: a check on the
	query's first keyword cannot see what the statement actually does.
	SQLite has allowed a CTE prefix in front of a data-modifying statement
	since 3.8.3, so

	@code
	WITH x AS (SELECT 1) DELETE FROM element
	@endcode

	begins with WITH, contains no semicolon, and deletes every row. With
	query_only set, SQLite fails that statement with SQLITE_READONLY when it
	tries to start writing, before any row is touched, while an ordinary
	WITH ... SELECT still runs.

	Why a pragma rather than sqlite3_stmt_readonly(): that needs the
	driver's native sqlite3 handle passed to the libsqlite3 QElectroTech
	links. The QSQLITE plugin of the Qt online installer carries its own
	private copy of SQLite, so that handle belongs to another library and
	the call crashes (qelectrotech-source-mirror#1045). A pragma goes
	through the driver, whichever SQLite it uses.

	A statement that succeeded here is read-only, so running the returned
	query again, as several callers do, runs a read-only statement again.
	A refused one comes back as an empty query with nothing to run again:
	query_only is only set for the duration of this call.

	Qt's SQLite driver refuses a second statement after the first one, so
	"SELECT 1; DROP TABLE element" is refused too.

	This is a read-only test, NOT a statement-type allowlist: query_only
	does not refuse ATTACH, BEGIN or most PRAGMAs, because none of them
	change the contents of the database. Callers that need to restrict
	which *kind* of statement is acceptable must say so separately --
	projectDataBase::newQuery() keeps isReadOnlySelect() in front of this
	for exactly that reason.

	@param db the connection to run the query on
	@param query the raw SQL text
	@param error set to a human-readable reason when the query is refused
	@return the executed query, or an empty query on @p db if @p query was
	refused or failed
*/
QSqlQuery execReadOnly(const QSqlDatabase &db, const QString &query, QString *error)
{
	if (error) {
		error->clear();
	}

	if (!QSqlQuery(db).exec(QStringLiteral("PRAGMA query_only = ON"))) {
		if (error) {
			*error = QCoreApplication::translate("QETSql",
				"Impossible de vérifier la requête : "
				"la base de données ne peut pas être mise en lecture seule.");
		}
		return QSqlQuery(db);
	}

	QSqlQuery result(db);
	const bool ok = result.exec(query);
	QSqlQuery(db).exec(QStringLiteral("PRAGMA query_only = OFF"));

	if (ok) {
		return result;
	}

	if (error) {
		// SQLITE_READONLY is 8; extended codes keep it in the low byte.
		if ((result.lastError().nativeErrorCode().toInt() & 0xff) == 8) {
			*error = QCoreApplication::translate("QETSql",
				"Seules les requêtes en lecture seule sont autorisées : "
				"cette requête modifierait la base de données.");
		} else {
			*error = QCoreApplication::translate("QETSql",
					"Requête SQL invalide : %1")
				 .arg(result.lastError().databaseText());
		}
	}
	return QSqlQuery(db);
}

} // namespace QETSql
