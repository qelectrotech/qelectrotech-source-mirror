// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"

#include "elementspanel.h"
#include "projectview.h"
#include "qetproject.h"

// projectWasOpened() adds the project to the elements panel. Before the panel's
// first reload() the project's tree item is left detached (treeWidget() ==
// nullptr), and selecting its first child crashed in QTreeModel
// (pre-existing since a82f6de23).
class tst_elementspanelprojectopen : public QObject
{
	Q_OBJECT

	private slots:
		void addProjectBeforeReloadDoesNotCrash()
		{
				// A real one-folio project from the running app; diagrams()
				// builds the lazy folio, so the project item will have a child.
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			QVERIFY(editor->newProject());
			ProjectView *pv = editor->currentProjectView();
			QVERIFY(pv);
			QETProject *project = pv->project();
			QVERIFY(project);
			QVERIFY(!project->diagrams().isEmpty());

				// Fresh panel: reload() has not run, so the project item is inserted detached.
			ElementsPanel panel;

				// Without the guard this selects a detached item; with it, nothing is selected.
			panel.projectWasOpened(project);

			QTreeWidgetItem *current = panel.currentItem();
			QVERIFY2(current == nullptr || current->treeWidget() == &panel,
				"the panel selected a detached item it does not own");
		}
};

QET_APP_TEST_MAIN(tst_elementspanelprojectopen)

#include "tst_elementspanelprojectopen.moc"
