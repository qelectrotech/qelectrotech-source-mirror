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
#include <QtTest>
#include <QTreeWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QTabBar>

#include "ui/devicenavigatordockwidget.h"
#include "qetproject.h"
#include "diagram.h"
#include "titleblockproperties.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/masterelement.h"
#include "qetgraphicsitem/slaveelement.h"
#include "ElementsCollection/elementslocation.h"

class tst_devicenavigator : public QObject
{
	Q_OBJECT

	private slots:
		void initTestCase();
		void testDefaultState();
		void testStructureGroupingAndNoStructure();
		void testWendeschuetzReversingContactor();
		void testFilterAndSearch();
		void testTableListView();
};

void tst_devicenavigator::initTestCase()
{
	qApp->setAttribute(Qt::AA_Use96Dpi, true);
}

void tst_devicenavigator::testDefaultState()
{
	DeviceNavigatorDockWidget nav;
	QCOMPARE(nav.windowTitle(), QStringLiteral("Device Navigator"));
	QVERIFY(nav.project() == nullptr);

	auto *tree = nav.findChild<QTreeWidget*>(QStringLiteral("m_tree_widget"));
	QVERIFY(tree != nullptr);
	QCOMPARE(tree->topLevelItemCount(), 0);

	auto *table = nav.findChild<QTableWidget*>(QStringLiteral("m_table_widget"));
	QVERIFY(table != nullptr);
	QCOMPARE(table->rowCount(), 0);
}

void tst_devicenavigator::testStructureGroupingAndNoStructure()
{
	QETProject project;
	auto *diag1 = project.addNewDiagram();
	TitleBlockProperties tbp1;
	tbp1.plant = QStringLiteral("010");
	tbp1.locmach = QStringLiteral("A1");
	diag1->border_and_titleblock.importTitleBlock(tbp1);

	auto *diag2 = project.addNewDiagram();
	// diag2 has NO plant and NO locmach

	// Add element 1 to diag1 (inherits =010 +A1)
	ElementsLocation loc;
	auto *elmt1 = new Element(loc);
	DiagramContext dc1 = elmt1->elementInformations();
	dc1.addValue(QStringLiteral("label"), QStringLiteral("FC1"));
	dc1.addValue(QStringLiteral("comment"), QStringLiteral("Einspeisung"));
	elmt1->setElementInformations(dc1);
	diag1->addItem(elmt1);

	// Add element 2 to diag2 (has NO structure)
	auto *elmt2 = new Element(loc);
	DiagramContext dc2 = elmt2->elementInformations();
	dc2.addValue(QStringLiteral("label"), QStringLiteral("Q01"));
	dc2.addValue(QStringLiteral("comment"), QStringLiteral("Hauptschalter"));
	elmt2->setElementInformations(dc2);
	diag2->addItem(elmt2);

	DeviceNavigatorDockWidget nav;
	nav.setProject(&project);
	nav.reload();

	auto *tree = nav.findChild<QTreeWidget*>(QStringLiteral("m_tree_widget"));
	QVERIFY(tree != nullptr);
	QCOMPARE(tree->topLevelItemCount(), 1); // 1 Project Root

	QTreeWidgetItem *projItem = tree->topLevelItem(0);
	QVERIFY(projItem != nullptr);
	QCOMPARE(projItem->childCount(), 2); // 2 Structures: "=010 +A1" and "No structure identifier"

	QTreeWidgetItem *structItem1 = projItem->child(0);
	QTreeWidgetItem *structItem2 = projItem->child(1);

	QCOMPARE(structItem1->text(0), QStringLiteral("=010 +A1"));
	QCOMPARE(structItem2->text(0), QStringLiteral("No structure identifier"));

	// Check FC1 under =010 +A1
	QCOMPARE(structItem1->childCount(), 1);
	QTreeWidgetItem *fc1Item = structItem1->child(0);
	QCOMPARE(fc1Item->text(0), QStringLiteral("-FC1"));

	// Check Q01 under No structure identifier
	QCOMPARE(structItem2->childCount(), 1);
	QTreeWidgetItem *q01Item = structItem2->child(0);
	QCOMPARE(q01Item->text(0), QStringLiteral("-Q01"));
}

