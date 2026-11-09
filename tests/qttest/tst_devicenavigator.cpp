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
#include <QStandardPaths>

#include "ui/devicenavigatordockwidget.h"
#include "ui/devicenavigatorfilterdialog.h"
#include "qetproject.h"
#include "diagram.h"
#include "titleblockproperties.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/masterelement.h"
#include "qetgraphicsitem/slaveelement.h"
#include "cable/cable.h"
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/structureboxitem.h"
#include "qetresult.h"

#include <QDomDocument>
#include <QTemporaryDir>

class tst_devicenavigator : public QObject
{
	Q_OBJECT

	private slots:
		void initTestCase();
		void cleanup();
		void testDefaultState();
		void testStructureGroupingAndNoStructure();
		void testReversingContactorMasterSlave();
		void testFilterAndSearch();
		void testTableListView();
		void testExcludesFolioReportsAndConductorDefinitions();
		void testConfigurableFilterSchemes();
		void testFolioFormulaEvaluation();
		void testTreePreservesExpansionAndSelectionOnReload();
		void testLazyLoadingWhenDockIsHidden();
		void testFolioSortingNumerically();
		void testCableManagementIntegration();
		void testFilterDialogOkPersistsEdits();
		void testFilterDialogDeleteDoesNotOverwriteNext();
		void testLazyLoadingOpensProjectWithoutBuildingFolios();
		void testStructureBoxGivesThePlace();
		void testStructureBoxOfAnUnbuiltFolio();

	private:
		Element *symbol(Diagram *diagram);
		void structureBoxFolio(Diagram *diagram);
		void checkStructureBoxTree(DeviceNavigatorDockWidget &nav);
};

void tst_devicenavigator::initTestCase()
{
	QStandardPaths::setTestModeEnabled(true);
	qApp->setAttribute(Qt::AA_Use96Dpi, true);
}

void tst_devicenavigator::cleanup()
{
	QSettings settings;
	settings.clear();
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
	// diag2 has no plant and no locmach

	// Add element 1 to diag1 (inherits structure =010 +A1)
	ElementsLocation loc;
	auto *elmt1 = new Element(loc);
	DiagramContext dc1 = elmt1->elementInformations();
	dc1.addValue(QStringLiteral("label"), QStringLiteral("FC1"));
	dc1.addValue(QStringLiteral("comment"), QStringLiteral("Power supply"));
	elmt1->setElementInformations(dc1);
	diag1->addItem(elmt1);

	// Add element 2 to diag2 (has no structure)
	auto *elmt2 = new Element(loc);
	DiagramContext dc2 = elmt2->elementInformations();
	dc2.addValue(QStringLiteral("label"), QStringLiteral("Q01"));
	dc2.addValue(QStringLiteral("comment"), QStringLiteral("Main switch"));
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

	// Check FC1 under =010 +A1 (displayed as entered, without forced prepending of -)
	QCOMPARE(structItem1->childCount(), 1);
	QTreeWidgetItem *fc1Item = structItem1->child(0);
	QCOMPARE(fc1Item->text(0), QStringLiteral("FC1"));

	// Check Q01 under No structure identifier
	QCOMPARE(structItem2->childCount(), 1);
	QTreeWidgetItem *q01Item = structItem2->child(0);
	QCOMPARE(q01Item->text(0), QStringLiteral("Q01"));
}

