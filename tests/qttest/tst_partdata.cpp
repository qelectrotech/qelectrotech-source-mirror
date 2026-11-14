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
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "materiallist/materialentrydialog.h"
#include "materiallist/materiallist.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetinformation.h"
#include "qetproject.h"
#include "qetresult.h"

#include <QDomDocument>
#include <QApplication>
#include <QFile>
#include <QLineEdit>
#include <QTimer>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

/**
	Part data (discussion #1878, A3): a symbol's unit price and labour
	time, beside its width, height and depth. Each is element information
	like the order number: kept by .qet and .qetdb, filled from the
	material file, and a column of the parts list. A project which never
	sets them saves as before.
*/
class tst_partdata : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	static Element *symbol(Diagram *diagram, QPointF at)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		if (!collection->exist(QStringLiteral("import/part.elmt"))) {
			QDomDocument definition;
			definition.setContent(QString(
				R"(<definition type="element" link_type="simple" uuid="%1" width="40" height="60" hotspot_x="20" hotspot_y="30">
<names><name lang="en">part</name></names>
<description><line x1="-20" y1="-30" x2="20" y2="-30"/>
<terminal x="0" y="30" orientation="s" uuid="%2"/></description></definition>)")
				.arg(QUuid::createUuid().toString(), QUuid::createUuid().toString()));
			if (!collection->addElementDefinition(QStringLiteral("import"), QStringLiteral("part"),
												  definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/part.elmt"), diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		element->setPos(at);
		return element;
	}

		//-K1 with every part data field, -K2 with none
	static void twoParts(QETProject &project)
	{
		Diagram *folio = project.addNewDiagram();
		Element *k1 = symbol(folio, {100, 100});
		Element *k2 = symbol(folio, {300, 100});
		QVERIFY(k1 && k2);
		DiagramContext info = k1->elementInformations();
		info.addValue(QStringLiteral("label"), QStringLiteral("-K1"));
		info.addValue(QStringLiteral("manufacturer_reference"), QStringLiteral("3RT2016"));
		info.addValue(QStringLiteral("width"), QStringLiteral("45"));
		info.addValue(QStringLiteral("height"), QStringLiteral("57.5"));
		info.addValue(QStringLiteral("depth"), QStringLiteral("73"));
		info.addValue(QStringLiteral("unit_price"), QStringLiteral("38.90"));
		info.addValue(QStringLiteral("labour_time"), QStringLiteral("0.25"));
		k1->setElementInformations(info);
		info = k2->elementInformations();
		info.addValue(QStringLiteral("label"), QStringLiteral("-K2"));
		k2->setElementInformations(info);
	}

		//"label|width|height|depth|unit_price|labour_time" of each row of
		//the parts list of @p project
	static QStringList partsList(QETProject &project)
	{
		project.dataBase()->updateDB();
		QString error;
		QSqlQuery query = project.dataBase()->newQuery(QStringLiteral(
			"SELECT label, width, height, depth, unit_price, labour_time "
			"FROM element_nomenclature_view ORDER BY label"), &error);
		if (!error.isEmpty()) return {error};
		QStringList rows;
		while (query.next()) {
			QStringList row;
			for (int i = 0 ; i < 6 ; ++i) row << query.value(i).toString();
			rows << row.join(QLatin1Char('|'));
		}
		return rows;
	}

	static QStringList expected()
	{
		return {QStringLiteral("-K1|45|57.5|73|38.90|0.25"),
				QStringLiteral("-K2|||||")};
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
	}

	void partsListHasThePartData()
	{
		QETProject project;
		twoParts(project);
		QCOMPARE(partsList(project), expected());
	}

		//Every field the parts list export offers to pick
		//(ElementQueryWidget::setUpItems()) is a column it can read:
		//width, height, depth and the cross-reference were offered and
		//made the export fail with "no such column"
	void everyOfferedFieldIsAColumn()
	{
		QETProject project;
		twoParts(project);
		project.dataBase()->updateDB();
		for (const QString &key : QETInformation::elementInfoKeys()) {
			if (key == QLatin1String("formula") || key == QLatin1String("exclude_from_bom")
					|| key == QETInformation::ELMT_TERMINAL_NUMBER)
				continue;
			QString error;
			QSqlQuery query = project.dataBase()->newQuery(
				QStringLiteral("SELECT %1 FROM element_nomenclature_view").arg(key), &error);
			QVERIFY2(error.isEmpty(), qPrintable(key + QStringLiteral(": ") + error));
		}
	}

	void projectKeepsThePartData_data()
	{
		QTest::addColumn<QString>("file");
		QTest::newRow("qet") << QStringLiteral("parts.qet");
		QTest::newRow("qetdb") << QStringLiteral("parts.qetdb");
	}

	void projectKeepsThePartData()
	{
		QFETCH(QString, file);
		QETProject project;
		twoParts(project);
		project.setFilePath(m_dir.filePath(file));
		QVERIFY(project.write().isOk());
		QETProject again(m_dir.filePath(file));
		QCOMPARE(partsList(again), expected());
	}

		//A symbol without them writes nothing of them
	void notSetIsNotWritten()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Element *k2 = symbol(folio, {300, 100});
		QVERIFY(k2);
		project.setFilePath(m_dir.filePath(QStringLiteral("none.qet")));
		QVERIFY(project.write().isOk());
		QFile saved(m_dir.filePath(QStringLiteral("none.qet")));
		QVERIFY(saved.open(QIODevice::ReadOnly));
		const QByteArray xml = saved.readAll();
		QVERIFY(!xml.contains("unit_price"));
		QVERIFY(!xml.contains("labour_time"));
	}

