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

/*
	QETSql::isSingleReadOnlyStatement() -- the read-only enforcement every
	project-database query goes through.

	The case that matters most here is the CTE prefix. SQLite has allowed
	WITH in front of DELETE/UPDATE/INSERT since 3.8.3, so a check on the
	first keyword lets a write through while reading as though it refused
	one. That is not hypothetical for QElectroTech: a <graphics_table>'s
	<query> is stored in the .qet and executed on load, so the text can
	arrive from a file rather than from the person at the keyboard.

	This test owns its own in-memory database and links nothing of
	QElectroTech but sqlreadonly.cpp, so it stays a fast, hermetic check of
	the security property itself.
*/

#include "dataBase/sqlreadonly.h"

#include <QtTest>

#include <sqlite3.h>

class TstSqlReadOnly : public QObject
{
		Q_OBJECT

	private slots:
		void initTestCase();
		void cleanupTestCase();

		void acceptsOrdinaryReads();
		void acceptsLegitimateCommonTableExpression();
		void refusesCtePrefixedWrites_data();
		void refusesCtePrefixedWrites();
		void refusesBareWrites_data();
		void refusesBareWrites();
		void refusesTrailingStatement();
		void refusesEmptyAndCommentOnly_data();
		void refusesEmptyAndCommentOnly();
		void refusesWithoutAConnection();
		void reportsAReason();
		void doesNotExecuteWhatItRefuses();

	private:
		sqlite3 *m_db = nullptr;
		int rowCount();
};

int TstSqlReadOnly::rowCount()
{
	sqlite3_stmt *st = nullptr;
	sqlite3_prepare_v2(m_db, "SELECT COUNT(*) FROM element", -1, &st, nullptr);
	sqlite3_step(st);
	const int n = sqlite3_column_int(st, 0);
	sqlite3_finalize(st);
	return n;
}

void TstSqlReadOnly::initTestCase()
{
	QCOMPARE(sqlite3_open(":memory:", &m_db), SQLITE_OK);
	QCOMPARE(sqlite3_exec(m_db,
		"CREATE TABLE element (uuid TEXT);"
		"INSERT INTO element VALUES ('a'),('b');", nullptr, nullptr, nullptr),
		SQLITE_OK);
	QCOMPARE(rowCount(), 2);
}

void TstSqlReadOnly::cleanupTestCase()
{
	sqlite3_close(m_db);
	m_db = nullptr;
}

void TstSqlReadOnly::acceptsOrdinaryReads()
{
	QVERIFY(QETSql::isSingleReadOnlyStatement(m_db, "SELECT * FROM element"));
	QVERIFY(QETSql::isSingleReadOnlyStatement(m_db, "SELECT uuid FROM element WHERE uuid = 'a'"));
	// A semicolon inside a string literal is not a second statement. The
	// textual check this replaced rejected exactly this.
	QVERIFY(QETSql::isSingleReadOnlyStatement(m_db, "SELECT ';' AS semicolon"));
	// One trailing semicolon is ordinary punctuation, not a second statement.
	QVERIFY(QETSql::isSingleReadOnlyStatement(m_db, "SELECT * FROM element;"));
}

void TstSqlReadOnly::acceptsLegitimateCommonTableExpression()
{
	// WITH must keep working -- the fix is not "ban CTEs".
	QVERIFY(QETSql::isSingleReadOnlyStatement(m_db,
		"WITH x AS (SELECT 1 AS n) SELECT n FROM x"));
	QVERIFY(QETSql::isSingleReadOnlyStatement(m_db,
		"WITH RECURSIVE c(n) AS (SELECT 1 UNION ALL SELECT n+1 FROM c) "
		"SELECT n FROM c LIMIT 3"));
}

void TstSqlReadOnly::refusesCtePrefixedWrites_data()
{
	QTest::addColumn<QString>("query");
	QTest::newRow("delete") << "WITH x AS (SELECT 1) DELETE FROM element";
	QTest::newRow("update") << "WITH x AS (SELECT 1) UPDATE element SET uuid = 'pwned'";
	QTest::newRow("insert") << "WITH x AS (SELECT 1) INSERT INTO element VALUES ('injected')";
}

void TstSqlReadOnly::refusesCtePrefixedWrites()
{
	QFETCH(QString, query);
	QVERIFY2(!QETSql::isSingleReadOnlyStatement(m_db, query),
		 qPrintable(QStringLiteral("accepted a write: %1").arg(query)));
}

void TstSqlReadOnly::refusesBareWrites_data()
{
	QTest::addColumn<QString>("query");
	QTest::newRow("delete") << "DELETE FROM element";
	QTest::newRow("update") << "UPDATE element SET uuid = 'pwned'";
	QTest::newRow("insert") << "INSERT INTO element VALUES ('injected')";
	QTest::newRow("drop")   << "DROP TABLE element";
}

void TstSqlReadOnly::refusesBareWrites()
{
	QFETCH(QString, query);
	QVERIFY(!QETSql::isSingleReadOnlyStatement(m_db, query));
}

void TstSqlReadOnly::refusesTrailingStatement()
{
	QVERIFY(!QETSql::isSingleReadOnlyStatement(m_db, "SELECT 1; DROP TABLE element"));
	QVERIFY(!QETSql::isSingleReadOnlyStatement(m_db, "SELECT 1; SELECT 2"));
}

void TstSqlReadOnly::refusesEmptyAndCommentOnly_data()
{
	QTest::addColumn<QString>("query");
	QTest::newRow("empty")      << "";
	QTest::newRow("whitespace") << "   ";
	QTest::newRow("comment")    << "-- nothing to see here";
}

void TstSqlReadOnly::refusesEmptyAndCommentOnly()
{
	// sqlite3_prepare_v2() reports success and a null statement for these;
	// sqlite3_stmt_readonly() must never be handed that.
	QFETCH(QString, query);
	QVERIFY(!QETSql::isSingleReadOnlyStatement(m_db, query));
}

void TstSqlReadOnly::refusesWithoutAConnection()
{
	// Fails closed: with no connection there is nothing to ask, and
	// guessing from the text is the weakness this replaced.
	QVERIFY(!QETSql::isSingleReadOnlyStatement(nullptr, "SELECT * FROM element"));
}

void TstSqlReadOnly::reportsAReason()
{
	QString reason;
	QVERIFY(!QETSql::isSingleReadOnlyStatement(
			m_db, "WITH x AS (SELECT 1) DELETE FROM element", &reason));
	QVERIFY2(!reason.isEmpty(), "a refusal must say why");

	reason = QStringLiteral("stale");
	QVERIFY(QETSql::isSingleReadOnlyStatement(m_db, "SELECT * FROM element", &reason));
	QVERIFY2(reason.isEmpty(), "an accepted query must not leave a reason behind");
}

void TstSqlReadOnly::doesNotExecuteWhatItRefuses()
{
	// The check compiles the statement to inspect it. Proving the table is
	// untouched afterwards is what says it compiled without running it --
	// and this same assertion goes red if the refusals above ever stop
	// refusing, since then the caller would run the DELETE for real.
	QCOMPARE(rowCount(), 2);
	QVERIFY(!QETSql::isSingleReadOnlyStatement(m_db, "WITH x AS (SELECT 1) DELETE FROM element"));
	QVERIFY(!QETSql::isSingleReadOnlyStatement(m_db, "DELETE FROM element"));
	QCOMPARE(rowCount(), 2);
}

QTEST_APPLESS_MAIN(TstSqlReadOnly)
#include "tst_sqlreadonly.moc"
