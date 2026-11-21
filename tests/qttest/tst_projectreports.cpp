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
#include "dataBase/projectdatabase.h"
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "reports/projectreports.h"
#include "ui/plantslocationsdialog.h"
#include "ui/reportsdialog.h"

#include <QComboBox>
#include <QDomDocument>
#include <QFile>
#include <QLabel>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QSqlQuery>
#include <QtTest>

/**
	The project's lists: each a query of the public views, for the whole
	project or a place, written to CSV and PDF.
*/
class tst_projectreports : public QObject
{
	Q_OBJECT
	QTemporaryDir m_dir;
	int m_position = 0;

	Element *symbol(Diagram *diagram, const QString &label)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		if (!collection->exist(QStringLiteral("import/simple.elmt"))) {
			QDomDocument definition;
			if (!definition.setContent(QString(
					R"(<definition type="element" link_type="simple" uuid="%1" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test</name></names>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" name="1" uuid="%2"/><terminal x="0" y="20" orientation="s" name="2" uuid="%3"/>
</description></definition>)")
					.arg(QUuid::createUuid().toString(), QUuid::createUuid().toString(),
						 QUuid::createUuid().toString())))
				return nullptr;
			if (!collection->addElementDefinition(QStringLiteral("import"), QStringLiteral("simple"),
												  definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/simple.elmt"), diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		element->setPos(100 + 80 * m_position++, 150);
		auto info = element->elementInformations();
		info.addValue(QStringLiteral("label"), label);
		info.remove(QStringLiteral("formula"));
		element->setElementInformations(info);
		return element;
	}

	static void setPlace(Diagram *diagram, const QString &plant, const QString &location)
	{
		auto block = diagram->border_and_titleblock.exportTitleBlock();
		block.plant = plant;
		block.locmach = location;
		diagram->border_and_titleblock.importTitleBlock(block);
	}

	static void wire(Diagram *diagram, Element *a, Element *b, const QString &number,
					 const QString &section = QString(), const QString &colour = QString())
	{
		auto *conductor = new Conductor(a->terminals().at(1), b->terminals().at(0));
		diagram->addItem(conductor);
		ConductorProperties properties = conductor->properties();
		properties.text = number;
		properties.m_wire_section = section;
		properties.m_wire_color = colour;
		conductor->setProperties(properties);
	}

		//Two places, a wire in each
	void build(QETProject &project)
	{
		Diagram *factory = project.addNewDiagram();
		Diagram *silo = project.addNewDiagram();
		setPlace(factory, QStringLiteral("FACTORY"), QStringLiteral("CP1"));
		setPlace(silo, QStringLiteral("SILO"), QStringLiteral("P_1"));
		Element *k1 = symbol(factory, QStringLiteral("K1"));
		Element *k2 = symbol(factory, QStringLiteral("K2"));
		Element *s1 = symbol(silo, QStringLiteral("S1"));
		Element *s2 = symbol(silo, QStringLiteral("S2"));
		QVERIFY(k1 && k2 && s1 && s2);
		wire(factory, k1, k2, QStringLiteral("101"), QStringLiteral("2.5mm"), QStringLiteral("Y/GN"));
		wire(silo, s1, s2, QStringLiteral("9"));
	}

	static QStringList column(const ProjectReports::Table &table, int index)
	{
		QStringList values;
		for (const QStringList &row : table.rows) values << row.at(index);
		return values;
	}

  private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QETProject::setBackupEnabled(false);
	}
	void init() {m_position = 0;}

	void everyReportRuns()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		for (const auto &report : ProjectReports::all()) {
			const auto table = ProjectReports::read(&project, report);
			QVERIFY2(table.error.isEmpty(), qPrintable(report.id + QLatin1Char(' ') + table.error));
			QCOMPARE(table.headers.size(), report.columns.size());
		}
	}

