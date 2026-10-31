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
};

QTEST_GUILESS_MAIN(tst_groupedbom)
#include "tst_groupedbom.moc"