void tst_devicenavigator::testReversingContactorMasterSlave()
{
	QETProject project;
	auto *diagPower = project.addNewDiagram();
	TitleBlockProperties tbpPower;
	tbpPower.folio = QStringLiteral("002");
	tbpPower.plant = QStringLiteral("422 (Drive)");
	diagPower->border_and_titleblock.importTitleBlock(tbpPower);

	auto *diagControl = project.addNewDiagram();
	TitleBlockProperties tbpControl;
	tbpControl.folio = QStringLiteral("005");
	tbpControl.plant = QStringLiteral("422 (Drive)");
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
	dcK1Coil.addValue(QStringLiteral("comment"), QStringLiteral("Forward"));
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

	// K1: NO auxiliary contact on folio 005
	auto *k1NO = new SlaveElement(loc);
	ElementData edK1NO = k1NO->elementData();
	edK1NO.m_type = ElementData::Slave;
	edK1NO.m_slave_state = ElementData::NO;
	k1NO->setElementData(edK1NO);
	DiagramContext dcK1NO = k1NO->elementInformations();
	dcK1NO.addValue(QStringLiteral("label"), QStringLiteral("K1"));
	dcK1NO.addValue(QStringLiteral("comment"), QStringLiteral("Self-hold"));
	k1NO->setElementInformations(dcK1NO);
	diagControl->addItem(k1NO);

	// K1: NC auxiliary contact on folio 005
	auto *k1NC = new SlaveElement(loc);
	ElementData edK1NC = k1NC->elementData();
	edK1NC.m_type = ElementData::Slave;
	edK1NC.m_slave_state = ElementData::NC;
	k1NC->setElementData(edK1NC);
	DiagramContext dcK1NC = k1NC->elementInformations();
	dcK1NC.addValue(QStringLiteral("label"), QStringLiteral("K1"));
	dcK1NC.addValue(QStringLiteral("comment"), QStringLiteral("Interlock K2"));
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
	dcK2Coil.addValue(QStringLiteral("comment"), QStringLiteral("Reverse"));
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

	QTreeWidgetItem *driveStruct = projItem->child(0);
	QVERIFY(driveStruct != nullptr);
	QCOMPARE(driveStruct->text(0), QStringLiteral("=422 (Drive)"));

	// Expect exactly 2 devices under Drive: K1 and K2
	QCOMPARE(driveStruct->childCount(), 2);
	QTreeWidgetItem *k1Item = driveStruct->child(0);
	QTreeWidgetItem *k2Item = driveStruct->child(1);

	QCOMPARE(k1Item->text(0), QStringLiteral("K1"));
	QCOMPARE(k2Item->text(0), QStringLiteral("K2"));

	// K1 has 4 functions: Coil (Master), Power contacts (Slave), NO contact (Slave), NC contact (Slave)
	QCOMPARE(k1Item->childCount(), 4);
	// K2 has 2 functions: Coil (Master), Power contacts (Slave)
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

	// Initially all 3 devices (FC1, K1, X1) are visible
	QCOMPARE(noStruct->childCount(), 3);
	QVERIFY(!noStruct->child(0)->isHidden());
	QVERIFY(!noStruct->child(1)->isHidden());
	QVERIFY(!noStruct->child(2)->isHidden());

	// Filter by K
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
	QCOMPARE(table->item(0, 0)->text(), QStringLiteral("K1"));
	QCOMPARE(table->item(0, 3)->text(), QStringLiteral("=010"));
}