void tst_devicenavigator::testWendeschuetzReversingContactor()
{
	QETProject project;
	auto *diagPower = project.addNewDiagram();
	TitleBlockProperties tbpPower;
	tbpPower.folio = QStringLiteral("002");
	tbpPower.plant = QStringLiteral("422 (Fahrantrieb)");
	diagPower->border_and_titleblock.importTitleBlock(tbpPower);

	auto *diagControl = project.addNewDiagram();
	TitleBlockProperties tbpControl;
	tbpControl.folio = QStringLiteral("005");
	tbpControl.plant = QStringLiteral("422 (Fahrantrieb)");
	diagControl->border_and_titleblock.importTitleBlock(tbpControl);

	ElementsLocation loc;

	// K1: Coil (Master) on folio 005
	auto *k1Coil = new MasterElement(loc);
	ElementData edK1Coil = k1Coil->elementData();
	edK1Coil.m_type = ElementData::Master;
	edK1Coil.m_master_type = ElementData::Coil;
	k1Coil->setElementData(edK1Coil);
	DiagramContext dcK1Coil = k1Coil->elementInformations();
	dcK1Coil.addValue(QStringLiteral("label"), QStringLiteral("K1"));
	dcK1Coil.addValue(QStringLiteral("comment"), QStringLiteral("Rechtslauf"));
	k1Coil->setElementInformations(dcK1Coil);
	diagControl->addItem(k1Coil);

	// K1: Power contacts (Slave) on folio 002
	auto *k1Power = new SlaveElement(loc);
	ElementData edK1Power = k1Power->elementData();
	edK1Power.m_type = ElementData::Slave;
	edK1Power.m_slave_type = ElementData::Power;
	k1Power->setElementData(edK1Power);
	DiagramContext dcK1Power = k1Power->elementInformations();
	dcK1Power.addValue(QStringLiteral("label"), QStringLiteral("K1"));
	k1Power->setElementInformations(dcK1Power);
	diagPower->addItem(k1Power);

	// K1: NO auxiliary contact (Selbsthaltung) on folio 005
	auto *k1NO = new SlaveElement(loc);
	ElementData edK1NO = k1NO->elementData();
	edK1NO.m_type = ElementData::Slave;
	edK1NO.m_slave_state = ElementData::NO;
	k1NO->setElementData(edK1NO);
	DiagramContext dcK1NO = k1NO->elementInformations();
	dcK1NO.addValue(QStringLiteral("label"), QStringLiteral("K1"));
	dcK1NO.addValue(QStringLiteral("comment"), QStringLiteral("Selbsthaltung"));
	k1NO->setElementInformations(dcK1NO);
	diagControl->addItem(k1NO);

	// K1: NC auxiliary contact (Verriegelung in K2) on folio 005
	auto *k1NC = new SlaveElement(loc);
	ElementData edK1NC = k1NC->elementData();
	edK1NC.m_type = ElementData::Slave;
	edK1NC.m_slave_state = ElementData::NC;
	k1NC->setElementData(edK1NC);
	DiagramContext dcK1NC = k1NC->elementInformations();
	dcK1NC.addValue(QStringLiteral("label"), QStringLiteral("K1"));
	dcK1NC.addValue(QStringLiteral("comment"), QStringLiteral("Verriegelung K2"));
	k1NC->setElementInformations(dcK1NC);
	diagControl->addItem(k1NC);

	// K2: Coil (Master) on folio 005
	auto *k2Coil = new MasterElement(loc);
	ElementData edK2Coil = k2Coil->elementData();
	edK2Coil.m_type = ElementData::Master;
	edK2Coil.m_master_type = ElementData::Coil;
	k2Coil->setElementData(edK2Coil);
	DiagramContext dcK2Coil = k2Coil->elementInformations();
	dcK2Coil.addValue(QStringLiteral("label"), QStringLiteral("K2"));
	dcK2Coil.addValue(QStringLiteral("comment"), QStringLiteral("Linkslauf"));
	k2Coil->setElementInformations(dcK2Coil);
	diagControl->addItem(k2Coil);

	// K2: Power contacts (Slave) on folio 002
	auto *k2Power = new SlaveElement(loc);
	ElementData edK2Power = k2Power->elementData();
	edK2Power.m_type = ElementData::Slave;
	edK2Power.m_slave_type = ElementData::Power;
	k2Power->setElementData(edK2Power);
	DiagramContext dcK2Power = k2Power->elementInformations();
	dcK2Power.addValue(QStringLiteral("label"), QStringLiteral("K2"));
	k2Power->setElementInformations(dcK2Power);
	diagPower->addItem(k2Power);

	DeviceNavigatorDockWidget nav;
	nav.setProject(&project);
	nav.reload();

	auto *tree = nav.findChild<QTreeWidget*>(QStringLiteral("m_tree_widget"));
	QVERIFY(tree != nullptr);

	QTreeWidgetItem *projItem = tree->topLevelItem(0);
	QVERIFY(projItem != nullptr);

	QTreeWidgetItem *fahrantrieb = projItem->child(0);
	QVERIFY(fahrantrieb != nullptr);
	QCOMPARE(fahrantrieb->text(0), QStringLiteral("=422 (Fahrantrieb)"));

	// Expect exactly 2 devices under Fahrantrieb: -K1 and -K2
	QCOMPARE(fahrantrieb->childCount(), 2);
	QTreeWidgetItem *k1Item = fahrantrieb->child(0);
	QTreeWidgetItem *k2Item = fahrantrieb->child(1);

	QCOMPARE(k1Item->text(0), QStringLiteral("-K1"));
	QCOMPARE(k2Item->text(0), QStringLiteral("-K2"));

	// -K1 has 4 functions: Coil, Power contacts, NO contact, NC contact
	QCOMPARE(k1Item->childCount(), 4);
	// -K2 has 2 functions: Coil, Power contacts
	QCOMPARE(k2Item->childCount(), 2);

	// Verify folio locators are present in function texts
	QVERIFY(k1Item->child(0)->text(0).contains(QStringLiteral("#005"))); // Coil on 005
	QVERIFY(k1Item->child(1)->text(0).contains(QStringLiteral("#002"))); // Power on 002
	QVERIFY(k1Item->child(2)->text(0).contains(QStringLiteral("#005"))); // NO on 005
	QVERIFY(k1Item->child(3)->text(0).contains(QStringLiteral("#005"))); // NC on 005
}

