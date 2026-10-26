// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"

#include "projectview.h"
#include "qetgraphicsitem/element.h"

#include <QDockWidget>

// The Selection properties panel shrinks to its title bar while the selection
// has nothing it can edit, so the other panels get the room (#1566), and gets
// back the height the user gave it when an element is selected again. Before
// #1439 it came back at its default height; #1533 kept the height but no
// longer shrank. Tabbed with another panel, it does not shrink: a tab stack
// takes the smallest maximum height of its tabs, and the empty panel held
// the Collections tab at its title bar.
class tst_selectionpanelheight : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	private slots:
		void keepsUserHeight()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
				// Tall enough to leave the panels room to be resized: at a
				// usual height every right-hand panel is at its minimum
			editor->resize(1600, 1500);
			editor->show();
			QVERIFY(QTest::qWaitForWindowExposed(editor));

				// A copy: opening a project leaves a lock file beside it
			const QString copy = m_dir.filePath("p.qet");
			QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/ten_sequential_numbers.qet"), copy));
			QVERIFY(editor->openAndAddProject(copy, false));
			Diagram *diagram = editor->currentProjectView()->currentDiagram()->diagram();
			Element *element = nullptr;
			for (QGraphicsItem *qgi : diagram->items())
				if (qgi->type() == Element::Type)
					element = static_cast<Element *>(qgi);
			QVERIFY(element);

			auto panel = editor->findChild<QDockWidget *>("diagram_properties_editor_dock_widget");
			QVERIFY(panel);

				// The panel follows the selection through a queued connection,
				// and the dock layout after it: the checks below wait for it
				// (QTRY_*) rather than for a fixed time, which a slow build,
				// e.g. a Debug MSVC one, can miss
			auto select = [&](Element *e) {
				diagram->clearSelection();
				if (e)
					e->setSelected(true);
				QTest::qWait(200);
			};

			select(element);
			const int start = panel->height();
				// What dragging the separator above the panel does
			editor->resizeDocks({panel}, {start + 150}, Qt::Vertical);
			QTest::qWait(200);
			const int user = panel->height();
			QVERIFY2(user > start, "the panel could not be made taller");

			for (int round = 0; round < 2; ++round) {
				select(nullptr);
				QTRY_VERIFY2(panel->height() < start / 4, "the empty panel did not shrink");
				select(element);
				QTRY_COMPARE(panel->height(), user);
			}
		}

		void tabbedStackResizes()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			editor->resize(1600, 1500);
			editor->show();
			QVERIFY(QTest::qWaitForWindowExposed(editor));
			const QString copy = m_dir.filePath("q.qet");
			QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/ten_sequential_numbers.qet"), copy));
			QVERIFY(editor->openAndAddProject(copy, false));
			Diagram *diagram = editor->currentProjectView()->currentDiagram()->diagram();
			Element *element = nullptr;
			for (QGraphicsItem *qgi : diagram->items())
				if (qgi->type() == Element::Type)
					element = static_cast<Element *>(qgi);
			QVERIFY(element);
			auto panel = editor->findChild<QDockWidget *>("diagram_properties_editor_dock_widget");
			auto collections = editor->findChild<QDockWidget *>("elements_collection_widget");
			QVERIFY(panel);
			QVERIFY(collections);

				// Collections dragged onto the empty panel, as a tab in front
			element->setSelected(true);
			QTest::qWait(200);
			diagram->clearSelection();
			QTest::qWait(200);
			QTRY_VERIFY2(panel->maximumHeight() < 100, "the panel is not empty");
			editor->tabifyDockWidget(panel, collections);
			collections->raise();
			QTest::qWait(200);
			QCOMPARE(editor->dockWidgetArea(collections), editor->dockWidgetArea(panel));
			QVERIFY(editor->tabifiedDockWidgets(panel).contains(collections));

			const int start = collections->height();
				// Above the minimum height of the element's editor, which
				// a tab stack keeps while the editor is shown
			editor->resizeDocks({collections}, {start + 700}, Qt::Vertical);
			QTest::qWait(200);
			const int user = collections->height();
			QVERIFY2(user > start, "the tab stack could not be made taller");

				// Selecting and deselecting leaves the stack alone
			element->setSelected(true);
			QTest::qWait(200);
			diagram->clearSelection();
			QTest::qWait(200);
			QCOMPARE(collections->height(), user);

				// Back on its own, the empty panel shrinks again
			editor->removeDockWidget(collections);
			editor->addDockWidget(Qt::RightDockWidgetArea, collections);
			collections->show();
			QTest::qWait(200);
			QVERIFY(editor->tabifiedDockWidgets(panel).isEmpty());
			element->setSelected(true);
			QTest::qWait(200);
			const int selected = panel->height();
			diagram->clearSelection();
			QTest::qWait(200);
			QTRY_VERIFY2(panel->height() < selected / 4, "the empty panel did not shrink once untabbed");
		}
};

QET_APP_TEST_MAIN(tst_selectionpanelheight)

#include "tst_selectionpanelheight.moc"