void tst_devicenavigator::testExcludesFolioReportsAndConductorDefinitions()
{
	QETProject project;
	auto *diag = project.addNewDiagram();

	ElementsLocation loc;

	// 1. NextReport - should be excluded
	auto *reportNext = new Element(loc);
	ElementData edNext = reportNext->elementData();
	edNext.m_type = ElementData::NextReport;
	reportNext->setElementData(edNext);
	DiagramContext dcNext = reportNext->elementInformations();
	dcNext.addValue(QStringLiteral("label"), QStringLiteral("L1"));
	reportNext->setElementInformations(dcNext);
	diag->addItem(reportNext);

	// 2. PreviousReport - should be excluded
	auto *reportPrev = new Element(loc);
	ElementData edPrev = reportPrev->elementData();
	edPrev.m_type = ElementData::PreviousReport;
	reportPrev->setElementData(edPrev);
	DiagramContext dcPrev = reportPrev->elementInformations();
	dcPrev.addValue(QStringLiteral("label"), QStringLiteral("N"));
	reportPrev->setElementInformations(dcPrev);
	diag->addItem(reportPrev);

	// 3. ConductorDefinition - should be excluded
	auto *condDef = new Element(loc);
	ElementData edCond = condDef->elementData();
	edCond.m_type = ElementData::ConductorDefinition;
	condDef->setElementData(edCond);
	DiagramContext dcCond = condDef->elementInformations();
	dcCond.addValue(QStringLiteral("label"), QStringLiteral("W1"));
	condDef->setElementInformations(dcCond);
	diag->addItem(condDef);

	// 4. Real Device: Contactor Coil (K1) - should be included
	auto *k1 = new MasterElement(loc);
	ElementData edK1 = k1->elementData();
	edK1.m_type = ElementData::Master;
	edK1.m_master_type = ElementData::Coil;
	k1->setElementData(edK1);
	DiagramContext dcK1 = k1->elementInformations();
	dcK1.addValue(QStringLiteral("label"), QStringLiteral("K1"));
	k1->setElementInformations(dcK1);
	diag->addItem(k1);

	// 5. Real Device: Terminal (X1) - should be included
	auto *x1 = new Element(loc);
	ElementData edX1 = x1->elementData();
	edX1.m_type = ElementData::Terminal;
	x1->setElementData(edX1);
	DiagramContext dcX1 = x1->elementInformations();
	dcX1.addValue(QStringLiteral("label"), QStringLiteral("X1"));
	x1->setElementInformations(dcX1);
	diag->addItem(x1);

	DeviceNavigatorDockWidget nav;
	nav.setProject(&project);
	nav.reload();

	auto *tree = nav.findChild<QTreeWidget*>(QStringLiteral("m_tree_widget"));
	QVERIFY(tree != nullptr);

	QTreeWidgetItem *proj = tree->topLevelItem(0);
	QVERIFY(proj != nullptr);

	QTreeWidgetItem *structItem = proj->child(0);
	QVERIFY(structItem != nullptr);

	// Only real devices K1 and X1 should exist (2 devices).
	QCOMPARE(structItem->childCount(), 2);
	QCOMPARE(structItem->child(0)->text(0), QStringLiteral("K1"));
	QCOMPARE(structItem->child(1)->text(0), QStringLiteral("X1"));
}

