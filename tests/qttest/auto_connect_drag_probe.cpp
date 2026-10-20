// SPDX-License-Identifier: GPL-2.0-or-later
// Placing corners, T-junctions and folio reports along a drag, with
// the project's automatic connections on; linked against the
// application's objects.
//
// Six terminals look down from the top of the folio, three more look left
// from the right. The fourth terminal at the top has a wire drawn by hand.
// - a drag across the lines finds every line it crosses, but not the one
//   drawn by hand;
// - a corner pressed and dragged diagonally, through the mouse events of
//   the placing tool, puts one corner on each of three lines, staggered,
//   and the automatic connections join each from the top to the right:
//   one undo step for all of it, mouse pressed once;
// - a T-junction dragged across the same three lines splits each line;
// - a folio report turned to face up, dragged across two free lines, ends
//   each of them;
// - Escape during a drag gives up the drag and keeps the tool.
#include <QApplication>
#include <QFontDatabase>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>
#include <stdexcept>

#include "../../sources/autoconnect.h"
#include "../../sources/diagram.h"
#include "../../sources/diagramevent/diagrameventaddelement.h"
#include "../../sources/qetapp.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/qetgraphicsitem/conductor.h"
#include "../../sources/qetgraphicsitem/element.h"
#include "../../sources/qetgraphicsitem/terminal.h"

static void check(bool value, const char *message)
{
	if (!value) throw std::runtime_error(message);
	std::printf("ok: %s\n", message);
}

static int count(Diagram *diagram, const QString &name)
{
	int n = 0;
	for (Element *e : diagram->elements())
		n += e->location().fileName() == name + QLatin1String(".elmt")
			 && (e->flags() & QGraphicsItem::ItemIsSelectable);	//not the tool's own previews
	return n;
}

static QList<Element *> elementsOf(Diagram *diagram, const QString &name)
{
	QList<Element *> found;
	for (Element *e : diagram->elements())
		if (e->location().fileName() == name + QLatin1String(".elmt"))
			found << e;
	std::sort(found.begin(), found.end(), [](Element *a, Element *b) { return a->pos().x() < b->pos().x(); });
	return found;
}

static int automaticWires(Diagram *diagram)
{
	int n = 0;
	for (Conductor *c : diagram->conductors())
		n += c->isAutomatic();
	return n;
}

static bool joined(Element *a, Element *b)
{
	for (Terminal *t : a->terminals())
		for (Conductor *c : t->conductors())
			if (c->isAutomatic() && (c->terminal1->parentElement() == b || c->terminal2->parentElement() == b))
				return true;
	return false;
}

static void mouse(DiagramEventAddElement &tool, QEvent::Type type, const QPointF &at, Qt::MouseButtons buttons)
{
	QGraphicsSceneMouseEvent event(type);
	event.setButton(Qt::LeftButton);
	event.setButtons(buttons);
	event.setScenePos(at);
	if (type == QEvent::GraphicsSceneMousePress) tool.mousePressEvent(&event);
	else if (type == QEvent::GraphicsSceneMouseMove) tool.mouseMoveEvent(&event);
	else tool.mouseReleaseEvent(&event);
}

