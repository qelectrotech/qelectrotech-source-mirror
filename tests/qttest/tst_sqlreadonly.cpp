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
	QETSql::execReadOnly() -- the read-only enforcement every
	project-database query goes through.

	The case that matters most here is the CTE prefix. SQLite has allowed
	WITH in front of DELETE/UPDATE/INSERT since 3.8.3, so a check on the
	first keyword lets a write through while reading as though it refused
	one. That is not hypothetical for QElectroTech: a <graphics_table>'s
	<query> is stored in the .qet and executed on load, so the text can
	arrive from a file rather than from the person at the keyboard.

	The connection is a QSQLITE one, as in QElectroTech, so this runs
	through whichever SQLite the Qt driver carries -- the point of #1045,
	where the previous check crashed because it did not.

	This test owns its own in-memory database and links nothing of
	QElectroTech but sqlreadonly.cpp, so it stays a fast, hermetic check of
	the security property itself.
*/

#include "dataBase/sqlreadonly.h"

#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>

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
		void reportsAReason();
		void doesNotExecuteWhatItRefuses();
		void refusedQueryCannotBeRunAgain();
		void acceptedQueryRunAgainStillReads();
		void leavesTheConnectionWritable();

	private:
		QSqlDatabase m_db;
		int rowCount();
		bool isAccepted(const QString &query, QString *error = nullptr);
};

int TstSqlReadOnly::rowCount()
{
	QSqlQuery q(m_db);
	q.exec(QStringLiteral("SELECT COUNT(*) FROM element"));
	q.next();
	return q.value(0).toInt();
}

bool TstSqlReadOnly::isAccepted(const QString &query, QString *error)
{
	QString reason;
	const QSqlQuery q = QETSql::execReadOnly(m_db, query, &reason);
	if (error) {
		*error = reason;
	}
	// Accepted means both: no reason given, and a query that actually ran.
	// A refusal must be both too, or a caller could act on either half.
	const bool accepted = reason.isEmpty();
	if (accepted != q.isActive()) {
		qWarning() << "reason and query state disagree for" << query
			   << reason << q.isActive();
		return !accepted; // fails whichever way the test expected
	}
	return accepted;
}

void TstSqlReadOnly::initTestCase()
{
	m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
					 QStringLiteral("tst_sqlreadonly"));
	QVERIFY(m_db.open());
	QSqlQuery q(m_db);
	QVERIFY(q.exec(QStringLiteral("CREATE TABLE element (uuid TEXT)")));
	QVERIFY(q.exec(QStringLiteral("INSERT INTO element VALUES ('a'),('b')")));
	QCOMPARE(rowCount(), 2);
}

void TstSqlReadOnly::cleanupTestCase()
{
	m_db.close();
	m_db = QSqlDatabase();
	QSqlDatabase::removeDatabase(QStringLiteral("tst_sqlreadonly"));
}

void TstSqlReadOnly::acceptsOrdinaryReads()
{
	QVERIFY(isAccepted("SELECT * FROM element"));
	QVERIFY(isAccepted("SELECT uuid FROM element WHERE uuid = 'a'"));
	// A semicolon inside a string literal is not a second statement.
	QVERIFY(isAccepted("SELECT ';' AS semicolon"));
	// One trailing semicolon is ordinary punctuation, not a second statement.
	QVERIFY(isAccepted("SELECT * FROM element;"));

	// And the rows come back: the returned query is the one that ran.
	QSqlQuery q = QETSql::execReadOnly(m_db, "SELECT uuid FROM element ORDER BY uuid");
	QStringList uuids;
	while (q.next()) {
		uuids << q.value(0).toString();
	}
	QCOMPARE(uuids, QStringList({"a", "b"}));
}

void TstSqlReadOnly::acceptsLegitimateCommonTableExpression()
{
	// WITH must keep working -- the fix is not "ban CTEs".
	QVERIFY(isAccepted("WITH x AS (SELECT 1 AS n) SELECT n FROM x"));
	QVERIFY(isAccepted(
		"WITH RECURSIVE c(n) AS (SELECT 1 UNION ALL SELECT n+1 FROM c) "
		"SELECT n FROM c LIMIT 3"));
}

void TstSqlReadOnly::refusesCtePrefixedWrites_data()
{
	QTest::addColumn<QString>("query");
	QTest::newRow("delete") << "WITH x AS (SELECT 1) DELETE FROM element";
	QTest::newRow("update") << "WITH x AS (SELECT 1) UPDATE element SET uuid = 'pwned'";
	QTest::newRow("insert") << "WITH x AS (SELECT 1) INSERT INTO element VALUES ('injected')";
	QTest::newRow("returning") << "WITH x AS (SELECT 1) DELETE FROM element RETURNING uuid";
}

