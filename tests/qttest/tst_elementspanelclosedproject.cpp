// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"

#include "elementspanel.h"
#include "elementspanelwidget.h"
#include "genericpanel.h"
#include "projectview.h"
#include "qetproject.h"

#include <QAbstractButton>
#include <QMessageBox>
#include <QPointer>
#include <QTimer>

// Closing a modified project and choosing Discard left the project in the
// project tree: the project still emitted projectInformationsChanged() while
// it was torn down, and the panel added it back, with items for its sheets.
// A click on one of those sheets then crashed in diagram->showMe() on the
// freed sheet (forum topic 3215, issue #1548).
class tst_elementspanelclosedproject : public QObject
{
	Q_OBJECT

	private slots:
		void discardedProjectLeavesTheTree()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			auto *panel_widget = editor->findChild<ElementsPanelWidget *>();
			QVERIFY(panel_widget);
			ElementsPanel &panel = panel_widget->elementsPanel();
				// What the app does 250 ms after the panel is first shown.
			panel.reload();

			QVERIFY(editor->newProject());
			QPointer<QETProject> project = editor->currentProjectView()->project();
			project->addNewDiagram();
			QVERIFY(project->projectWasModified());
			QTreeWidgetItem *project_item = panel.itemForProject(project);
			QVERIFY(project_item);

				// Answer "Discard" to "Do you want to save changes?".
			bool answered = false;
			QTimer answer;
			connect(&answer, &QTimer::timeout, this, [&answered]() {
				if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
					if (QAbstractButton *discard = box->button(QMessageBox::Discard)) {
						answered = true;
						discard->click();
					}
				}
			});
			answer.start(20);

				// Close it from the project tree's context menu.
			panel.setCurrentItem(project_item);
			panel_widget->closeProject();
			answer.stop();
			QVERIFY(answered);
			QTRY_VERIFY(project.isNull());

			for (int i = 0; i < panel.topLevelItemCount(); ++i) {
				QTreeWidgetItem *item = panel.topLevelItem(i);
				QVERIFY2(item->type() != QET::Project,
					qPrintable(QStringLiteral("the closed project is still in the tree: %1").arg(item->text(0))));
			}
		}
};

QET_APP_TEST_MAIN(tst_elementspanelclosedproject)

#include "tst_elementspanelclosedproject.moc"
