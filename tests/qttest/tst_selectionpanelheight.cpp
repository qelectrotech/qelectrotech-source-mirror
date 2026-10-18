// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"

#include "projectview.h"
#include "qetgraphicsitem/element.h"

#include <QDockWidget>

// The Selection properties panel keeps the height the user gave it when the
// selection has nothing it can edit for a moment (#1439). It used to shrink
// to its title bar, the other panels took the room, and an element selected
// again brought the panel back at its default height.
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

				// The panel follows the selection through a queued connection
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

			select(nullptr);
			QCOMPARE(panel->height(), user);
			select(element);
			QCOMPARE(panel->height(), user);
		}
};

QET_APP_TEST_MAIN(tst_selectionpanelheight)

#include "tst_selectionpanelheight.moc"
