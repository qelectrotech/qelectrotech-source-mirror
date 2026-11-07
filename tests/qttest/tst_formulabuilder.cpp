// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include "formulabuilder.h"

// The formulas of a table column are compiled to SQLite. Each case here runs
// the compiled expression on real rows, so what is checked is the result the
// table shows, not the shape of the text.
using namespace FormulaBuilder;

class tst_formulabuilder : public QObject
{
	Q_OBJECT

	const QList<Field> m_fields{{"Description", "description"}, {"Designation", "designation"},
								{"Quantity", "quantity"}, {"Reference", "reference"}};
	QSqlDatabase m_db;

	// The value of @p node on row @p row (0 based), compiled with the fields above
	QVariant run(const QVariantMap &node, int row = 0)
	{
		QString error;
		const QString sql = compile(node, m_fields, &error);
		if (sql.isEmpty()) return QStringLiteral("COMPILE ERROR: ") + error;
		QSqlQuery query(m_db);
		if (!query.exec("SELECT " + sql + " FROM parts ORDER BY rowid LIMIT 1 OFFSET " + QString::number(row)))
			return QStringLiteral("SQL ERROR: ") + query.lastError().text();
		return query.next() ? query.value(0) : QVariant(QStringLiteral("NO ROW"));
	}

private slots:
	void initTestCase()
	{
		m_db = QSqlDatabase::addDatabase("QSQLITE", "formulabuilder");
		m_db.setDatabaseName(":memory:");
		QVERIFY(m_db.open());
		QSqlQuery query(m_db);
		QVERIFY(query.exec("CREATE TABLE parts(description TEXT, designation TEXT, quantity TEXT, reference TEXT)"));
		// 0: ordinary row, 1: absent description, 2: blank description
		QVERIFY(query.exec("INSERT INTO parts VALUES(' Door''s sensor ','BNS-260','2','-S1'),"
						   "(NULL,'Relay','3','-K1'),('   ','Actuator','1','-S2')"));
	}

	void cleanupTestCase()
	{
		m_db.close();
		m_db = QSqlDatabase();
		QSqlDatabase::removeDatabase("formulabuilder");
	}

	void textFunctions()
	{
		const auto description = field("description"), designation = field("designation"), reference = field("reference");
		QCOMPARE(run(operation("trim", {description})).toString(), QStringLiteral("Door's sensor"));
		QCOMPARE(run(operation("upper", {designation}), 1).toString(), QStringLiteral("RELAY"));
		QCOMPARE(run(operation("lower", {designation})).toString(), QStringLiteral("bns-260"));
		QCOMPARE(run(operation("join", {operation("trim", {description}), reference}, {{"separator", " / "}})).toString(),
				 QStringLiteral("Door's sensor / -S1"));
		// punctuation in a typed text is quoted, never read as SQL
		QCOMPARE(run(operation("replace", {description, text("sensor"), text("x'; DROP TABLE parts; --")})).toString(),
				 QStringLiteral(" Door's x'; DROP TABLE parts; -- "));
		QCOMPARE(run(operation("replace", {description, text("sensor"), text("")})).toString().trimmed(),
				 QStringLiteral("Door's"));
	}

	void emptyAndFallback()
	{
		const auto description = field("description"), designation = field("designation");
		for (int row = 1 ; row <= 2 ; ++row)   // absent, then blank
			QVERIFY2(run(operation("empty", {description}), row).toBool(), qPrintable(QString::number(row)));
		QVERIFY(!run(operation("empty", {description}), 0).toBool());
		QVERIFY(run(operation("notempty", {description}), 0).toBool());
		QCOMPARE(run(operation("fallback", {description, designation}), 1).toString(), QStringLiteral("Relay"));
		QCOMPARE(run(operation("fallback", {description, designation}), 2).toString(), QStringLiteral("Actuator"));
		QCOMPARE(run(operation("fallback", {description, designation}), 0).toString(), QStringLiteral(" Door's sensor "));
	}

	void textConditions()
	{
		const auto designation = field("designation");
		QVERIFY(run(operation("contains", {designation, text("ns-2")})).toBool());   // case does not matter
		QVERIFY(!run(operation("contains", {designation, text("xyz")})).toBool());
		QVERIFY(run(operation("contains", {designation, text("%")})).toBool() == false);   // % is an ordinary character
		QVERIFY(run(operation("starts", {designation, text("bns")})).toBool());
		QVERIFY(!run(operation("starts", {designation, text("260")})).toBool());
		QVERIFY(run(operation("ends", {designation, text("260")})).toBool());
		QVERIFY(!run(operation("ends", {designation, text("BNS")})).toBool());
	}

