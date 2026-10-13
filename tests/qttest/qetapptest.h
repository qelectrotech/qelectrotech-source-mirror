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
#ifndef QETAPPTEST_H
#define QETAPPTEST_H

/**
	For tests that need the application around the class they test: a
	folio, the view showing it, the editor window and its actions. They
	link the application's own code (see qet_add_app_test() in
	CMakeLists.txt) and run a real QETApp, offscreen, with its settings,
	configuration and data in a folder of their own.
*/

#include "diagram.h"
#include "diagramview.h"
#include "qetapp.h"
#include "qetarguments.h"
#include "qetdiagrameditor.h"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

namespace QetAppTest {

/// The editor window QETApp opened at start.
inline QETDiagramEditor *editor()
{
	const QList<QETDiagramEditor *> editors = QETApp::diagramEditors();
	return editors.isEmpty() ? nullptr : editors.first();
}

/// The view of the folio shown in the editor, after a new project of one
/// folio if the editor has none open.
inline DiagramView *view()
{
	QETDiagramEditor *e = editor();
	if (!e)
		return nullptr;
	if (!e->findChild<DiagramView *>())
		e->newProject();
	return e->findChild<DiagramView *>();
}

inline Diagram *diagram()
{
	DiagramView *v = view();
	return v ? v->diagram() : nullptr;
}

/// The editor's action whose data() is @p name, as the menus, the
/// toolbar and the keyboard shortcut trigger it ("rotate_selection" is
/// Space).
inline QAction *action(const QString &name)
{
	QETDiagramEditor *e = editor();
	if (!e)
		return nullptr;
	for (QAction *a : e->findChildren<QAction *>())
		if (a->data().toString() == name)
			return a;
	return nullptr;
}

/// Run @p test inside a QETApp whose settings, configuration and data
/// live in a temporary folder, so that no test reads or changes the
/// user's own.
template <typename Test>
int run(int argc, char **argv)
{
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
	QETApp qetapp;
	QCoreApplication::processEvents();
	Test test;
	return QTest::qExec(&test, argc, argv);
}

} // namespace QetAppTest

#define QET_APP_TEST_MAIN(Test) \
	int main(int argc, char **argv) { return QetAppTest::run<Test>(argc, argv); }

#endif