		//The material file carries them like the dimensions: they belong
		//to the article, so picking another article takes them away
	void materialFileCarriesThem()
	{
		for (const QString &column : {QStringLiteral("unit_price"), QStringLiteral("labour_time")}) {
			QVERIFY(MaterialList::columnsForBlock(0).contains(column));
			QVERIFY(MaterialList::defaultColumns().contains(column));
			QCOMPARE(MaterialList::elementInfoKey(column, 0), column);
			QVERIFY(MaterialList::isArticleBound(column));
			QVERIFY(QETInformation::isNumericInfoKey(column));
			QVERIFY(!QETInformation::translatedInfoKey(column).isEmpty());
		}
	}

		//A size takes two decimals; a price or a time takes four, for a
		//part bought by the reel or a job of a few minutes. "12." is
		//unfinished, which the material entry dialog refuses to save
	void numbersTakeTheirDecimals_data()
	{
		QTest::addColumn<QString>("key");
		QTest::addColumn<QString>("value");
		QTest::addColumn<int>("state");
		const int ok = QValidator::Acceptable, not_ok = QValidator::Invalid,
				unfinished = QValidator::Intermediate;
		QTest::newRow("width 57.5") << "width" << "57.5" << ok;
		QTest::newRow("width 57.125") << "width" << "57.125" << not_ok;
		QTest::newRow("price 0.0045") << "unit_price" << "0.0045" << ok;
		QTest::newRow("price 38,90") << "unit_price" << "38,90" << ok;
		QTest::newRow("price 0.00451") << "unit_price" << "0.00451" << not_ok;
		QTest::newRow("price 12.") << "unit_price" << "12." << unfinished;
		QTest::newRow("labour 0.125") << "labour_time" << "0.125" << ok;
		QTest::newRow("labour 1h") << "labour_time" << "1h" << not_ok;
	}

	void numbersTakeTheirDecimals()
	{
		QFETCH(QString, key);
		QFETCH(QString, value);
		QFETCH(int, state);
		QETInformation::NumericInfoValidator validator(key);
		int pos = 0;
		QCOMPARE(int(validator.validate(value, pos)), state);
	}

		//A new article with a number left unfinished is not saved
	void entryRefusesAnUnfinishedNumber()
	{
		MaterialEntryDialog dialog({QStringLiteral("unit_price")});
		auto *edit = dialog.findChild<QLineEdit *>();
		QVERIFY(edit);
		bool warned = false;
		const auto closeWarning = [&warned] {
			if (QWidget *box = QApplication::activeModalWidget()) {
				warned = true;
				box->close();
			}
		};
		edit->setText(QStringLiteral("12."));
		QTimer::singleShot(0, &dialog, closeWarning);
		dialog.accept();
		QVERIFY(warned);
		QCOMPARE(dialog.result(), int(QDialog::Rejected));

		edit->setText(QStringLiteral("12.5"));
		dialog.accept();
		QCOMPARE(dialog.result(), int(QDialog::Accepted));
		QCOMPARE(dialog.record().values.value(QStringLiteral("unit_price")), QStringLiteral("12.5"));
	}
};

QET_APP_TEST_MAIN(tst_partdata)
#include "tst_partdata.moc"