/// Press at @a from, move to @a to in ten steps, release: one drag
static void drag(DiagramEventAddElement &tool, const QPointF &from, const QPointF &to)
{
	mouse(tool, QEvent::GraphicsSceneMouseMove, from, Qt::NoButton);
	mouse(tool, QEvent::GraphicsSceneMousePress, from, Qt::LeftButton);
	for (int i = 1; i <= 10; ++i)
		mouse(tool, QEvent::GraphicsSceneMouseMove, from + (to - from) * i / 10, Qt::LeftButton);
	mouse(tool, QEvent::GraphicsSceneMouseRelease, to, Qt::NoButton);
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	std::freopen(argv[2], "w", stdout);

	QTemporaryDir settings;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
	QCoreApplication::setOrganizationName("QETAutoConnectDragRegression");
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");

	try
	{
		QETProject project(QString::fromLocal8Bit(argv[1]));
		check(project.state() == QETProject::Ok, "fixture opens");
		check(project.liveAutoConnect(), "the project keeps its automatic connections up to date");
		project.buildFolios(project.diagrams());
			//As the editor does: the placing tool finds the project's
			//elements through it
		QETApp::registerProject(&project);
		Diagram *diagram = project.diagrams().first();
		const QList<Element *> top = elementsOf(diagram, "abgang");
		const QList<Element *> right = elementsOf(diagram, "klemme");
		check(top.size() == 6 && right.size() == 3, "fixture: six terminals at the top, three on the right");
		const int by_hand = diagram->conductors().size();

		// Which lines a level drag crosses: not the one drawn by hand.
		{
			ElementsLocation corner("embed://import/winkel.elmt", &project);
			DiagramEventAddElement tool(corner, diagram);
			check(tool.isRunning(), "the placing tool starts with the corner");
			Element *sample = nullptr;
			for (Element *e : diagram->elements())
				if (e->location().fileName() == "winkel.elmt") sample = e;
			const QList<QPointF> at = AutoConnect::dragPlacements(diagram, sample, QLineF(90, 350, 240, 350));
			check(at.size() == 3, "a drag across four lines finds three: the line drawn by hand is not one");
			check(at.value(0) == QPointF(100, 350) && at.value(2) == QPointF(180, 350),
				  "each copy stands on its line, on the drag");
		}

		// The corner, dragged diagonally: one per line, staggered, each
		// joined from the top to the right, all in one undo step.
		const int steps = diagram->undoStack().count();
		{
			ElementsLocation corner("embed://import/winkel.elmt", &project);
			DiagramEventAddElement tool(corner, diagram);
			drag(tool, QPointF(90, 300), QPointF(190, 200));
			check(count(diagram, "winkel") == 3, "a diagonal drag places three corners");
			check(diagram->undoStack().count() == steps + 1, "in one undo step");

			// Escape during a drag: no copies, the tool goes on.
			mouse(tool, QEvent::GraphicsSceneMousePress, QPointF(90, 600), Qt::LeftButton);
			mouse(tool, QEvent::GraphicsSceneMouseMove, QPointF(300, 600), Qt::LeftButton);
			QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
			tool.keyPressEvent(&escape);
			check(tool.isRunning(), "Escape gives up the drag, the tool goes on");
			mouse(tool, QEvent::GraphicsSceneMouseRelease, QPointF(300, 600), Qt::NoButton);
		}
		const QList<Element *> corners = elementsOf(diagram, "winkel");
		check(corners.size() == 3, "the tool's own preview is gone with it");
		check(corners[0]->pos() == QPointF(100, 290) && corners[1]->pos() == QPointF(140, 250)
			  && corners[2]->pos() == QPointF(180, 210), "the corners stand where the drag crosses each line");
		bool all = true;
		for (int i = 0; i < 3; ++i)
			all &= joined(top[i], corners[i]) && joined(corners[i], right[2 - i]);
		check(all, "each corner joins its line from the top to its terminal on the right");
		check(automaticWires(diagram) == 6, "six automatic wires");
		check(diagram->conductors().size() == by_hand + 6, "the wire drawn by hand is untouched");

		diagram->undoStack().undo();
		check(elementsOf(diagram, "winkel").isEmpty() && automaticWires(diagram) == 0,
			  "one undo takes the corners and their wires away");
		diagram->undoStack().redo();
		check(count(diagram, "winkel") == 3 && automaticWires(diagram) == 6, "redo brings them back");

		// A T-junction across the same lines splits each of them.
		{
			ElementsLocation t("embed://import/t.elmt", &project);
			DiagramEventAddElement tool(t, diagram);
			drag(tool, QPointF(90, 150), QPointF(190, 150));
		}
		const QList<Element *> ts = elementsOf(diagram, "t");
		check(ts.size() == 3, "a level drag places three T-junctions");
		all = true;
		for (int i = 0; i < 3; ++i)
			all &= joined(top[i], ts[i]) && joined(ts[i], corners[i]) && !joined(top[i], corners[i]);
		check(all, "each T-junction splits its line: top to T, T to corner");
		check(automaticWires(diagram) == 9, "nine automatic wires");

		// Folio reports, turned to face up, end two free lines.
		{
			ElementsLocation report("embed://import/verweis.elmt", &project);
			DiagramEventAddElement tool(report, diagram);
			QKeyEvent space(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
			tool.keyPressEvent(&space);	//its terminal looked left, now up
			drag(tool, QPointF(290, 500), QPointF(350, 500));
		}
		const QList<Element *> reports = elementsOf(diagram, "verweis");
		check(reports.size() == 2, "a drag places two folio reports, turned to face up");
		check(joined(top[4], reports[0]) && joined(top[5], reports[1]), "each ends its line");

		std::printf("CLICKS: corners %d mouse presses for 3 lines (by hand: 3 placed + 6 wires drawn = 9)\n", 1);
		QETApp::unregisterProject(&project);
		std::puts("PASS");
	}
	catch (const std::exception &error)
	{
		std::printf("FAIL: %s\n", error.what());
		std::fflush(stdout);
		return 1;
	}
	std::fflush(stdout);
	return 0;
}
