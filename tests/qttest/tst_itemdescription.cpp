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
#include "shownkinds.h"
#include "diagram.h"
#include "itemdescription.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/diagramimageitem.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/qetshapeitem.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "projectview.h"
#include "diagramview.h"

#include <QLabel>
#include <QStatusBar>

/**
	What the status bar says an item of a folio is (ItemDescription): its
	kind, with the label, the name or the text that tells it apart, for
	every kind of item of fixtures/every_feature.qet; a terminal is its
	symbol; a selection of several items is counted.
*/
class tst_itemdescription : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	private slots:
		void everyKind()
		{
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			QVERIFY(!project.diagrams().isEmpty());

			QMap<QString, int> seen;
			for (Diagram *diagram : project.diagrams()) {
				for (QGraphicsItem *item : diagram->items()) {
					const QString text = ItemDescription::of(item);
					if (auto element = qgraphicsitem_cast<Element *>(item)) {
						QVERIFY2(text.contains(element->name()), qPrintable(text));
						const QString label = element->actualLabel();
						if (!label.isEmpty())
							QVERIFY2(text.contains(label), qPrintable(text));
						if (element->elementData().m_type == ElementData::Simple)
							QVERIFY2(text.startsWith(QStringLiteral("Symbol: ")), qPrintable(text));
						++seen["element"];
					} else if (auto dti = qgraphicsitem_cast<DynamicElementTextItem *>(item)) {
						QVERIFY2(text.startsWith(QStringLiteral("Symbol text of ")), qPrintable(text));
						QVERIFY2(text.contains(dti->parentElement()->name()), qPrintable(text));
						++seen["symbol text"];
					} else if (qgraphicsitem_cast<IndependentTextItem *>(item)) {
						QVERIFY2(text.startsWith(QStringLiteral("Free text: ")), qPrintable(text));
						++seen["free text"];
					} else if (qgraphicsitem_cast<Conductor *>(item)) {
						QVERIFY2(text.startsWith(QStringLiteral("Conductor")), qPrintable(text));
						++seen["conductor"];
					} else if (qgraphicsitem_cast<QetShapeItem *>(item)) {
						QVERIFY2(text.endsWith(QStringLiteral("(shape)")), qPrintable(text));
						++seen["shape"];
					} else if (qgraphicsitem_cast<DiagramImageItem *>(item)) {
						QCOMPARE(text, QStringLiteral("Picture"));
						++seen["picture"];
					} else if (auto terminal = qgraphicsitem_cast<Terminal *>(item)) {
						QCOMPARE(text, ItemDescription::of(terminal->parentItem()));
						++seen["terminal"];
					}
				}
			}
			for (const char *kind : {"element", "symbol text", "free text",
						 "conductor", "terminal"})
				QVERIFY2(seen.value(kind) > 0, kind);

			Diagram *diagram = project.diagrams().first();
			QList<QGraphicsItem *> two;
			for (QGraphicsItem *item : diagram->items())
				if (qgraphicsitem_cast<Element *>(item) && two.size() < 2)
					two << item;
			QCOMPARE(two.size(), 2);
			QCOMPARE(ItemDescription::ofSelection(two), QStringLiteral("2 items selected"));
			QCOMPARE(ItemDescription::ofSelection({two.first()}), ItemDescription::of(two.first()));
			QVERIFY(ItemDescription::ofSelection({}).isEmpty());
			QVERIFY(ItemDescription::of(nullptr).isEmpty());
		}

			/// The status bar of the editor: the selection, and over it
			/// what the mouse points at
		void statusBar()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			editor->resize(1400, 1000);
			editor->show();
			QVERIFY(QTest::qWaitForWindowExposed(editor));
			const QString copy = m_dir.filePath("p.qet");
			QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/every_feature.qet"), copy));
			QVERIFY(editor->openAndAddProject(copy, false));
			DiagramView *view = editor->currentProjectView()->currentDiagram();
			Diagram *diagram = view->diagram();
			auto label = editor->findChild<QLabel *>(QStringLiteral("item_info_label"));
			QVERIFY(label);
			editor->statusBar()->clearMessage();

			Element *element = nullptr;
			for (QGraphicsItem *item : diagram->items())
				if (item->type() == Element::Type && !element)
					element = static_cast<Element *>(item);
			QVERIFY(element);
			diagram->clearSelection();
			element->setSelected(true);
			QTest::qWait(50);
			QCOMPARE(label->text(), ItemDescription::of(element));
			QVERIFY(label->isVisible());

			diagram->clearSelection();
			view->centerOn(element);
			QTest::qWait(50);
			const QPoint at = view->mapFromScene(element->mapToScene(element->boundingRect().center()));
			QTest::mouseMove(view->viewport(), at);
			QTest::qWait(50);
			QVERIFY2(!label->text().isEmpty(), "nothing said over a symbol");

				//A free text hidden by View > Show is not described
			IndependentTextItem *text = nullptr;
			for (QGraphicsItem *item : diagram->items())
				if (item->type() == IndependentTextItem::Type && !text)
					text = static_cast<IndependentTextItem *>(item);
			QVERIFY(text);
			ShownKinds::setShown(ShownKinds::FreeTexts, false);
			ShownKinds::apply(diagram, ShownKinds::FreeTexts);
			view->centerOn(text);
			QTest::qWait(50);
			const QPoint over_text = view->mapFromScene(text->mapToScene(text->boundingRect().center()));
			QTest::mouseMove(view->viewport(), over_text);
			QTest::qWait(50);
			QVERIFY(label->text() != ItemDescription::of(text));
			ShownKinds::setShown(ShownKinds::FreeTexts, true);
			ShownKinds::apply(diagram, ShownKinds::FreeTexts);
		}
};

QET_APP_TEST_MAIN(tst_itemdescription)

#include "tst_itemdescription.moc"