void tst_devicenavigator::testFilterAndSearch()
{
	QETProject project;
	auto *diag = project.addNewDiagram();

	ElementsLocation loc;
	auto *k1 = new Element(loc);
	DiagramContext dck1 = k1->elementInformations();
	dck1.addValue(QStringLiteral("label"), QStringLiteral("K1"));
	k1->setElementInformations(dck1);
	diag->addItem(k1);

	auto *fc1 = new Element(loc);
	DiagramContext dcfc1 = fc1->elementInformations();
	dcfc1.addValue(QStringLiteral("label"), QStringLiteral("FC1"));
	fc1->setElementInformations(dcfc1);
	diag->addItem(fc1);

	auto *x1 = new Element(loc);
	DiagramContext dcx1 = x1->elementInformations();
	dcx1.addValue(QStringLiteral("label"), QStringLiteral("X1"));
	x1->setElementInformations(dcx1);
	diag->addItem(x1);

	DeviceNavigatorDockWidget nav;
	nav.setProject(&project);
	nav.reload();

	auto *search = nav.findChild<QLineEdit*>(QStringLiteral("m_search_field"));
	auto *filter = nav.findChild<QComboBox*>(QStringLiteral("m_filter_combo"));
	auto *tree = nav.findChild<QTreeWidget*>(QStringLiteral("m_tree_widget"));
	QVERIFY(search != nullptr && filter != nullptr && tree != nullptr);

	QTreeWidgetItem *proj = tree->topLevelItem(0);
	QTreeWidgetItem *noStruct = proj->child(0);

	// Initially all 3 devices (-FC1, -K1, -X1) are visible
	QCOMPARE(noStruct->childCount(), 3);
	QVERIFY(!noStruct->child(0)->isHidden());
	QVERIFY(!noStruct->child(1)->isHidden());
	QVERIFY(!noStruct->child(2)->isHidden());

	// Filter by -K
	filter->setCurrentIndex(1); // Contactors / Relays (-K)
	QVERIFY(noStruct->child(0)->isHidden());  // FC1 hidden
	QVERIFY(!noStruct->child(1)->isHidden()); // K1 visible
	QVERIFY(noStruct->child(2)->isHidden());  // X1 hidden

	// Reset filter, search for "X1"
	filter->setCurrentIndex(0);
	search->setText(QStringLiteral("X1"));
	QVERIFY(noStruct->child(0)->isHidden());  // FC1 hidden
	QVERIFY(noStruct->child(1)->isHidden());  // K1 hidden
	QVERIFY(!noStruct->child(2)->isHidden()); // X1 visible
}

void tst_devicenavigator::testTableListView()
{
	QETProject project;
	auto *diag = project.addNewDiagram();

	ElementsLocation loc;
	auto *k1 = new Element(loc);
	DiagramContext dck1 = k1->elementInformations();
	dck1.addValue(QStringLiteral("label"), QStringLiteral("K1"));
	dck1.addValue(QStringLiteral("plant"), QStringLiteral("010"));
	k1->setElementInformations(dck1);
	diag->addItem(k1);

	DeviceNavigatorDockWidget nav;
	nav.setProject(&project);
	nav.reload();

	auto *tabs = nav.findChild<QTabBar*>(QStringLiteral("m_view_mode_tabs"));
	auto *table = nav.findChild<QTableWidget*>(QStringLiteral("m_table_widget"));
	QVERIFY(tabs != nullptr && table != nullptr);

	// Switch to List view
	tabs->setCurrentIndex(1);
	QCOMPARE(table->rowCount(), 1);
	QCOMPARE(table->item(0, 0)->text(), QStringLiteral("-K1"));
	QCOMPARE(table->item(0, 3)->text(), QStringLiteral("=010"));
}

QTEST_MAIN(tst_devicenavigator)
#include "tst_devicenavigator.moc"

