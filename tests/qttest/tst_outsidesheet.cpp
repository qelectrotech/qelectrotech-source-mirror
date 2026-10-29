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
#include "qetapptest.h"

#include "diagram.h"
#include "qetdiagrameditor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetproject.h"

#include <QStandardPaths>

/**
	What lies outside the sheet is cut off or left out when a folio is
	printed or exported: Diagram::itemsOutsideSheet() names it, symbols by
	their label, and the editor's report gives one line per folio unless
	the warning is switched off (Settings > General).
*/
class tst_outsidesheet : public QObject
{
	Q_OBJECT

	QETProject *m_project = nullptr;
	Diagram *m_folio = nullptr;
	Element *m_element = nullptr;
	QRectF m_sheet;

	private slots:
		void init();
		void cleanup();
		void nothingOutsideAtFirst();
		void symbolWhollyOutside();
		void symbolPartlyOutside();
		void hiddenSymbolIgnored();
		void freeTextOutside();
		void reportPerFolioAndSwitch();
};

void tst_outsidesheet::init()
{
	const QString fixture_ = QFINDTESTDATA("fixtures/every_feature.qet");
	QVERIFY(!fixture_.isEmpty());
	m_project = new QETProject(fixture_);
	QCOMPARE(m_project->state(), QETProject::Ok);
	m_project->buildFolios(m_project->diagrams());
	m_folio = m_project->diagrams().value(0);
	QVERIFY(m_folio);
	m_element = m_folio->elements().value(0);
	QVERIFY(m_element);
	m_sheet = m_folio->border_and_titleblock.borderAndTitleBlockRect();
}

void tst_outsidesheet::cleanup()
{
	QETDiagramEditor::setWarnsOutsideSheet(true);
	delete m_project;
	m_project = nullptr;
}

void tst_outsidesheet::nothingOutsideAtFirst()
{
	QCOMPARE(m_folio->itemsOutsideSheet(), QStringList());
}

void tst_outsidesheet::symbolWhollyOutside()
{
	m_element->setPos(m_sheet.right() + 200, m_sheet.top() + 100);
	const QString label_ = m_element->actualLabel();
	const QStringList outside_ = m_folio->itemsOutsideSheet();
	QCOMPARE(outside_.size(), 1);
	QCOMPARE(outside_.first(), label_.isEmpty() ? m_element->name() : label_);
}

void tst_outsidesheet::symbolPartlyOutside()
{
	const QRectF body_ = m_element->sceneBoundingRect();
	m_element->moveBy(m_sheet.right() - body_.center().x(), 0);
	QVERIFY(m_element->sceneBoundingRect().intersects(m_sheet));
	QCOMPARE(m_folio->itemsOutsideSheet().size(), 1);
}

void tst_outsidesheet::hiddenSymbolIgnored()
{
	m_element->setPos(m_sheet.right() + 200, m_sheet.top() + 100);
	m_element->setVisible(false);
	QCOMPARE(m_folio->itemsOutsideSheet(), QStringList());
}

void tst_outsidesheet::freeTextOutside()
{
	auto *text_ = new IndependentTextItem(QStringLiteral("note"));
	m_folio->addItem(text_);
	text_->setPos(m_sheet.left() - 300, m_sheet.top());
	QCOMPARE(m_folio->itemsOutsideSheet(), QStringList(QObject::tr("a text")));
}

void tst_outsidesheet::reportPerFolioAndSwitch()
{
	QCOMPARE(QETDiagramEditor::outsideSheetReport(m_project), QStringList());
	m_element->setPos(m_sheet.right() + 200, m_sheet.top() + 100);
	const QStringList report_ = QETDiagramEditor::outsideSheetReport(m_project);
	QCOMPARE(report_.size(), 1);
	QVERIFY2(report_.first().startsWith(QStringLiteral("Folio 1")), qPrintable(report_.first()));
	QETDiagramEditor::setWarnsOutsideSheet(false);
	QCOMPARE(QETDiagramEditor::outsideSheetReport(m_project), QStringList());
}

int main(int argc, char **argv)
{
	QStandardPaths::setTestModeEnabled(true);
	return QetAppTest::run<tst_outsidesheet>(argc, argv);
}

#include "tst_outsidesheet.moc"
