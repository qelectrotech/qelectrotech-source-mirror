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
#include "ui/dialogwaiting.h"

#include <QDialog>
#include <QTimer>

/**
	A file handed to QElectroTech while a project is being read (a
	double-click in the file manager, another instance passing it on)
	waits until that project is read. The progress dialog of the reading
	lets events through: read inside the first project, the second one
	closed the dialog the first one then used, and QElectroTech crashed
	(#1978).
*/
class tst_openwhilereading : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

		/// A copy of the fixture, named @p name
	QString copy(const QString &name)
	{
		const QString path = m_dir.filePath(name);
		QFile::copy(QFINDTESTDATA("fixtures/every_feature.qet"), path);
		return path;
	}

	private slots:
		void fileHandedOverWaits()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			const QString first = m_dir.filePath("first.qet");
			const QString second = m_dir.filePath("second.qet");
			const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
			QVERIFY(QFile::copy(fixture, first));
			QVERIFY(QFile::copy(fixture, second));

				//The second file is opened as a person opens it, so what it
				//asks on opening (a backup copy) is answered. The
				//progress dialog of the reading is left alone
			QTimer closer;
			connect(&closer, &QTimer::timeout, this, []() {
				auto *box = qobject_cast<QDialog *>(
					QApplication::activeModalWidget());
				if (box && !box->inherits("DialogWaiting")) {
					box->reject();
				}
			});
			closer.start(20);

				//Runs as the events are let through during the reading
			bool handed_over = false;
			bool read_inside = false;
			QObject in_this_test;
			QTimer::singleShot(0, &in_this_test, [&]() {
				handed_over = DialogWaiting::hasInstance();
				QETApp::instance()->openFiles(
					QETArguments(QStringList{second}));
				read_inside = QETApp::diagramEditorForFile(second);
			});
			QVERIFY(editor->openAndAddProject(first, false));

				//The reading was going on, else this test proves nothing
			QVERIFY(handed_over);
			QVERIFY(!read_inside);
			QVERIFY(QETApp::diagramEditorForFile(first));
			QTRY_VERIFY(QETApp::diagramEditorForFile(second));
		}

			/// A project opened straight away from inside a reading, as
			/// the last session's projects or a script can be: it shares
			/// the progress dialog and leaves it to the first reading
		void openedInsideSharesTheDialog()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			const QString outer = copy("outer.qet");
			const QString inner = copy("inner.qet");

			bool opened_inside = false;
			bool dialog_left_up = false;
			QObject in_this_test;
			QTimer::singleShot(0, &in_this_test, [&]() {
				if (!DialogWaiting::hasInstance()) return;
				opened_inside = editor->openAndAddProject(inner, false);
				dialog_left_up = DialogWaiting::hasInstance();
			});
			QVERIFY(editor->openAndAddProject(outer, false));

			QVERIFY(opened_inside);
			QVERIFY(dialog_left_up);
			QVERIFY(!DialogWaiting::hasInstance());
			QVERIFY(QETApp::diagramEditorForFile(outer));
			QVERIFY(QETApp::diagramEditorForFile(inner));
		}

			/// A new project shows no progress dialog and leaves none, so a
			/// project handed over afterwards opens at once
		void newProjectLeavesNoDialog()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			QVERIFY(!DialogWaiting::hasInstance());
			editor->newProject();
			QVERIFY(!DialogWaiting::hasInstance());

			const QString later = copy("later.qet");
			QTimer closer;
			connect(&closer, &QTimer::timeout, this, []() {
				if (auto *box = qobject_cast<QDialog *>(
						QApplication::activeModalWidget()))
					box->reject();
			});
			closer.start(20);
			QETApp::instance()->openFiles(QETArguments(QStringList{later}));
			QVERIFY(QETApp::diagramEditorForFile(later));
		}
};

QET_APP_TEST_MAIN(tst_openwhilereading)

#include "tst_openwhilereading.moc"