	void wiresWithSectionAndEnds()
	{
		QETProject project;
		build(project);
		const auto table = ProjectReports::read(&project, ProjectReports::report(QStringLiteral("wires")));
		QVERIFY2(table.error.isEmpty(), qPrintable(table.error));
		QCOMPARE(table.rows.size(), 2);
		QCOMPARE(table.rows.first(), QStringList({QStringLiteral("101"), QStringLiteral("2.5mm"),
												  QStringLiteral("Y/GN"), QStringLiteral("=FACTORY+CP1-K1:2"),
												  QStringLiteral("=FACTORY+CP1-K2:1"), QString(), QStringLiteral("1")}));
			//A terminal X5:1 with the pin 1 is X5:1, not X5:1:1
		Diagram *folio = project.folios().first();
		Element *x = symbol(folio, QStringLiteral("X5:1"));
		Element *k = symbol(folio, QStringLiteral("K3"));
		QVERIFY(x && k);
		auto *conductor = new Conductor(k->terminals().at(1), x->terminals().at(0));
		folio->addItem(conductor);
		const auto again = ProjectReports::read(&project, ProjectReports::report(QStringLiteral("wires")));
		QVERIFY(column(again, 4).contains(QStringLiteral("=FACTORY+CP1-X5:1")));
	}

	void limitedToAPlace()
	{
		QETProject project;
		build(project);
		const auto wires = ProjectReports::report(QStringLiteral("wires"));
		const auto parts = ProjectReports::report(QStringLiteral("parts"));
		const auto potentials = ProjectReports::report(QStringLiteral("potentials"));
		QCOMPARE(column(ProjectReports::read(&project, wires, {QStringLiteral("SILO"), {}}), 0),
				 QStringList({QStringLiteral("9")}));
			//An underscore is a letter here, not a wildcard
		QCOMPARE(column(ProjectReports::read(&project, parts, {QStringLiteral("SILO"), QStringLiteral("P_1")}), 0),
				 QStringList({QStringLiteral("=SILO+P_1-S1"), QStringLiteral("=SILO+P_1-S2")}));
		QVERIFY(ProjectReports::read(&project, parts, {QStringLiteral("SILO"), QStringLiteral("PX1")}).rows.isEmpty());
		const auto one = ProjectReports::read(&project, potentials, {QStringLiteral("FACTORY"), {}});
		QVERIFY2(one.error.isEmpty(), qPrintable(one.error));
		QCOMPARE(one.rows.size(), 1);
		QCOMPARE(one.rows.first().at(0), QStringLiteral("101"));
		QCOMPARE(one.rows.first().at(2), QStringLiteral("2"));
		QCOMPARE(one.rows.first().at(3), QStringLiteral("=FACTORY+CP1-K1:2, =FACTORY+CP1-K2:1"));

		const auto places = ProjectReports::places(&project);
		QStringList texts;
		for (const auto &place : places) texts << place.text();
		QCOMPARE(texts, QStringList({QStringLiteral("=FACTORY"), QStringLiteral("=FACTORY+CP1"),
									 QStringLiteral("=SILO"), QStringLiteral("=SILO+P_1")}));
	}

	void plantsAndLocationsWithCounts()
	{
		QETProject project;
		build(project);
			//A folio and a symbol with no plant and no location
		Diagram *loose = project.addNewDiagram();
		QVERIFY(symbol(loose, QStringLiteral("Q1")));
		const auto table = ProjectReports::locations(&project);
		QVERIFY2(table.error.isEmpty(), qPrintable(table.error));
		QCOMPARE(table.headers.size(), 6);
			//The screen is the view, row for row
		QSqlQuery view = project.dataBase()->newQuery(QStringLiteral(
				"SELECT COUNT(*) FROM qet_locations"));
		QVERIFY(view.next());
		QCOMPARE(table.rows.size(), view.value(0).toInt());
		QCOMPARE(table.rows.size(), 3);
			//Plant, location, designation, symbols, folios, first folio
		QCOMPARE(table.rows.at(0), QStringList({QString(), QString(), QString(),
												QStringLiteral("1"), QStringLiteral("1"), QStringLiteral("3")}));
		QCOMPARE(table.rows.at(1).mid(0, 2), QStringList({QStringLiteral("FACTORY"), QStringLiteral("CP1")}));
		QCOMPARE(table.rows.at(1).at(2), QStringLiteral("=FACTORY+CP1"));
		QCOMPARE(table.rows.at(1).mid(3), QStringList({QStringLiteral("2"), QStringLiteral("1"), QStringLiteral("1")}));
		QCOMPARE(table.rows.at(2).mid(3), QStringList({QStringLiteral("2"), QStringLiteral("1"), QStringLiteral("2")}));
	}

