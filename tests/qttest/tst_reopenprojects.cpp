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
	The projects of the last session are opened again at start when the
	setting is on, and the ones open when the editor closes are kept for
	the next start. Unlike the other app tests, the settings are written
	before QETApp starts, since that is when it reads them.
*/
#include "qetapptest.h"
#include "qetdiagrameditor.h"
#include "projectview.h"
#include "qetproject.h"

#include <QFile>

namespace {
	QString s_project;	///< the fixture, copied, listed in the settings
}

class TstReopenProjects : public QObject
{
	Q_OBJECT

	private slots:
		void lastProjectReopened()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			QTRY_COMPARE(editor->openedProjects().count(), 1);
			QCOMPARE(editor->openedProjects().first()->project()->filePath(), s_project);
		}

			//Closing one of two windows mid-session keeps nothing: the
			//projects of the last session are those open at the end
		void closingOneOfTwoWindowsKeepsNothing()
		{
			const QStringList before {QStringLiteral("/sentinel.qet")};
			QSettings().setValue(QStringLiteral("projects/open_at_quit"), before);
			auto *second = new QETDiagramEditor();
			second->show();
			QTRY_COMPARE(QETApp::diagramEditors().size(), 2);
			QVERIFY(second->close());
			QCOMPARE(QSettings().value(QStringLiteral("projects/open_at_quit")).toStringList(), before);
			QTRY_COMPARE(QETApp::diagramEditors().size(), 1);
		}

		void closingKeepsTheOpenProjects()
		{
			QSettings().setValue(QStringLiteral("projects/open_at_quit"), QStringList());
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor->close());
			QCOMPARE(QSettings().value(QStringLiteral("projects/open_at_quit")).toStringList(),
				 QStringList{s_project});
		}
};

int main(int argc, char **argv)
{
	QStandardPaths::setTestModeEnabled(true);
	QApplication app(argc, argv);
	QCoreApplication::setOrganizationName("QElectroTech");
	QCoreApplication::setApplicationName("QElectroTech");
	QTemporaryDir home;
	if (!home.isValid())
		return 1;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.filePath("settings"));
	QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, home.filePath("settings"));
	QETApp::applyDirectoryArguments(QETArguments(QList<QString>{
		"--config-dir=" + home.filePath("config"),
		"--data-dir=" + home.filePath("data")}));

	s_project = home.filePath("reopened.qet");
	if (!QFile::copy(QStringLiteral(QET_FIXTURE), s_project))
		return 1;
	QFile::setPermissions(s_project, QFile::ReadOwner | QFile::WriteOwner);
	{
		QSettings settings;
		settings.setValue(QStringLiteral("projects/reopen_at_start"), true);
			//One that no longer exists is skipped
		settings.setValue(QStringLiteral("projects/open_at_quit"),
				  QStringList{home.filePath("gone.qet"), s_project});
	}

	QETApp qetapp;
	QCoreApplication::processEvents();
	TstReopenProjects test;
	return QTest::qExec(&test, argc, argv);
}

#include "tst_reopenprojects.moc"