	void numbersAndComparisons()
	{
		const auto quantity = field("quantity");
		QVERIFY(run(operation("eq", {quantity, number(2)})).toBool());   // a text column, compared as a number
		const auto calculatedTwo = operation("multiply", {number(1), number(2)});
		QVERIFY(run(operation("eq", {quantity, calculatedTwo})).toBool());
		QVERIFY(!run(operation("ne", {quantity, calculatedTwo})).toBool());
		// two plain texts stay texts: 002 is not 2
		QVERIFY(!run(operation("eq", {text("002"), text("2")})).toBool());
		QVERIFY(run(operation("gt", {quantity, number(1)})).toBool());
		QVERIFY(!run(operation("gt", {quantity, number(10)})).toBool());   // not a comparison of texts
		QVERIFY(run(operation("ge", {quantity, number(2)})).toBool());
		QVERIFY(run(operation("le", {quantity, number(2)})).toBool());
		QVERIFY(run(operation("lt", {quantity, number(3)})).toBool());
		QVERIFY(run(operation("between", {quantity, number(1), number(2)})).toBool());
		QVERIFY(!run(operation("between", {quantity, number(3), number(4)})).toBool());
		QCOMPARE(run(operation("add", {quantity, number(1)})).toDouble(), 3.0);
		QCOMPARE(run(operation("subtract", {quantity, number(1)})).toDouble(), 1.0);
		QCOMPARE(run(operation("multiply", {quantity, number(5)})).toDouble(), 10.0);
		QCOMPARE(run(operation("divide", {quantity, number(4)})).toDouble(), 0.5);
		QCOMPARE(run(operation("round", {operation("divide", {number(10), number(3)}), number(2)})).toDouble(), 3.33);
	}

	void divisionByZeroIsEmpty()
	{
		const QVariant value = run(operation("divide", {field("quantity"), number(0)}));
		QVERIFY2(value.isNull(), qPrintable(value.toString()));
	}

	void decimalCommaIsRead()
	{
		QSqlQuery query(m_db);
		QVERIFY(query.exec("INSERT INTO parts VALUES('x','y','2,5','z')"));
		QCOMPARE(run(operation("multiply", {field("quantity"), number(2)}), 3).toDouble(), 5.0);
	}

	void logic()
	{
		const auto description = field("description"), designation = field("designation");
		const auto cond = operation("empty", {description});
		QCOMPARE(run(operation("if", {cond, text("none"), designation}), 1).toString(), QStringLiteral("none"));
		QCOMPARE(run(operation("if", {cond, text("none"), designation}), 0).toString(), QStringLiteral("BNS-260"));
		QVERIFY(run(operation("and", {operation("notempty", {description}), operation("contains", {designation, text("bns")})})).toBool());
		QVERIFY(!run(operation("and", {operation("notempty", {description}), operation("contains", {designation, text("zzz")})})).toBool());
		QVERIFY(run(operation("or", {operation("empty", {description}), operation("contains", {designation, text("bns")})})).toBool());
		QVERIFY(run(operation("not", {cond}), 0).toBool());
		// a result which is a number is compared as one
		QVERIFY(run(operation("eq", {field("quantity"),
									 operation("if", {operation("notempty", {description}), number(2), number(0)})})).toBool());
	}

	void invalidFormulasSayWhy()
	{
		QString error;
		QVERIFY(compile(field("nonexistent"), m_fields, &error).isEmpty());
		QVERIFY(!error.isEmpty());
		error.clear();
		QVERIFY(compile(operation("upper", {}), m_fields, &error).isEmpty());   // an input is missing
		QVERIFY(!error.isEmpty());
		error.clear();
		QVERIFY(compile(operation("bogus"), m_fields, &error).isEmpty());
		QVERIFY(!error.isEmpty());
		// a field is only what the table lists: SQL typed as one is refused
		QVERIFY(compile(field("1); DROP TABLE parts; --"), m_fields).isEmpty());
		// too deep
		QVariantMap deep = field("description");
		for (int i = 0 ; i < 50 ; ++i)
			deep = operation("upper", {deep});
		error.clear();
		QVERIFY(compile(deep, m_fields, &error).isEmpty());
		QVERIFY(!error.isEmpty());
	}

	void defaultNodesCompile()
	{
		for (const Function &f : functions()) {
			QString error;
			QVERIFY2(!compile(defaultNode(f.id, m_fields), m_fields, &error).isEmpty(),
					 qPrintable(f.id + ": " + error));
		}
	}
};

QTEST_GUILESS_MAIN(tst_formulabuilder)
#include "tst_formulabuilder.moc"
