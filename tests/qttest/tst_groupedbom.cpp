// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "bomexport.h"

// BomExport::groupedQuery() on a table shaped like element_nomenclature_view:
// identical parts become one line with the quantities added up.
class tst_groupedbom : public QObject
{
	Q_OBJECT

	QSqlDatabase m_db;

	void add(const QString &label, const QString &manufacturer, const QString &article,
			 const QString &designation, const QString &quantity = QString(),
			 const QString &aux_article = QString())
	{
		QSqlQuery q(m_db);
		q.prepare(QStringLiteral(
			"INSERT INTO element_nomenclature_view (label, manufacturer, manufacturer_reference, "
			"designation, quantity, manufacturer_reference_auxiliary1) VALUES (?,?,?,?,?,?)"));
		for (const QString &v : {label, manufacturer, article, designation, quantity, aux_article})
			q.addBindValue(v);
		QVERIFY(q.exec());
	}

		// The costing columns are added only by the tests which need them:
		// the others prove a list without costing never reads them.
	void addCostingColumns()
	{
		QSqlQuery q(m_db);
		QVERIFY(q.exec(QStringLiteral("ALTER TABLE element_nomenclature_view ADD COLUMN unit_price")));
		QVERIFY(q.exec(QStringLiteral("ALTER TABLE element_nomenclature_view ADD COLUMN labour_time")));
	}

	void addPriced(const QString &label, const QString &article, const QString &price,
				   const QString &labour, const QString &quantity = QString(),
				   const QString &aux_article = QString())
	{
		add(label, "ABB", article, "Part " + article, quantity, aux_article);
		QSqlQuery q(m_db);
		q.prepare(QStringLiteral("UPDATE element_nomenclature_view SET unit_price = ?, labour_time = ? "
								 "WHERE rowid = (SELECT MAX(rowid) FROM element_nomenclature_view)"));
		q.addBindValue(price);
		q.addBindValue(labour);
		QVERIFY(q.exec());
	}

	QList<QStringList> run(const BomExport::GroupedOptions &options, QStringList *columns = nullptr)
	{
		QSqlQuery q(m_db);
		if (!q.exec(BomExport::groupedQuery(options))) {
			qWarning() << q.lastError().text();
			return {};
		}
		if (columns) {
			columns->clear();
			for (int i = 0; i < q.record().count(); ++i)
				columns->append(q.record().fieldName(i));
		}
		QList<QStringList> rows;
		while (q.next()) {
			QStringList row;
			for (int i = 0; i < q.record().count(); ++i)
				row << q.value(i).toString();
			rows << row;
		}
		return rows;
	}