void TstSqlReadOnly::refusesCtePrefixedWrites()
{
	QFETCH(QString, query);
	QVERIFY2(!isAccepted(query),
		 qPrintable(QStringLiteral("accepted a write: %1").arg(query)));
	QCOMPARE(rowCount(), 2);
}

void TstSqlReadOnly::refusesBareWrites_data()
{
	QTest::addColumn<QString>("query");
	QTest::newRow("delete") << "DELETE FROM element";
	QTest::newRow("update") << "UPDATE element SET uuid = 'pwned'";
	QTest::newRow("insert") << "INSERT INTO element VALUES ('injected')";
	QTest::newRow("drop")   << "DROP TABLE element";
	QTest::newRow("create") << "CREATE TABLE injected (x)";
	QTest::newRow("temp")   << "CREATE TEMP TABLE injected (x)";
}

void TstSqlReadOnly::refusesBareWrites()
{
	QFETCH(QString, query);
	QVERIFY2(!isAccepted(query),
		 qPrintable(QStringLiteral("accepted a write: %1").arg(query)));
	QCOMPARE(rowCount(), 2);
}

void TstSqlReadOnly::refusesTrailingStatement()
{
	QVERIFY(!isAccepted("SELECT 1; DROP TABLE element"));
	QVERIFY(!isAccepted("SELECT 1; SELECT 2"));
	QCOMPARE(rowCount(), 2);
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
	QFETCH(QString, query);
	QVERIFY(!isAccepted(query));
}

void TstSqlReadOnly::reportsAReason()
{
	QString reason;
	QVERIFY(!isAccepted("WITH x AS (SELECT 1) DELETE FROM element", &reason));
	QVERIFY2(!reason.isEmpty(), "a refusal must say why");

	reason = QStringLiteral("stale");
	QVERIFY(isAccepted("SELECT * FROM element", &reason));
	QVERIFY2(reason.isEmpty(), "an accepted query must not leave a reason behind");
}

void TstSqlReadOnly::doesNotExecuteWhatItRefuses()
{
	// Proving the table is untouched afterwards is what says the write was
	// refused rather than run -- and this same assertion goes red if the
	// refusals above ever stop refusing.
	QCOMPARE(rowCount(), 2);
	QVERIFY(!isAccepted("WITH x AS (SELECT 1) DELETE FROM element"));
	QVERIFY(!isAccepted("DELETE FROM element"));
	QCOMPARE(rowCount(), 2);
}

void TstSqlReadOnly::refusedQueryCannotBeRunAgain()
{
	// Several callers of projectDataBase::newQuery() call exec() again on
	// what it returns, and query_only is off by then. A refused query
	// must therefore come back with nothing left to run.
	QSqlQuery q = QETSql::execReadOnly(m_db, "WITH x AS (SELECT 1) DELETE FROM element");
	QVERIFY(!q.exec());
	QCOMPARE(rowCount(), 2);
}

void TstSqlReadOnly::acceptedQueryRunAgainStillReads()
{
	QSqlQuery q = QETSql::execReadOnly(m_db, "SELECT uuid FROM element");
	QVERIFY(q.exec());
	int n = 0;
	while (q.next()) {
		++n;
	}
	QCOMPARE(n, 2);
}

void TstSqlReadOnly::leavesTheConnectionWritable()
{
	// query_only must not outlive the call, whatever its outcome: the
	// project database is rebuilt by writes on this same connection.
	QETSql::execReadOnly(m_db, "SELECT * FROM element");
	QETSql::execReadOnly(m_db, "DELETE FROM element");
	QETSql::execReadOnly(m_db, "not even sql");

	QSqlQuery q(m_db);
	QVERIFY(q.exec(QStringLiteral("PRAGMA query_only")));
	QVERIFY(q.next());
	QCOMPARE(q.value(0).toInt(), 0);

	QVERIFY(q.exec(QStringLiteral("INSERT INTO element VALUES ('c')")));
	QCOMPARE(rowCount(), 3);
	QVERIFY(q.exec(QStringLiteral("DELETE FROM element WHERE uuid = 'c'")));
	QCOMPARE(rowCount(), 2);
}

QTEST_GUILESS_MAIN(TstSqlReadOnly)
#include "tst_sqlreadonly.moc"