void tst_devicenavigator::testConfigurableFilterSchemes()
{
	// 1. Verify default schemes and dynamic translation
	QList<DeviceNavigatorFilterScheme> defaults = DeviceNavigatorFilterScheme::defaultSchemes();
	QVERIFY(defaults.size() >= 8);
	QCOMPARE(defaults.at(0).id, QStringLiteral("none"));
	QCOMPARE(defaults.at(0).displayName(), QStringLiteral("- Not active -"));
	QCOMPARE(defaults.at(1).id, QStringLiteral("contactors"));
	QCOMPARE(defaults.at(1).bmkLetters, QStringLiteral("K"));

	// 2. Test Scheme Matching logic with hasStructure flag
	DeviceNavigatorFilterScheme schemeK;
	schemeK.bmkLetters = QStringLiteral("K");
	// Should match K1 (or -K1)
	QVERIFY(schemeK.matches(QStringLiteral("K1"), QStringLiteral("=010"), true, true, false, false, false, false, false, false));
	// Should NOT match FC1
	QVERIFY(!schemeK.matches(QStringLiteral("FC1"), QStringLiteral("=010"), true, true, false, false, false, false, false, false));
	// Should NOT match X1
	QVERIFY(!schemeK.matches(QStringLiteral("X1"), QStringLiteral("=010"), true, false, false, true, false, false, false, false));

	// Test Structure filter mode (hasStructure flag, independent of localized string)
	DeviceNavigatorFilterScheme schemeNoStruct;
	schemeNoStruct.structureMode = 2; // Without structure only
	QVERIFY(schemeNoStruct.matches(QStringLiteral("K1"), QStringLiteral("No structure identifier"), false, true, false, false, false, false, false, false));
	QVERIFY(!schemeNoStruct.matches(QStringLiteral("K1"), QStringLiteral("=010 +A1"), true, true, false, false, false, false, false, false));

	// Test element types filtering
	DeviceNavigatorFilterScheme schemeTerminalsOnly;
	schemeTerminalsOnly.showMasters = false;
	schemeTerminalsOnly.showSlaves = false;
	schemeTerminalsOnly.showTerminals = true;
	QVERIFY(!schemeTerminalsOnly.matches(QStringLiteral("K1"), QStringLiteral("=010"), true, true, false, false, false, false, false, false));
	QVERIFY(schemeTerminalsOnly.matches(QStringLiteral("X1"), QStringLiteral("=010"), true, false, false, true, false, false, false, false));

	// 3. Test Filter Dialog load / edit
	DeviceNavigatorFilterDialog dlg;
	dlg.setSchemes(defaults, 1);
	QCOMPARE(dlg.currentSchemeIndex(), 1);
	QCOMPARE(dlg.schemes().size(), defaults.size());

	// 4. Test Scheme in DeviceNavigatorDockWidget
	QETProject project;
	auto *diag = project.addNewDiagram();
	ElementsLocation loc;

	auto *fc1 = new Element(loc);
	DiagramContext dcfc = fc1->elementInformations();
	dcfc.addValue(QStringLiteral("label"), QStringLiteral("FC1"));
	fc1->setElementInformations(dcfc);
	diag->addItem(fc1);

	auto *k1 = new Element(loc);
	DiagramContext dck = k1->elementInformations();
	dck.addValue(QStringLiteral("label"), QStringLiteral("K1"));
	k1->setElementInformations(dck);
	diag->addItem(k1);

	DeviceNavigatorDockWidget nav;
	nav.setProject(&project);
	nav.reload();

	auto *filterCombo = nav.findChild<QComboBox*>(QStringLiteral("m_filter_combo"));
	auto *tree = nav.findChild<QTreeWidget*>(QStringLiteral("m_tree_widget"));
	QVERIFY(filterCombo != nullptr && tree != nullptr);

	QTreeWidgetItem *proj = tree->topLevelItem(0);
	QTreeWidgetItem *noStruct = proj->child(0);
	QCOMPARE(noStruct->childCount(), 2);

	// Select K scheme (index 1)
	filterCombo->setCurrentIndex(1);
	QVERIFY(noStruct->child(0)->isHidden());  // FC1 hidden
	QVERIFY(!noStruct->child(1)->isHidden()); // K1 visible

	// Select F scheme (index 2)
	filterCombo->setCurrentIndex(2);
	QVERIFY(!noStruct->child(0)->isHidden()); // FC1 visible
	QVERIFY(noStruct->child(1)->isHidden());  // K1 hidden
}

void tst_devicenavigator::testFolioFormulaEvaluation()
{
	QETProject project;
	auto *diag1 = project.addNewDiagram();
	auto *diag2 = project.addNewDiagram();

	TitleBlockProperties tbp1;
	tbp1.folio = QStringLiteral("%id/%total");
	diag1->border_and_titleblock.importTitleBlock(tbp1);

	TitleBlockProperties tbp2;
	tbp2.folio = QStringLiteral("%id");
	diag2->border_and_titleblock.importTitleBlock(tbp2);

	ElementsLocation loc;
	auto *k1 = new Element(loc);
	DiagramContext dck1 = k1->elementInformations();
	dck1.addValue(QStringLiteral("label"), QStringLiteral("K1"));
	k1->setElementInformations(dck1);
	diag1->addItem(k1);

	auto *k2 = new Element(loc);
	DiagramContext dck2 = k2->elementInformations();
	dck2.addValue(QStringLiteral("label"), QStringLiteral("K2"));
	k2->setElementInformations(dck2);
	diag2->addItem(k2);

	DeviceNavigatorDockWidget nav;
	nav.setProject(&project);
	nav.reload();

	auto *table = nav.findChild<QTableWidget*>(QStringLiteral("m_table_widget"));
	QVERIFY(table != nullptr);
	QCOMPARE(table->rowCount(), 2);

	// Folio column is column 4
	QString folio1 = table->item(0, 4)->text();
	QString folio2 = table->item(1, 4)->text();

	// Neither should contain unexpanded raw variables
	QVERIFY(!folio1.contains(QStringLiteral("%id")));
	QVERIFY(!folio1.contains(QStringLiteral("%total")));
	QVERIFY(!folio2.contains(QStringLiteral("%id")));

	// Folio 1 evaluated to "1/2", Folio 2 to "2"
	QCOMPARE(folio1, QStringLiteral("1/2"));
	QCOMPARE(folio2, QStringLiteral("2"));
}