	private slots:
		void init()
		{
			m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("groupedbom"));
			m_db.setDatabaseName(QStringLiteral(":memory:"));
			QVERIFY(m_db.open());
				// a table is enough: the query only reads these columns
			QSqlQuery q(m_db);
			QVERIFY(q.exec(QStringLiteral(
				"CREATE TABLE element_nomenclature_view (label, manufacturer, manufacturer_reference, "
				"designation, description, quantity, unity, "
				"manufacturer_auxiliary1, manufacturer_reference_auxiliary1, designation_auxiliary1, "
				"description_auxiliary1, quantity_auxiliary1, unity_auxiliary1, "
				"manufacturer_auxiliary2, manufacturer_reference_auxiliary2, designation_auxiliary2, "
				"description_auxiliary2, quantity_auxiliary2, unity_auxiliary2, "
				"manufacturer_auxiliary3, manufacturer_reference_auxiliary3, designation_auxiliary3, "
				"description_auxiliary3, quantity_auxiliary3, unity_auxiliary3, "
				"manufacturer_auxiliary4, manufacturer_reference_auxiliary4, designation_auxiliary4, "
				"description_auxiliary4, quantity_auxiliary4, unity_auxiliary4)")));
		}

		void cleanup()
		{
			m_db.close();
			m_db = QSqlDatabase();
			QSqlDatabase::removeDatabase(QStringLiteral("groupedbom"));
		}

		void samePartIsOneLine()
		{
			add("K1", "ABB", "A9", "Contactor");
			add("K2", "ABB", "A9", "Contactor");
			add("F1", "ABB", "B1", "Breaker");
			BomExport::GroupedOptions o;
			QStringList columns;
			const auto rows = run(o, &columns);
			QCOMPARE(columns, (QStringList{"number", "quantity", "name", "part_number",
										   "references", "manufacturer", "unit"}));
			QCOMPARE(rows.size(), 2);
			QCOMPARE(rows[0], (QStringList{"1", "2", "Contactor", "A9", "K1,K2", "ABB", ""}));
			QCOMPARE(rows[1][1], QStringLiteral("1"));
		}

		void strayBlanksDoNotSplitAPart()
		{
			add("K1", "ABB", "A9", "Contactor");
			add("K2", " ABB ", "A9 ", "Contactor");
			BomExport::GroupedOptions o;
			QCOMPARE(run(o).size(), 1);
		}

		void quantityFieldIsAddedUp()
		{
			add("W1", "", "CAB3", "Cable", "2,5");
			add("W2", "", "CAB3", "Cable", "4");
			BomExport::GroupedOptions o;
			QCOMPARE(run(o).first()[1], QStringLiteral("6.5"));
			o.sum_quantity = false;
			QCOMPARE(run(o).first()[1], QStringLiteral("2"));
		}

		void noPartNumberIsNeverMerged()
		{
			add("X1", "", "", "Terminal");
			add("X1", "", "", "Terminal");
			add("X2", "", "", "Terminal");
			BomExport::GroupedOptions o;
			QCOMPARE(run(o).size(), 3);
		}

		void rowsWithNothingToOrderStayOut()
		{
			add("J1", "", "", "");
			add("K1", "ABB", "A9", "Contactor");
			BomExport::GroupedOptions o;
			QCOMPARE(run(o).size(), 1);
		}

		void auxiliaryBlocksAreParts()
		{
			add("QF1", "ABB", "B1", "Breaker", "", "AUX9");
			BomExport::GroupedOptions o;
			QCOMPARE(run(o).size(), 2);
			o.auxiliaries = false;
			QCOMPARE(run(o).size(), 1);
		}

		void columnsFollowTheOptions()
		{
			add("K1", "ABB", "A9", "Contactor");
			BomExport::GroupedOptions o;
			o.references = false;
			o.numbering = false;
			QStringList columns;
			run(o, &columns);
			QCOMPARE(columns, (QStringList{"quantity", "name", "part_number", "manufacturer", "unit"}));
		}
		void costingColumnsAndTotalLine()
		{
			addCostingColumns();
			BomExport::GroupedOptions o;
			o.costing = true;
			o.total_label = QStringLiteral("All 'parts'");
			QStringList columns;
			run(o, &columns);
			QCOMPARE(columns, (QStringList{"number", "quantity", "name", "part_number",
										   "references", "manufacturer", "unit",
										   "unit_price", "total_price", "labour_time", "total_labour"}));
			o.numbering = false;
			o.references = false;
			const auto rows = run(o, &columns);
			QCOMPARE(columns, (QStringList{"quantity", "name", "part_number", "manufacturer", "unit",
										   "unit_price", "total_price", "labour_time", "total_labour"}));
				// no part at all: still the last line, adding up to nothing
			QCOMPARE(rows.size(), 1);
			QCOMPARE(rows[0], (QStringList{"", "All 'parts'", "", "", "", "", "0", "", "0"}));
		}

			// The hand count: 2 x A9 at 12.5 (0.25 h) = 25 (0.5 h),
			// B1 quantity 2 at 30 (0.5 h) = 60 (1 h), X1 has no price.
			// Project: 85 and 1.5 h.
		void totalsMatchAHandCount()
		{
			addCostingColumns();
			addPriced("K1", "A9", "12.5", "0.25");
			addPriced("K2", "A9", "12.5", "0.25");
			addPriced("F1", "B1", "30", "0.5", "2");
			add("X1", "", "", "Terminal");
			BomExport::GroupedOptions o;
			o.costing = true;
			const auto rows = run(o);
			QCOMPARE(rows.size(), 4);
			QCOMPARE(rows[0], (QStringList{"1", "1", "Terminal", "", "X1", "", "", "", "", "", ""}));
			QCOMPARE(rows[1], (QStringList{"2", "2", "Part A9", "A9", "K1,K2", "ABB", "",
										   "12.5", "25", "0.25", "0.5"}));
			QCOMPARE(rows[2], (QStringList{"3", "2", "Part B1", "B1", "F1", "ABB", "",
										   "30", "60", "0.5", "1"}));
			QCOMPARE(rows[3], (QStringList{"", "", "Total", "", "", "", "", "", "85", "", "1.5"}));
		}

		void elementsPricedDifferentlyStillAddUp()
		{
			addCostingColumns();
			addPriced("K1", "A9", "10", "");
			addPriced("K2", "A9", "12,5", "");
			BomExport::GroupedOptions o;
			o.costing = true;
			const auto rows = run(o);
				// no single unit price to show, but the total is right
			QCOMPARE(rows[0].mid(7), (QStringList{"", "22.5", "", ""}));
			QCOMPARE(rows[1][8], QStringLiteral("22.5"));
		}

		void textThatIsNoNumberIsNoPrice()
		{
			addCostingColumns();
			addPriced("K1", "A9", "€12", "");
			addPriced("K2", "B1", "1,234.50", "");
			addPriced("K3", "C1", "7", "");
			BomExport::GroupedOptions o;
			o.costing = true;
			const auto rows = run(o);
				// neither reads as 0 or 1.234: no price, and the total is K3's
			QCOMPARE(rows[0].mid(7, 2), (QStringList{"", ""}));
			QCOMPARE(rows[1].mid(7, 2), (QStringList{"", ""}));
			QCOMPARE(rows[2].mid(7, 2), (QStringList{"7", "7"}));
			QCOMPARE(rows[3][8], QStringLiteral("7"));
		}

		void countedRowsCostOnceEach()
		{
			addCostingColumns();
			addPriced("W1", "CAB3", "2", "", "10");
			BomExport::GroupedOptions o;
			o.costing = true;
			QCOMPARE(run(o)[0][8], QStringLiteral("20"));
			o.sum_quantity = false;
			QCOMPARE(run(o)[0][8], QStringLiteral("2"));
		}

		void smallPricesKeepTheirDecimals()
		{
			addCostingColumns();
			addPriced("R1", "R10K", "0.0045", "0.0001", "100");
			BomExport::GroupedOptions o;
			o.costing = true;
			const auto rows = run(o);
			QCOMPARE(rows[0].mid(7), (QStringList{"0.0045", "0.45", "0.0001", "0.01"}));
		}

		void auxiliaryBlocksHaveNoPrice()
		{
			addCostingColumns();
			addPriced("QF1", "B1", "30", "0.5", "", "AUX9");
			BomExport::GroupedOptions o;
			o.costing = true;
			const auto rows = run(o);
			QCOMPARE(rows.size(), 3);
			QCOMPARE(rows[0][3], QStringLiteral("AUX9"));
			QCOMPARE(rows[0][8], QString());
			QCOMPARE(rows[2][8], QStringLiteral("30"));
		}
};

QTEST_GUILESS_MAIN(tst_groupedbom)
#include "tst_groupedbom.moc"
