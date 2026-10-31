// SPDX-License-Identifier: GPL-2.0-or-later
// Cable undo ownership regression, linked against the application's objects.
//
// Deleting a cable and undoing the delete twice leaves the cable out of
// the project while AddCableCommand still holds it, and the next edit
// makes the stack throw both commands away. Whoever of the two runs
// first frees the cable; the other one must not follow it into the freed
// memory. A raw pointer there meant freeing the same cable twice, which
// a normal build hides and AddressSanitizer reports as a heap
// use-after-free in ~AddCableCommand.
#include <QApplication>
#include <QFontDatabase>
#include <QSettings>
#include <QTemporaryDir>
#include <QUuid>
#include <cstdio>
#include <stdexcept>

#include "../../sources/diagram.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/cable/addcablecommand.h"
#include "../../sources/cable/cable.h"
#include "../../sources/cable/cablepart.h"
#include "../../sources/cable/editcablecommand.h"

static void check(bool value, const char *message)
{
	if (!value) throw std::runtime_error(message);
}

// Any edit the user makes afterwards: pushing it is what makes the stack
// discard the commands it still holds undone, and that is where the two
// destructors meet the same cable.
struct AnyEdit : QUndoCommand
{
	void redo() override {}
	void undo() override {}
};

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	std::freopen(argv[2], "w", stdout);

	QTemporaryDir settings;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
	QCoreApplication::setOrganizationName("QETCableUndoRegression");
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");

	try
	{
		QETProject project(QString::fromLocal8Bit(argv[1]));
		check(project.state() == QETProject::Ok, "fixture opens");
		Diagram *scene = project.diagrams().first();

		// The state the cable tool leaves behind when it has drawn one
		// line and the dialog accepted it: a cable holding the section,
		// a line on the folio pointing back at that cable.
		CablePartData data;
		data.uuid = QUuid::createUuid();
		data.diagram = scene->uuid();
		data.p1 = QPointF(100, 100);
		data.p2 = QPointF(300, 100);

		//The cable the create dialog hands over: owned by the project,
		//which is the only way it may enter the project's list at all.
		auto *cable = project.newCable();
		cable->addPart(data);
		auto *item = new CablePart(data);
		item->setCable(cable);
		scene->addItem(item);

		// Draw the line.
		scene->undoStack().push(new AddCableCommand(cable, item, scene, true));
		check(project.cables().contains(cable), "drawn cable is in the project");

		// Select all and delete.
		scene->undoStack().push(new RemoveCableCommand(scene, {item}));
		check(!project.cables().contains(cable), "delete took the cable out of the project");

		// Ctrl+Z, Ctrl+Z: the cable is in nobody's hands now, but both
		// commands still stand and both think they may have to clean it up.
		scene->undoStack().undo();
		scene->undoStack().undo();
		check(!project.cables().contains(cable), "cable orphaned by two undos");

		// The next edit of any kind: the stack throws the undone commands
		// away, and ~RemoveCableCommand frees the cable before
		// ~AddCableCommand gets to look at it.
		scene->undoStack().push(new AnyEdit);

		check(project.state() == QETProject::Ok, "project survives the edit");
		std::puts("PASS: deleting a cable, undoing twice and editing again frees it exactly once");
	}
	catch (const std::exception &error)
	{
		std::printf("FAIL: %s\n", error.what());
		return 1;
	}
	return 0;
}