void tst_devicenavigator::testTreePreservesExpansionAndSelectionOnReload()
{
	QETProject project;
	auto *diag = project.addNewDiagram();
	TitleBlockProperties tbp;
	tbp.plant = QStringLiteral("010 (Supply)");
	diag->border_and_titleblock.importTitleBlock(tbp);

	ElementsLocation loc;
	auto *fc1 = new Element(loc);
	DiagramContext dcfc = fc1->elementInformations();
	dcfc.addValue(QStringLiteral("label"), QStringLiteral("FC1"));
	fc1->setElementInformations(dcfc);
	diag->addItem(fc1);

	DeviceNavigatorDockWidget nav;
	nav.setProject(&project);
	nav.reload();

	auto *tree = nav.findChild<QTreeWidget*>(QStringLiteral("m_tree_widget"));
	QVERIFY(tree != nullptr);
	QCOMPARE(tree->topLevelItemCount(), 1);

	QTreeWidgetItem *projectItem = tree->topLevelItem(0);
	QVERIFY(projectItem->isExpanded());
	QCOMPARE(projectItem->childCount(), 1);

	QTreeWidgetItem *structItem = projectItem->child(0);
	QCOMPARE(structItem->text(0), QStringLiteral("=010 (Supply)"));
	QCOMPARE(structItem->childCount(), 1);

	QTreeWidgetItem *deviceItem = structItem->child(0);
	QCOMPARE(deviceItem->text(0), QStringLiteral("FC1"));

	// Initially structure and device are not expanded
	QVERIFY(!structItem->isExpanded());
	QVERIFY(!deviceItem->isExpanded());

	// Expand structure and device, select device item
	structItem->setExpanded(true);
	deviceItem->setExpanded(true);
	tree->setCurrentItem(deviceItem);

	QVERIFY(structItem->isExpanded());
	QVERIFY(deviceItem->isExpanded());
	QCOMPARE(tree->currentItem(), deviceItem);

	// Trigger reload
	nav.reload();

	// Verify project root, structure item, and device item are still expanded
	QCOMPARE(tree->topLevelItemCount(), 1);
	projectItem = tree->topLevelItem(0);
	QVERIFY(projectItem->isExpanded());
	QCOMPARE(projectItem->childCount(), 1);

	structItem = projectItem->child(0);
	QVERIFY(structItem->isExpanded());
	QCOMPARE(structItem->childCount(), 1);

	deviceItem = structItem->child(0);
	QVERIFY(deviceItem->isExpanded());

	// Verify selection is preserved on the device item
	QVERIFY(tree->currentItem() != nullptr);
	QCOMPARE(tree->currentItem()->text(0), QStringLiteral("FC1"));
}

void tst_devicenavigator::testLazyLoadingWhenDockIsHidden()
{
	QETProject project;
	auto *diag = project.addNewDiagram();
	ElementsLocation loc;
	auto *k1 = new Element(loc);
	DiagramContext dck = k1->elementInformations();
	dck.addValue(QStringLiteral("label"), QStringLiteral("K1"));
	k1->setElementInformations(dck);
	diag->addItem(k1);

	// Create dock widget without showing it
	DeviceNavigatorDockWidget nav;
	QVERIFY(!nav.isVisible());

	nav.setProject(&project);

	auto *tree = nav.findChild<QTreeWidget*>(QStringLiteral("m_tree_widget"));
	QVERIFY(tree != nullptr);

	// Hidden dock widget must not have built/populated items yet
	QCOMPARE(tree->topLevelItemCount(), 0);

	// Showing the dock widget triggers visibilityChanged(true) and populates data
	nav.show();
	QTRY_VERIFY_WITH_TIMEOUT(tree->topLevelItemCount() == 1, 1000);
}

