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

#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTimer>

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
		void onlyTheFoliosAsked();
		void labelStickingOut();
		void namesCappedPerFolio();
		void cancelStopsYesGoesOn();
		void saveHintBuildsNoFolio();
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
	const QList<Diagram *> all_ = m_project->diagrams();
	QCOMPARE(QETDiagramEditor::outsideSheetReport(all_), QStringList());
	m_element->setPos(m_sheet.right() + 200, m_sheet.top() + 100);
	const QStringList report_ = QETDiagramEditor::outsideSheetReport(all_);
	QCOMPARE(report_.size(), 1);
		//Named as in the Projects panel: the folio's number, its title
	QString title_ = m_folio->title();
	if (title_.isEmpty())
		title_ = QETDiagramEditor::tr("Untitled sheet", "Fallback label when a diagram has no title");
	const QString start_ = m_folio->border_and_titleblock.finalfolio() + QStringLiteral(" - ") + title_;
	QVERIFY2(report_.first().startsWith(start_), qPrintable(report_.first() + " / " + start_));
	QETDiagramEditor::setWarnsOutsideSheet(false);
	QCOMPARE(QETDiagramEditor::outsideSheetReport(all_), QStringList());
}

void tst_outsidesheet::onlyTheFoliosAsked()
{
	m_element->setPos(m_sheet.right() + 200, m_sheet.top() + 100);
	QList<Diagram *> others_ = m_project->diagrams();
	others_.removeAll(m_folio);
	QCOMPARE(QETDiagramEditor::outsideSheetReport(others_), QStringList());
}

	//The symbol is inside, its text is not: the symbol is named
void tst_outsidesheet::labelStickingOut()
{
	auto *text_ = new IndependentTextItem(QStringLiteral("x"));
	text_->setParentItem(m_element);
	const QRectF body_ = m_element->sceneBoundingRect();
	m_element->setPos(m_element->pos() + QPointF(m_sheet.right() - body_.right() - 5, 0));
	QVERIFY(m_sheet.contains(m_element->sceneBoundingRect()));
	text_->setPos(m_element->mapFromScene(QPointF(m_sheet.right() + 20, body_.top())));
	QCOMPARE(m_folio->itemsOutsideSheet().size(), 1);
}

	//A folio with many items outside names five and counts the rest
void tst_outsidesheet::namesCappedPerFolio()
{
	for (int i = 0; i < 7; ++i) {
		auto *text_ = new IndependentTextItem(QStringLiteral("note"));
		m_folio->addItem(text_);
		text_->setPos(m_sheet.left() - 300, m_sheet.top() + 40 * i);
	}
	const QStringList report_ = QETDiagramEditor::outsideSheetReport({m_folio});
	QCOMPARE(report_.size(), 1);
	QCOMPARE(report_.first().count(QObject::tr("a text")), 5);
	QVERIFY2(report_.first().endsWith(QETDiagramEditor::tr("%n more", "", 2)), qPrintable(report_.first()));
}

	//The question stops printing or exporting on Cancel, goes on on Yes
void tst_outsidesheet::cancelStopsYesGoesOn()
{
	m_element->setPos(m_sheet.right() + 200, m_sheet.top() + 100);
	const auto answer = [](QMessageBox::StandardButton button) {
		QTimer::singleShot(0, [button] {
			auto *box_ = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
			QVERIFY(box_);
			box_->button(button)->click();
		});
	};
	answer(QMessageBox::Cancel);
	QVERIFY(!QETDiagramEditor::confirmOutsideSheet(nullptr, {m_folio}));
	answer(QMessageBox::Yes);
	QVERIFY(QETDiagramEditor::confirmOutsideSheet(nullptr, {m_folio}));
		//Nothing asked when nothing lies outside
	QList<Diagram *> others_ = m_project->diagrams();
	others_.removeAll(m_folio);
	QVERIFY(QETDiagramEditor::confirmOutsideSheet(nullptr, others_));
}

	//With lazy folios, the hint after saving builds no folio (#1461)
void tst_outsidesheet::saveHintBuildsNoFolio()
{
	if (!QETProject::lazyFolios())
		QSKIP("folios are built on opening (QET_EAGER_FOLIOS)");
	QETProject lazy_(QFINDTESTDATA("fixtures/every_feature.qet"));
	QVERIFY(lazy_.folios().size() > 1);
	Diagram *first_ = lazy_.folios().first();
	lazy_.buildFolios({first_});
	const int built_ = lazy_.builtFolios().size();
	QVERIFY(built_ < lazy_.folios().size());
	Element *element_ = first_->elements().value(0);
	QVERIFY(element_);
	const QRectF sheet_ = first_->border_and_titleblock.borderAndTitleBlockRect();
	element_->setPos(sheet_.right() + 200, sheet_.top() + 100);
	QVERIFY(!QETDiagramEditor::outsideSheetSaveHint(&lazy_).isEmpty());
	QCOMPARE(lazy_.builtFolios().size(), built_);
}

int main(int argc, char **argv)
{
	QStandardPaths::setTestModeEnabled(true);
	return QetAppTest::run<tst_outsidesheet>(argc, argv);
}

#include "tst_outsidesheet.moc"
