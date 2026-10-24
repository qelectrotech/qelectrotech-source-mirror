// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "bridge/bridge.h"
#include "bridge/bridgemanager.h"
#include "diagram.h"
#include "properties/elementdata.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"

#include <QDomDocument>

/**
	The bridge drawn over terminals: the object behind the line (what it
	holds, what it may hold), which terminals a line about to be drawn
	crosses, and which parallel track a new bridge takes at them.

	fixtures/bridge_tracks.qet is every_feature.qet with two more
	terminals in the strip X1: four terminals in a row, so the tracking
	of a bridge across a strip is exercised the way it is built in a
	cabinet -- the bar lies across the terminals it does not land on as
	well, and takes its parallel track there too. The two new symbols
	bring terminal ids of their own: the ids name terminals for the
	conductors and must be unique on a folio, an id used twice makes the
	whole symbol drop out when the folio is read.
*/
class tst_bridgetool : public QObject
{
	Q_OBJECT

	//The terminals of the fixture's first strip which belong to a
	//symbol of the drawing
	static QVector<QSharedPointer<RealTerminal>> stripTerminals(QETProject &project)
	{
		QVector<QSharedPointer<RealTerminal>> with_element;
		const QVector<TerminalStrip *> strips = project.terminalStrip();
		if (strips.isEmpty()) return with_element;
		for (const QSharedPointer<RealTerminal> &real_t : strips.first()->realTerminals())
		{
			if (real_t && real_t->element()) with_element << real_t;
		}
		return with_element;
	}

