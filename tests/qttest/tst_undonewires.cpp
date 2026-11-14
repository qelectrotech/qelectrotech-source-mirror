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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "undocommand/addgraphicsobjectcommand.h"
#include <QDomDocument>
#include <QPointer>
#include <QUndoCommand>
#include <QtTest>

/**
	One undo step that adds a symbol and wires to it, as placing with
	automatic wires, replacing a symbol or setting it from its device type
	do. Once undone, the step is dropped by the next action or by closing
	the project, and it deletes the symbol before the wires. Before, each
	wire then read the deleted terminals: such a read seldom shows, but a
	build with AddressSanitizer (-fsanitize=address) fails this test.
*/
class tst_undonewires : public QObject
{
	Q_OBJECT

		/// A symbol with two terminals, not on a folio yet
	static Element *symbol(QETProject *project)
	{
		auto *collection = project->embeddedElementCollection();
		const QString name = QStringLiteral("test_simple");
		if (!collection->exist(QStringLiteral("import/%1.elmt").arg(name))) {
			QDomDocument definition;
			definition.setContent(
				QString(R"(<definition type="element" link_type="simple" uuid="%1" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test</name></names>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%2"/><terminal x="0" y="20" orientation="s" uuid="%3"/>
</description></definition>)")
					.arg(QUuid::createUuid().toString(), QUuid::createUuid().toString(),
						 QUuid::createUuid().toString()));
			if (!collection->addElementDefinition(QStringLiteral("import"), name,
												  definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(name),
							 project),
			nullptr, &state);
		if (state || !element) {
			delete element;
			return nullptr;
		}
		return element;
	}

		/// A symbol on the folio, a step that adds a second one wired to
		/// it, and that step undone
	static QPointer<Conductor> undoneStep(Diagram *diagram, Terminal **kept)
	{
		Element *placed = symbol(diagram->project());
		Element *fresh = symbol(diagram->project());
		if (!placed || !fresh) return nullptr;
		diagram->addItem(placed);
		placed->setPos(100, 300);
		*kept = placed->terminals().at(0);

		auto *step = new QUndoCommand(QStringLiteral("place"));
		new AddGraphicsObjectCommand(fresh, diagram, QPointF(100, 100), step);
		auto *wire = new Conductor(fresh->terminals().at(1), *kept);
		new AddGraphicsObjectCommand(wire, diagram, QPointF(), step);
		diagram->undoStack().push(step);
		if ((*kept)->conductors().size() != 1) return nullptr;
		diagram->undoStack().undo();
		return wire;
	}

private slots:
	void initTestCase() { QETProject::setBackupEnabled(false); }

	void theNextActionDropsTheUndoneWires()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		Terminal *kept = nullptr;
		const QPointer<Conductor> wire = undoneStep(diagram, &kept);
		QVERIFY(wire);
		QVERIFY(kept->conductors().isEmpty());

		diagram->undoStack().push(new QUndoCommand(QStringLiteral("next")));
		QVERIFY(wire.isNull());
		QVERIFY(kept->conductors().isEmpty());
	}

	void closingTheProjectDropsTheUndoneWires()
	{
		QPointer<Conductor> wire;
		{
			QETProject project;
			Diagram *diagram = project.addNewDiagram();
			Terminal *kept = nullptr;
			wire = undoneStep(diagram, &kept);
			QVERIFY(wire);
		}
		QVERIFY(wire.isNull());
	}

		//The wires of a deleted symbol still leave the terminals at their
		//other end, and a deleted wire leaves both of its terminals
	void wiresStillLeaveTheirTerminals()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		Element *one = symbol(&project);
		Element *two = symbol(&project);
		QVERIFY(one && two);
		diagram->addItem(one);
		diagram->addItem(two);
		Terminal *a = one->terminals().at(1);
		Terminal *b = two->terminals().at(0);

		auto *wire = new Conductor(a, b);
		diagram->addItem(wire);
		QCOMPARE(a->conductors().size(), 1);
		delete wire;
		QVERIFY(a->conductors().isEmpty());
		QVERIFY(b->conductors().isEmpty());

		const QPointer<Conductor> other = new Conductor(a, b);
		diagram->addItem(other);
		QCOMPARE(b->conductors().size(), 1);
		delete one;
		QVERIFY(other.isNull());
		QVERIFY(b->conductors().isEmpty());
	}
};

QET_APP_TEST_MAIN(tst_undonewires)
#include "tst_undonewires.moc"