void tst_devicenavigator::testFolioSortingNumerically()
{
	QETProject project;

	// Create 11 diagrams (folios 1 to 11)
	Diagram *diag9 = nullptr;
	Diagram *diag10 = nullptr;
	for (int i = 1; i <= 11; ++i) {
		auto *diag = project.addNewDiagram();
		TitleBlockProperties tbp;
		tbp.folio = QString::number(i);
		diag->border_and_titleblock.importTitleBlock(tbp);
		if (i == 9) diag9 = diag;
		if (i == 10) diag10 = diag;
	}

	ElementsLocation loc;
	// Add K1 slave on folio 10
	auto *k1_slave10 = new SlaveElement(loc);
	ElementData ed10 = k1_slave10->elementData();
	ed10.m_type = ElementData::Slave;
	k1_slave10->setElementData(ed10);
	DiagramContext dc10 = k1_slave10->elementInformations();
	dc10.addValue(QStringLiteral("label"), QStringLiteral("K1"));
	k1_slave10->setElementInformations(dc10);
	diag10->addItem(k1_slave10);

	// Add K1 slave on folio 9
	auto *k1_slave9 = new SlaveElement(loc);
	ElementData ed9 = k1_slave9->elementData();
	ed9.m_type = ElementData::Slave;
	k1_slave9->setElementData(ed9);
	DiagramContext dc9 = k1_slave9->elementInformations();
	dc9.addValue(QStringLiteral("label"), QStringLiteral("K1"));
	k1_slave9->setElementInformations(dc9);
	diag9->addItem(k1_slave9);

	DeviceNavigatorDockWidget nav;
	nav.setProject(&project);
	nav.reload();

	auto *tree = nav.findChild<QTreeWidget*>(QStringLiteral("m_tree_widget"));
	QVERIFY(tree != nullptr);
	QCOMPARE(tree->topLevelItemCount(), 1);

	QTreeWidgetItem *projItem = tree->topLevelItem(0);
	QTreeWidgetItem *noStructItem = projItem->child(0);
	QTreeWidgetItem *k1Item = noStructItem->child(0);
	QCOMPARE(k1Item->text(0), QStringLiteral("K1"));
	QCOMPARE(k1Item->childCount(), 2);

	// Folio 9 must be sorted BEFORE Folio 10 (numeric sort, not lexicographical "10" < "9")
	QVERIFY(k1Item->child(0)->text(0).contains(QStringLiteral("#9")));
	QVERIFY(k1Item->child(1)->text(0).contains(QStringLiteral("#10")));
}

void tst_devicenavigator::testCableManagementIntegration()
{
	QETProject project;
	auto *diag = project.addNewDiagram();

	// Create and register a Cable in the project
	auto *cable = new Cable(&project);
	cable->setDesignation(QStringLiteral("W1"));
	cable->setType(QStringLiteral("ÖLFLEX CLASSIC 110 4G1.5"));
	cable->setCoreCount(4);
	cable->setInstallation(QStringLiteral("010"));
	cable->setLocation(QStringLiteral("M1"));
	project.addCable(cable);

	DeviceNavigatorDockWidget nav;
	nav.setProject(&project);
	nav.reload();

	auto *tree = nav.findChild<QTreeWidget*>(QStringLiteral("m_tree_widget"));
	QVERIFY(tree != nullptr);

	QTreeWidgetItem *projItem = tree->topLevelItem(0);
	QVERIFY(projItem != nullptr);

	// Structure "=010 +M1"
	QTreeWidgetItem *structItem = projItem->child(0);
	QVERIFY(structItem != nullptr);
	QCOMPARE(structItem->text(0), QStringLiteral("=010 +M1"));

	// Cable device "W1"
	QCOMPARE(structItem->childCount(), 1);
	QTreeWidgetItem *cableItem = structItem->child(0);
	QCOMPARE(cableItem->text(0), QStringLiteral("W1"));

	// Leaf function
	QCOMPARE(cableItem->childCount(), 1);
	QTreeWidgetItem *funcItem = cableItem->child(0);
	QVERIFY(funcItem->text(0).contains(QStringLiteral("4 cores")));
	QVERIFY(funcItem->text(0).contains(QStringLiteral("ÖLFLEX CLASSIC 110 4G1.5")));
}