	void plantsAndLocationsDialog()
	{
		QETProject project;
		build(project);
		PlantsLocationsDialog dialog(&project);
		auto *table = dialog.findChild<QTableWidget *>(QStringLiteral("placesTable"));
		QVERIFY(table);
		QCOMPARE(table->rowCount(), 2);
		QCOMPARE(table->item(1, 0)->text(), QStringLiteral("SILO"));
			//Double-click asks for that place's first folio
		QSignalSpy spy(&dialog, &PlantsLocationsDialog::showFolio);
		table->itemDoubleClicked(table->item(1, 1));
		QCOMPARE(spy.count(), 1);
		QCOMPARE(spy.first().first().toInt(), 2);
			//An empty project is an empty table, not an error
		QETProject empty;
		PlantsLocationsDialog none(&empty);
		QCOMPARE(none.findChild<QTableWidget *>(QStringLiteral("placesTable"))->rowCount(), 0);
	}

	void writesCsv()
	{
		ProjectReports::Table table;
		table.headers = {QStringLiteral("Wire"), QStringLiteral("To")};
		table.rows = {{QStringLiteral("1"), QStringLiteral("a;b")}, {QStringLiteral("2"), QString()}};
		const QString path = m_dir.filePath(QStringLiteral("list.csv"));
		QVERIFY(ProjectReports::writeCsv(table, path));
		QFile file(path);
		QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
		QCOMPARE(QString::fromUtf8(file.readAll()), QStringLiteral("Wire;To\n1;\"a;b\"\n2;\n"));
	}

	void writesPdfOverSeveralPages()
	{
		ProjectReports::Table table;
		table.headers = {QStringLiteral("Wire"), QStringLiteral("Connected")};
		for (int i = 0 ; i < 500 ; ++i)
			table.rows << QStringList({QString::number(i), QStringLiteral("=FACTORY+CP1-K1:13, ").repeated(i % 30 + 1)});
		const QString path = m_dir.filePath(QStringLiteral("list.pdf"));
		const int pages = ProjectReports::writePdf(table, QStringLiteral("Wire list"), QStringLiteral("Project"), path);
		QVERIFY(pages > 1);
		QFile file(path);
		QVERIFY(file.open(QIODevice::ReadOnly));
		QVERIFY(file.read(5) == "%PDF-");
			//An empty list still makes a page with its titles
		table.rows.clear();
		QCOMPARE(ProjectReports::writePdf(table, QStringLiteral("Wire list"), QString(),
										  m_dir.filePath(QStringLiteral("empty.pdf"))), 1);
	}

	void dialogShowsAndExports()
	{
		QETProject project;
		build(project);
		ReportsDialog dialog(&project);
		auto *choice = dialog.findChild<QComboBox *>(QStringLiteral("reportChoice"));
		auto *place = dialog.findChild<QComboBox *>(QStringLiteral("reportPlace"));
		auto *table = dialog.findChild<QTableWidget *>(QStringLiteral("reportTable"));
		QVERIFY(choice && place && table);
		QCOMPARE(choice->count(), ProjectReports::all().size());
		QCOMPARE(table->rowCount(), 4);   //the parts list
		choice->setCurrentIndex(1);       //the wire list
		QCOMPARE(table->rowCount(), 2);
		place->setCurrentText(QStringLiteral("=SILO"));
		QCOMPARE(table->rowCount(), 1);
		QCOMPARE(table->item(0, 0)->text(), QStringLiteral("9"));
		QVERIFY(dialog.exportCsv(m_dir.filePath(QStringLiteral("silo.csv"))));
		QVERIFY(dialog.exportPdf(m_dir.filePath(QStringLiteral("silo.pdf"))));
		QVERIFY(QFile::exists(m_dir.filePath(QStringLiteral("silo.pdf"))));
	}
};

QET_APP_TEST_MAIN(tst_projectreports)
#include "tst_projectreports.moc"
