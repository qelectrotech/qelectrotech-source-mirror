// SPDX-License-Identifier: GPL-2.0-or-later
// Automatic connections kept up to date (AutoConnect), linked against the
// application's objects.
//
// Three terminals A, B, C stand in a row, two more, D and E, are joined by
// a wire drawn by hand. With the project's live automatic connections on:
// - connecting the folio joins A-B and B-C, and leaves D-E as it is;
// - moving B out of the row removes its wires and joins A to C, since they
//   are still in line; undo and redo bring both states back whole;
// - moving B back between them breaks A-C again and joins A-B and B-C;
// - a small interruption symbol moved between A and B separates them;
// - moving E out of line keeps the wire drawn by hand;
// - deleting B joins A and C in the same step;
// - the flag is written to the file.
#include <QApplication>
#include <QDomDocument>
#include <QFontDatabase>
#include <QSettings>
#include <QTemporaryDir>
#include <cstdio>
#include <stdexcept>

#include "../../sources/autoconnect.h"
#include "../../sources/diagram.h"
#include "../../sources/diagramcontent.h"
#include "../../sources/elementsmover.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/qetgraphicsitem/conductor.h"
#include "../../sources/qetgraphicsitem/element.h"
#include "../../sources/qetgraphicsitem/terminal.h"
#include "../../sources/undocommand/deleteqgraphicsitemcommand.h"

static void check(bool value, const char *message)
{
	if (!value) throw std::runtime_error(message);
	std::printf("ok: %s\n", message);
}

static Terminal *terminal(Element *e, const char *name)
{
	for (Terminal *t : e->terminals())
		if (t->name() == QLatin1String(name)) return t;
	throw std::runtime_error("terminal missing");
}

static Conductor *wire(Terminal *a, Terminal *b)
{
	for (Conductor *c : a->conductors())
		if ((c->terminal1 == a && c->terminal2 == b) || (c->terminal1 == b && c->terminal2 == a))
			return c;
	return nullptr;
}

static void move(Diagram *diagram, Element *e, const QPointF &by)
{
	diagram->clearSelection();
	e->setSelected(true);
	ElementsMover &mover = diagram->elementsMover();
	mover.beginMovement(diagram);
	mover.continueMovement(by);
	mover.endMovement();
	diagram->clearSelection();
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	std::freopen(argv[2], "w", stdout);

	QTemporaryDir settings;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
	QCoreApplication::setOrganizationName("QETAutoConnectRegression");
	QETProject::setBackupEnabled(false);
	QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");

	try
	{
		QETProject project(QString::fromLocal8Bit(argv[1]));
		check(project.state() == QETProject::Ok, "fixture opens");
		check(project.liveAutoConnect(), "the project keeps its automatic connections up to date");
		project.buildFolios(project.diagrams());
		Diagram *diagram = project.diagrams().first();

		QList<Element *> row, pair;
		Element *stop = nullptr;
		for (Element *e : diagram->elements()) {
			if (e->terminals().isEmpty()) stop = e;
			else (e->pos().y() < 300 ? row : pair) << e;
		}
		std::sort(row.begin(), row.end(), [](Element *a, Element *b) { return a->pos().x() < b->pos().x(); });
		std::sort(pair.begin(), pair.end(), [](Element *a, Element *b) { return a->pos().x() < b->pos().x(); });
		check(row.size() == 3 && pair.size() == 2 && stop, "fixture: a row of three, a wired pair, an interruption");
		Element *A = row[0], *B = row[1], *C = row[2], *D = pair[0], *E = pair[1];
		Terminal *aE = terminal(A, "E"), *bW = terminal(B, "W"), *bE = terminal(B, "E"), *cW = terminal(C, "W");
		Conductor *by_hand = wire(terminal(D, "E"), terminal(E, "W"));
		check(by_hand && !by_hand->isAutomatic(), "D-E is a wire drawn by hand");

		check(AutoConnect::partner(aE) == bW, "A looks at B");
		check(AutoConnect::partner(bE) == cW, "B looks at C");

		auto *connect = new QUndoCommand("connect");
		check(AutoConnect::connectWholeDiagram(diagram, connect) == 2, "connecting the folio makes two wires");
		diagram->undoStack().push(connect);
		check(wire(aE, bW) && wire(aE, bW)->isAutomatic(), "A-B joined automatically");
		check(wire(bE, cW) && wire(bE, cW)->isAutomatic(), "B-C joined automatically");
		check(diagram->conductors().size() == 3, "D-E untouched, nothing else made");

		// The middle one out of the row: A and C are still in line.
		move(diagram, B, QPointF(0, -40));
		check(!wire(aE, bW) && !wire(bE, cW), "B moved out: its automatic wires are gone");
		check(wire(aE, cW) && wire(aE, cW)->isAutomatic(), "A and C join, still in line");
		check(diagram->conductors().size() == 2, "two wires: A-C and D-E");

		diagram->undoStack().undo();
		check(B->pos() == QPointF(125, 200), "undo puts B back");
		check(wire(aE, bW) && wire(bE, cW) && !wire(aE, cW), "undo brings A-B and B-C back, A-C gone");
		diagram->undoStack().redo();
		check(wire(aE, cW) && !wire(aE, bW) && !wire(bE, cW), "redo: A-C again");

		// Back into the row: B's body stands between A and C.
		move(diagram, B, QPointF(0, 40));
		check(!wire(aE, cW), "B back: A-C broken");
		check(wire(aE, bW) && wire(bE, cW), "B back: A-B and B-C again");

		// An interruption point between A and B.
		const QPointF gap((aE->dockConductor().x() + bW->dockConductor().x()) / 2, aE->dockConductor().y());
		move(diagram, stop, gap - stop->pos());
		check(!wire(aE, bW), "an interruption point separates A and B");
		check(wire(bE, cW), "B-C stays");
		check(AutoConnect::partner(aE) == nullptr, "A finds no partner past the interruption");
		move(diagram, stop, QPointF(400, 300));
		check(wire(aE, bW), "interruption gone: A-B again");

		// A wire drawn by hand is never touched.
		move(diagram, E, QPointF(0, 40));
		check(wire(terminal(D, "E"), terminal(E, "W")) == by_hand, "moving E keeps the wire drawn by hand");

		// Deleting B joins A and C in the same step.
		diagram->clearSelection();
		B->setSelected(true);
		DiagramContent dc(diagram);
		auto *remove = new QUndoCommand("delete");
		new DeleteQGraphicsItemCommand(diagram, dc, remove);
		AutoConnect::update(diagram, {}, {}, remove, {B});
		diagram->undoStack().push(remove);
		check(!B->scene(), "B deleted");
		check(wire(aE, cW) && wire(aE, cW)->isAutomatic(), "deleting B joins A and C");
		diagram->undoStack().undo();
		check(B->scene() && wire(aE, bW) && wire(bE, cW) && !wire(aE, cW), "undo brings B and its wires back");

		// The flag is part of the file.
		QDomDocument doc;
		QHash<Terminal *, int> ids;
		const QDomElement xml = wire(aE, bW)->toXml(doc, ids);
		check(xml.attribute("automatic") == "true", "an automatic wire is written as automatic");
		check(!by_hand->toXml(doc, ids).hasAttribute("automatic"), "a wire drawn by hand is not");
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