void tst_devicenavigator::testFilterDialogOkPersistsEdits()
{
	DeviceNavigatorFilterDialog dlg;
	QList<DeviceNavigatorFilterScheme> schemes = DeviceNavigatorFilterScheme::defaultSchemes();
	dlg.setSchemes(schemes, 1); // Select index 1 ("-K")

	auto *cbReport = dlg.findChild<QCheckBox*>(QStringLiteral("m_cb_report"));
	QVERIFY(cbReport != nullptr);
	cbReport->setChecked(true);

	// Accept dialog (simulates user pressing OK)
	dlg.accept();

	QList<DeviceNavigatorFilterScheme> saved = dlg.schemes();
	QVERIFY(saved.size() > 1);
	QCOMPARE(saved.at(1).showReports, true);
}

void tst_devicenavigator::testFilterDialogDeleteDoesNotOverwriteNext()
{
	DeviceNavigatorFilterDialog dlg;
	QList<DeviceNavigatorFilterScheme> schemes = DeviceNavigatorFilterScheme::defaultSchemes();
	dlg.setSchemes(schemes, 1); // Index 1: "-K" with bmkLetters "K"

	// Index 2 is "-F" with bmkLetters "F"
	QCOMPARE(schemes.at(2).bmkLetters, QStringLiteral("F"));

	auto *deleteBtn = dlg.findChild<QPushButton*>(QStringLiteral("m_delete_scheme_btn"));
	QVERIFY(deleteBtn != nullptr);

	QTimer::singleShot(50, []() {
		for (QWidget *w : QApplication::topLevelWidgets()) {
			if (auto *mb = qobject_cast<QMessageBox*>(w)) {
				if (QAbstractButton *btn = mb->button(QMessageBox::Yes)) {
					btn->click();
					break;
				}
			}
		}
	});

	deleteBtn->click();

	QList<DeviceNavigatorFilterScheme> remaining = dlg.schemes();
	QVERIFY(remaining.size() > 1);
	// Slot 1 is now the former scheme 2 ("-F")
	QCOMPARE(remaining.at(1).bmkLetters, QStringLiteral("F"));
}

void tst_devicenavigator::testLazyLoadingOpensProjectWithoutBuildingFolios()
{
	QString projectPath = QStringLiteral("examples/industrial.qet");
	if (!QFile::exists(projectPath)) {
		projectPath = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../../../examples/industrial.qet"));
	}
	if (!QFile::exists(projectPath)) {
		projectPath = QStringLiteral("../examples/industrial.qet");
	}
	if (!QFile::exists(projectPath)) {
		projectPath = QStringLiteral("../../examples/industrial.qet");
	}
	if (!QFile::exists(projectPath)) {
		QSKIP("examples/industrial.qet not found");
	}

	QETProject project(projectPath);
	QVERIFY(project.unloadedFolioCount() > 0);
	const int unloadedBefore = project.unloadedFolioCount();

	DeviceNavigatorDockWidget nav;
	nav.show();
	nav.setProject(&project);
	nav.reload();

	// Zero folios must be built by loading or reloading the Device Navigator!
	QCOMPARE(project.unloadedFolioCount(), unloadedBefore);

	auto *tree = nav.findChild<QTreeWidget*>(QStringLiteral("m_tree_widget"));
	QVERIFY(tree != nullptr);
	QVERIFY(tree->topLevelItemCount() > 0);

	auto *table = nav.findChild<QTableWidget*>(QStringLiteral("m_table_widget"));
	QVERIFY(table != nullptr);
	QVERIFY(table->rowCount() > 0);
}

