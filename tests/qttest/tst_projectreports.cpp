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

	Element *symbol(Diagram *diagram, const QString &label,
					const QString &link_type = QStringLiteral("simple"))
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		if (!collection->exist(QStringLiteral("import/%1.elmt").arg(link_type))) {
			QDomDocument definition;
			if (!definition.setContent(QString(
					R"(<definition type="element" link_type="%4" uuid="%1" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test</name></names>
<kindInformations><kindInformation name="type">%5</kindInformation></kindInformations>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" name="1" uuid="%2"/><terminal x="0" y="20" orientation="s" name="2" uuid="%3"/>
</description></definition>)")
					.arg(QUuid::createUuid().toString(), QUuid::createUuid().toString(),
						 QUuid::createUuid().toString(), link_type,
						 link_type == QLatin1String("master") ? QStringLiteral("coil")
															  : QStringLiteral("simple"))))
				return nullptr;
			if (!collection->addElementDefinition(QStringLiteral("import"), link_type,
												  definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(link_type), diagram->project()),
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

	void panelSpacePerPlace()
	{
		QETProject project;
		build(project);
		Diagram *factory = project.folios().at(0);
		Diagram *silo = project.folios().at(1);
		const auto part = [](Element *element, const QHash<QString, QString> &fields) {
			auto info = element->elementInformations();
			for (auto it = fields.cbegin() ; it != fields.cend() ; ++it)
				info.addValue(it.key(), it.value());
			element->setElementInformations(info);
		};
		const auto labelled = [factory](const QString &label) -> Element * {
			for (Element *e : factory->elements())
				if (e->elementInformations().value(QStringLiteral("label")).toString() == label)
					return e;
			return nullptr;
		};
		Element *k1 = labelled(QStringLiteral("K1"));
		Element *k2 = labelled(QStringLiteral("K2"));
		QVERIFY(k1 && k2);
			//K1 two of 45 x 90 ("2 pcs" read as the parts list reads it),
			//K2 100 x 50 and 120 deep, K5 10 x 10, the unlabelled coil
			//20 x 10: 13400 mm2
		part(k1, {{"width", "45"}, {"height", "90"}, {"quantity", "2 pcs"}, {"manufacturer_reference", "A"}});
		part(k2, {{"width", "100"}, {"height", "50"}, {"depth", "120"}});
			//A coil and its contact with the same part data: one part
		Element *k5 = symbol(factory, QStringLiteral("K5"), QStringLiteral("master"));
		Element *contact = symbol(factory, QString(), QStringLiteral("slave"));
		QVERIFY(k5 && contact);
		part(k5, {{"width", "10"}, {"height", "10"}});
		part(contact, {{"width", "10"}, {"height", "10"}});
		contact->linkToElement(k5);
		QVERIFY(!contact->isFree());
			//The same without labels: still one part
		Element *coil = symbol(factory, QString(), QStringLiteral("master"));
		Element *its_contact = symbol(factory, QString(), QStringLiteral("slave"));
		QVERIFY(coil && its_contact);
		part(coil, {{"width", "20"}, {"height", "10"}});
		part(its_contact, {{"width", "20"}, {"height", "10"}});
		its_contact->linkToElement(coil);
		QVERIFY(!its_contact->isFree());
			//A coil left out of the parts list takes its contact, drawn
			//in another place with the part data, out with it
		Element *k6 = symbol(factory, QStringLiteral("K6"), QStringLiteral("master"));
		Element *k6_contact = symbol(silo, QString(), QStringLiteral("slave"));
		QVERIFY(k6 && k6_contact);
		part(k6, {{"exclude_from_bom", "true"}});
		part(k6_contact, {{"width", "30"}, {"height", "30"}, {"manufacturer_reference", "D"}});
		k6_contact->linkToElement(k6);
		QVERIFY(!k6_contact->isFree());
			//A part number and no size; no part at all; left out of the parts list
		Element *x1 = symbol(factory, QStringLiteral("X1"));
		Element *h1 = symbol(factory, QStringLiteral("H1"));
		Element *k4 = symbol(factory, QStringLiteral("K4"));
		QVERIFY(x1 && h1 && k4);
		part(x1, {{"manufacturer_reference", "B"}});
		part(k4, {{"width", "500"}, {"height", "500"}, {"exclude_from_bom", "true"}});
			//"45 mm" is no number: no size, so a part without one
		Element *s1 = nullptr;
		for (Element *e : silo->elements())
			if (e->elementInformations().value(QStringLiteral("label")).toString() == QLatin1String("S1")) s1 = e;
		QVERIFY(s1);
		part(s1, {{"width", "45 mm"}, {"height", "90"}, {"manufacturer_reference", "C"}});

			//Shown to the mm2: a 5 x 5 part alone is 0.000025, not 0
		Element *s9 = symbol(silo, QStringLiteral("S9"));
		QVERIFY(s9);
		part(s9, {{"width", "5"}, {"height", "5"}});

		const auto space = ProjectReports::report(QStringLiteral("space"));
		const auto table = ProjectReports::read(&project, space);
		QVERIFY2(table.error.isEmpty(), qPrintable(table.error));
		QCOMPARE(table.rows.size(), 2);
		QCOMPARE(table.rows.at(0), QStringList({QStringLiteral("=FACTORY+CP1"), QStringLiteral("4"),
												QStringLiteral("0.0134"), QStringLiteral("120"),
												QStringLiteral("1")}));
		QCOMPARE(table.rows.at(1), QStringList({QStringLiteral("=SILO+P_1"), QStringLiteral("1"),
												QStringLiteral("0.000025"), QString(), QStringLiteral("1")}));
		QCOMPARE(column(ProjectReports::read(&project, space, {QStringLiteral("SILO"), {}}), 0),
				 QStringList({QStringLiteral("=SILO+P_1")}));
		QCOMPARE(column(ProjectReports::read(&project, space, {QStringLiteral("FACTORY"), QStringLiteral("CP1")}), 0),
				 QStringList({QStringLiteral("=FACTORY+CP1")}));
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
