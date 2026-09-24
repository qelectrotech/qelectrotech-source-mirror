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

#include <sqlite3.h>

namespace QETSql {

/**
	@brief QETSql::isSingleReadOnlyStatement
	Ask SQLite itself whether @p query is exactly one statement, and whether
	that statement only reads.

	Why SQLite is asked rather than the text inspected: a check on the
	query's first keyword cannot see what the statement actually does.
	SQLite has allowed a CTE prefix in front of a data-modifying statement
	since 3.8.3, so

	@code
	WITH x AS (SELECT 1) DELETE FROM element
	@endcode

	begins with WITH, contains no semicolon, and deletes every row.
	sqlite3_stmt_readonly() reports on the statement SQLite compiled, not
	on how it was spelled, so the same query is correctly refused here
	while an ordinary WITH ... SELECT still passes.

	The statement is compiled and immediately finalised; sqlite3_prepare_v2()
	does not run it, so nothing is executed to reach this verdict.

	This is a read-only test, NOT a statement-type allowlist: SQLite
	considers ATTACH, BEGIN and several PRAGMAs read-only too, because none
	of them change the contents of the database. Callers that need to
	restrict which *kind* of statement is acceptable must say so separately
	-- projectDataBase::newQuery() keeps isReadOnlySelect() in front of this
	for exactly that reason.

	@param handle the connection the query would run on. A null handle is
	refused rather than waved through: without it there is nothing to ask,
	and guessing from the text is the weakness this exists to replace.
	@param query the raw SQL text
	@param error set to a human-readable reason when this returns false
	@return true if @p query is a single, read-only statement
*/
bool isSingleReadOnlyStatement(sqlite3 *handle, const QString &query, QString *error)
{
	if (error) {
		error->clear();
	}

	if (!handle) {
		if (error) {
			*error = QCoreApplication::translate("QETSql",
				"Impossible de vérifier la requête : "
				"aucune connexion SQLite disponible.");
		}
		return false;
	}

	const QByteArray utf8 = query.toUtf8();
	sqlite3_stmt *statement = nullptr;
	const char *tail = nullptr;

	if (sqlite3_prepare_v2(handle, utf8.constData(), utf8.size(),
			       &statement, &tail) != SQLITE_OK)
	{
		if (error) {
			*error = QCoreApplication::translate("QETSql",
					"Requête SQL invalide : %1")
				 .arg(QString::fromUtf8(sqlite3_errmsg(handle)));
		}
		sqlite3_finalize(statement);
		return false;
	}

	// Whitespace or a bare comment compiles successfully to no statement
	// at all, and sqlite3_stmt_readonly() must not be handed that.
	if (!statement) {
		if (error) {
			*error = QCoreApplication::translate("QETSql",
				"La requête ne contient aucune instruction.");
		}
		return false;
	}

	const bool read_only = sqlite3_stmt_readonly(statement) != 0;
	sqlite3_finalize(statement);

	if (!read_only) {
		if (error) {
			*error = QCoreApplication::translate("QETSql",
				"Seules les requêtes en lecture seule sont autorisées : "
				"cette requête modifierait la base de données.");
		}
		return false;
	}

	// tail points just past the first statement, semicolon included.
	// Anything left once semicolons and spacing are stripped is a second
	// statement -- caught structurally here, where "SELECT ';'" is a
	// perfectly ordinary query rather than a suspicious string.
	if (tail) {
		QString rest = QString::fromUtf8(tail);
		rest.remove(QLatin1Char(';'));
		if (!rest.trimmed().isEmpty()) {
			if (error) {
				*error = QCoreApplication::translate("QETSql",
					"Une seule requête est autorisée.");
			}
			return false;
		}
	}

	return true;
}

} // namespace QETSql