Element *tst_devicenavigator::symbol(Diagram *diagram)
{
	auto *collection = diagram->project()->embeddedElementCollection();
	if (!collection->exist(QStringLiteral("import/simple.elmt"))) {
		QDomDocument definition;
		definition.setContent(
			QString(R"(<definition type="element" link_type="simple" uuid="%1" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test symbol</name></names>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%2"/><terminal x="0" y="20" orientation="s" uuid="%3"/>
</description></definition>)")
				.arg(QUuid::createUuid().toString(),
					 QUuid::createUuid().toString(),
					 QUuid::createUuid().toString()));
		if (!collection->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("simple"),
				definition.documentElement()))
			return nullptr;
	}
	int state = 0;
	Element *element = ElementFactory::Instance()->createElement(
		ElementsLocation(QStringLiteral("embed://import/simple.elmt"),
						 diagram->project()),
		nullptr, &state);
	if (state || !element) return nullptr;
	diagram->addItem(element);
	return element;
}

// A folio =FACTORY +CP1 holding a box +FIELD: K1 in the box, K2 outside it,
// K3 in the box with its own location
void tst_devicenavigator::structureBoxFolio(Diagram *diagram)
{
	TitleBlockProperties folio;
	folio.plant = QStringLiteral("FACTORY");
	folio.locmach = QStringLiteral("CP1");
	diagram->border_and_titleblock.importTitleBlock(folio);

	auto *box = new StructureBoxItem(QPointF(100, 100), QPointF(500, 500));
	box->setLocation(QStringLiteral("FIELD"));
	diagram->addItem(box);

	const auto place = [this, diagram](const QString &label, QPointF pos,
									   const QString &location) {
		Element *element = symbol(diagram);
		QVERIFY(element);
		element->setPos(pos);
		DiagramContext dc = element->elementInformations();
		dc.addValue(QStringLiteral("label"), label);
		if (!location.isEmpty())
			dc.addValue(QStringLiteral("location"), location);
		element->setElementInformations(dc);
	};
	place(QStringLiteral("K1"), QPointF(200, 200), QString());
	place(QStringLiteral("K2"), QPointF(800, 800), QString());
	place(QStringLiteral("K3"), QPointF(300, 300), QStringLiteral("DB50"));
}

void tst_devicenavigator::checkStructureBoxTree(DeviceNavigatorDockWidget &nav)
{
	auto *tree = nav.findChild<QTreeWidget*>(QStringLiteral("m_tree_widget"));
	QVERIFY(tree);
	QCOMPARE(tree->topLevelItemCount(), 1);
	QMap<QString, QStringList> shown;
	QTreeWidgetItem *root = tree->topLevelItem(0);
	for (int s = 0 ; s < root->childCount() ; ++s)
		for (int d = 0 ; d < root->child(s)->childCount() ; ++d)
			shown[root->child(s)->text(0)] << root->child(s)->child(d)->text(0);
	QMap<QString, QStringList> expected;
	expected[QStringLiteral("=FACTORY +FIELD")] = {QStringLiteral("K1")};
	expected[QStringLiteral("=FACTORY +CP1")] = {QStringLiteral("K2")};
	expected[QStringLiteral("=FACTORY +DB50")] = {QStringLiteral("K3")};
	QCOMPARE(shown, expected);
}

void tst_devicenavigator::testStructureBoxGivesThePlace()
{
	QETProject project;
	structureBoxFolio(project.addNewDiagram());
	if (QTest::currentTestFailed()) return;

	DeviceNavigatorDockWidget nav;
	nav.setProject(&project);
	nav.reload();
	checkStructureBoxTree(nav);
}

void tst_devicenavigator::testStructureBoxOfAnUnbuiltFolio()
{
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QString path = dir.filePath(QStringLiteral("boxes.qet"));
	{
		QETProject project;
		structureBoxFolio(project.addNewDiagram());
		if (QTest::currentTestFailed()) return;
		project.setFilePath(path);
		const QETResult saved = project.write();
		QVERIFY2(saved.isOk(), qPrintable(saved.errorMessage()));
	}

	QETProject project(path);
	QCOMPARE(project.unloadedFolioCount(), 1);

	DeviceNavigatorDockWidget nav;
	nav.show();
	nav.setProject(&project);
	nav.reload();
	QCOMPARE(project.unloadedFolioCount(), 1);
	checkStructureBoxTree(nav);
}

QTEST_MAIN(tst_devicenavigator)
#include "tst_devicenavigator.moc"
