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
/*
	A project is of one kind for good: File > New makes one without
	automatic connections, File > New project with automatic connections
	one with them, and the Project menu only shows which.
*/
#include "qetapptest.h"
#include "projectview.h"
#include "qetmessagebox.h"
#include "qetproject.h"

class TstNewProjectKind : public QObject
{
	Q_OBJECT

	private:
		QAction *entry(const QString &text)
		{
			for (QAction *a : QetAppTest::editor()->findChildren<QAction *>())
				if (a->text().startsWith(text))
					return a;
			return nullptr;
		}

		/// The Project menu's entry that shows the project's kind
		QAction *kindEntry() { return entry(QStringLiteral("Automatic connections")); }

	private slots:
		void initTestCase()
		{
			QET::QetMessageBox::setNonInteractive(true);
			QVERIFY(QetAppTest::editor());
		}

		void plainNewHasNone()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor->newProject());
			QApplication::processEvents();
			QVERIFY(!editor->currentProjectView()->project()->liveAutoConnect());
			QVERIFY(kindEntry());
			QVERIFY(!kindEntry()->isChecked());
			QVERIFY(!kindEntry()->isEnabled());
		}

		void autoConnectNewHasThem()
		{
			QAction *create = entry(QStringLiteral("New project with automatic connections"));
			QVERIFY(create);
			create->trigger();
			QApplication::processEvents();
			QETProject *project = QetAppTest::editor()->currentProjectView()->project();
			QVERIFY(project->liveAutoConnect());
			QVERIFY(kindEntry()->isChecked());
			QVERIFY(!kindEntry()->isEnabled());

				//Kept when saved and opened again
			QTemporaryDir dir;
			const QString path = dir.filePath(QStringLiteral("auto.qet"));
			project->setFilePath(path);
			QVERIFY(project->write().isOk());
			QETProject reopened(path);
			QVERIFY(reopened.liveAutoConnect());
		}
};

QET_APP_TEST_MAIN(TstNewProjectKind)

#include "tst_newprojectkind.moc"