	//The terminal of @p real_t as the bridge names it
	static BridgeTerminal terminalOf(const QSharedPointer<RealTerminal> &real_t)
	{
		BridgeTerminal terminal;
		Element *element = real_t->element();
		if (!element) return terminal;
		terminal.element = element->uuid();
		if (!element->terminals().isEmpty()) {
			terminal.terminal = element->terminals().first()->name();
		}
		return terminal;
	}

private slots:
	//The shipped default: a bridge is black, holds as many terminals as
	//the user draws, and its track is worked out when it is drawn
	void bridgeDefaultsAreTheShippedOnes()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		Bridge *bridge = project.newBridge();
		QVERIFY(bridge);
		QVERIFY2(bridge->color() == QColor(Qt::black), "a bridge is black unless the user says otherwise");
		QCOMPARE(bridge->maxConnections(), Bridge::unlimited_connections);
		QCOMPARE(bridge->column(), 0);
		QCOMPARE(bridge->terminalCount(), 0);
		QVERIFY(!bridge->wouldExceed(100));
		QVERIFY(!bridge->wouldExceed(1000));
	}

	//What a bridge holds: its terminals once each, its sections, and all
	//of it comes back from the file unchanged
	void bridgeHoldsItsTerminalsAndSections()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		const auto terminals = stripTerminals(project);
		QVERIFY2(terminals.size() >= 2, "the fixture has the terminals of the strip X1");

		Bridge *bridge = project.newBridge();
		const BridgeTerminal first = terminalOf(terminals.at(0));
		const BridgeTerminal second = terminalOf(terminals.at(1));
		bridge->addTerminal(first);
		bridge->addTerminal(first); //twice is once
		bridge->addTerminal(second);
		QCOMPARE(bridge->terminalCount(), 2);
		QVERIFY(bridge->hasTerminal(first));
		QVERIFY(bridge->hasTerminal(second));

		BridgePartData part;
		part.uuid = QUuid::createUuid();
		part.diagram = project.diagrams().first()->uuid();
		part.p1 = QPointF(10, 20);
		part.p2 = QPointF(30, 40);
		bridge->addPart(part);
		bridge->addPart(part); //twice is once
		QCOMPARE(bridge->parts().size(), 1);
		bridge->setPartGeometry(part.uuid, QPointF(50, 60), part.p2);
		QCOMPARE(bridge->parts().first().p1, QPointF(50, 60));

			//What is written is what is read
		QDomDocument document;
		const QDomElement written = bridge->toXml(document);
		Bridge read_back;
		read_back.fromXml(written);
		QCOMPARE(read_back.uuid(), bridge->uuid());
		QCOMPARE(read_back.color(), bridge->color());
		QCOMPARE(read_back.maxConnections(), bridge->maxConnections());
		QCOMPARE(read_back.column(), bridge->column());
		QCOMPARE(read_back.terminals(), bridge->terminals());
		QCOMPARE(read_back.parts().size(), 1);
		QCOMPARE(read_back.parts().first().uuid, part.uuid);
		QCOMPARE(read_back.parts().first().p1, QPointF(50, 60));

			//And it comes back with the project: adding it is one step,
			//taking it away leaves nothing behind
		QVERIFY(project.bridges().contains(bridge));
		QVERIFY(project.removeBridge(bridge));
		QVERIFY(!project.bridges().contains(bridge));
	}

	//The one limit which is never crossed: a bridge which cannot be
	//built in the cabinet is not drawn either
	void theLimitOfABridgeIsNeverCrossed()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		const auto terminals = stripTerminals(project);
		QVERIFY(terminals.size() >= 2);

		Bridge *bridge = project.newBridge();
		bridge->setMaxConnections(2);
		bridge->addTerminal(terminalOf(terminals.at(0)));
		QVERIFY(!bridge->wouldExceed(1));
		bridge->addTerminal(terminalOf(terminals.at(1)));
		QVERIFY2(bridge->wouldExceed(1), "two is all it takes");
		QVERIFY(bridge->wouldExceed(2));

		Bridge *unlimited = project.newBridge();
		QVERIFY(!unlimited->wouldExceed(1000));
	}

	//A line about to be drawn crosses the terminals it lands on, and the
	//first one crossed is the first one of the bridge
	void crossedTerminalsAreFoundAlongTheLine()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		QVERIFY(!project.diagrams().isEmpty());

		const auto terminals = stripTerminals(project);
		QVERIFY2(terminals.size() >= 2, "the fixture has the terminals of the strip X1");
		Element *first = terminals.first()->element();
		Element *second = terminals.last()->element();
		QVERIFY(first && second);

		const QLineF line(first->scenePos(), second->scenePos());
		const QList<BridgeCrossing> crossings = BridgeManager::crossedTerminals(project.diagrams().first(), line);
		QVERIFY2(crossings.size() >= 2, "the line crosses the terminals it runs over");

		int at_first = -1;
		int at_second = -1;
		for (int i = 0 ; i < crossings.size() ; ++i) {
			if (crossings.at(i).element == first) at_first = i;
			if (crossings.at(i).element == second) at_second = i;
		}
		QVERIFY2(at_first >= 0, "the terminal at one end is crossed");
		QVERIFY2(at_second >= 0, "the terminal at the other end is crossed");
		QVERIFY2(at_first < at_second, "the terminals are crossed in the order the line runs over them");
	}

	//Two bridges side by side do not share a track: a bridge lies across
	//the terminals between the ones it lands on as well
	void parallelBridgesTakeTheirOwnColumn()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		const auto terminals = stripTerminals(project);
		QVERIFY2(terminals.size() >= 4, "the fixture has the terminals of the strip X1");

		const BridgeTerminal t0 = terminalOf(terminals.at(0));
		const BridgeTerminal t1 = terminalOf(terminals.at(1));
		const BridgeTerminal t2 = terminalOf(terminals.at(2));
		const BridgeTerminal t3 = terminalOf(terminals.at(3));

			//The first bridge over 0..2 takes the first track
		Bridge *first = project.newBridge();
		first->addTerminal(t0);
		first->addTerminal(t2);
		first->setColumn(BridgeManager::freeColumn(&project, first->terminals()));
		QCOMPARE(first->column(), 1);

			//The second over 1..3 lies across 2 as well, where the first
			//one already runs: it takes the second track
		Bridge *second = project.newBridge();
		second->addTerminal(t1);
		second->addTerminal(t3);
		second->setColumn(BridgeManager::freeColumn(&project, second->terminals()));
		QVERIFY2(second->column() == 2, "the two bridges run side by side");

			//And a third one over 3 alone takes the first track again:
			//the first bridge stops before that terminal
		Bridge *third = project.newBridge();
		third->addTerminal(t3);
		QCOMPARE(BridgeManager::freeColumn(&project, third->terminals()), 1);

			//One over 1 alone lies under both bridges already there and
			//takes what is left
		Bridge *fourth = project.newBridge();
		fourth->addTerminal(t1);
		QCOMPARE(BridgeManager::freeColumn(&project, fourth->terminals()), 3);
	}

	//Four bridges are all a terminal takes: the four bridge points
	void fourBridgesFillATerminal()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		const auto terminals = stripTerminals(project);
		QVERIFY(!terminals.isEmpty());
		const BridgeTerminal terminal = terminalOf(terminals.first());

		for (int column = 1 ; column <= Bridge::max_columns ; ++column) {
			Bridge *bridge = project.newBridge();
			bridge->addTerminal(terminal);
			bridge->setColumn(column);
			QCOMPARE(bridge->column(), column);
		}
		QVERIFY2(BridgeManager::freeColumn(&project, {terminal}) == 0,
				 "no track is left at that terminal");
	}
};

QET_APP_TEST_MAIN(tst_bridgetool)
#include "tst_bridgetool.moc"
