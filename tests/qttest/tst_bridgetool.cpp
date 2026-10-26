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
#include "TerminalStrip/UndoCommand/sortterminalstripcommand.h"
#include "TerminalStrip/physicalterminal.h"
#include "TerminalStrip/terminalstripbridge.h"
#include "TerminalStrip/ui/terminalstripmodel.h"
#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "bridge/bridge.h"
#include "bridge/addbridgecommand.h"
#include "bridge/bridgecreatedialog.h"
#include "bridge/bridgemanager.h"
#include "bridge/bridgepart.h"
#include "bridge/bridgeundocommands.h"
#include "diagram.h"
#include "properties/elementdata.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "conductorproperties.h"

#include <QCheckBox>
#include <QDomDocument>
#include <QListWidget>
#include <QSpinBox>
#include <QTabWidget>
#include <QUndoStack>

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
			terminal.terminal = element->terminals().first()->stableUuid().toString();
		}
		return terminal;
	}

	//The folio a terminal symbol of the drawing sits on
	static Diagram *diagramOf(QETProject &project, Element *element)
	{
		for (Diagram *diagram : project.diagrams()) {
			if (diagram->elements().contains(element)) return diagram;
		}
		return nullptr;
	}

	//The terminals a line from @p from to @p to crosses, the way the
	//tool sees them while the line is pulled
	static QList<BridgeCrossing> crossingsOver(QETProject &project,
											   const QSharedPointer<RealTerminal> &from,
											   const QSharedPointer<RealTerminal> &to)
	{
		Element *first = from->element();
		Element *second = to->element();
		if (!first || !second) return {};
		Diagram *diagram = diagramOf(project, first);
		if (!diagram) return {};
		return BridgeManager::crossedTerminals(diagram, QLineF(first->scenePos(), second->scenePos()));
	}

private slots:

	//Closing a project takes its symbols down with it, and their
	//terminals say "a wire left" on the way out. What a symbol fills
	//must not be asked about wires while its texts are being deleted --
	//they are gone before the terminals, and asking them takes the whole
	//application down with the project
	void closingAProjectTakesItsSymbolsDownWhole()
	{
		const QString fixture = QFINDTESTDATA("fixtures/linked_reports.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		QVERIFY(!project.diagrams().isEmpty());

			//A report symbol with texts of its own and a wire on one of
			//its terminals: the very chain which fills those texts
		Element *report = nullptr;
		for (Diagram *diagram : project.diagrams()) {
			for (Element *element : diagram->elements()) {
				if ((element->linkType() & Element::AllReport)
					&& !element->dynamicTextItems().isEmpty()) {
					report = element;
					break;
				}
			}
			if (report) break;
		}
		if (!report) {
			QSKIP("the fixture has no report symbol with texts");
		}
		QVERIFY(!report->terminals().isEmpty());

		Diagram *diagram = report->diagram();
		Terminal *first = report->terminals().first();
		Terminal *second = nullptr;
		for (Element *element : diagram->elements()) {
			if (element == report) continue;
			for (Terminal *terminal : element->terminals()) {
				if (!terminal->conductors().isEmpty()) {
					second = terminal;
					break;
				}
			}
			if (second) break;
		}
		if (second) {
			auto *conductor = new Conductor(first, second);
			diagram->addItem(conductor);
		}

			//The symbol goes with its texts still asking about wires
		delete report;
		QVERIFY2(true, "the symbol went down without taking the application with it");
	}
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
		QCOMPARE(bridge->lineWidth(), Bridge::default_line_width);
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
		bridge->setLineWidth(2.5);
		bridge->setTensionText(QStringLiteral("24VAC"));
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
		QCOMPARE(read_back.lineWidth(), bridge->lineWidth());
		QCOMPARE(read_back.tensionText(), QStringLiteral("24VAC"));
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

	//A new bridge: black, as many terminals as the user draws, and
	//nothing to type in but that
	void newBridgeOffersTheShippedDefaults()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY(terminals.size() >= 2);

		const QList<BridgeCrossing> crossings = crossingsOver(project, terminals.at(0), terminals.at(1));
		QVERIFY2(crossings.size() >= 2, "the line crosses the terminals it runs over");

		BridgeCreateDialog dialog(diagramOf(project, terminals.at(0)->element()), crossings);
		QVERIFY2(dialog.bridge() == nullptr, "a new bridge is offered first");
		QCOMPARE(dialog.newMaxConnections(), Bridge::unlimited_connections);
		QCOMPARE(dialog.newLineWidth(), Bridge::default_line_width);
		QCOMPARE(dialog.newColor(), QColor(Qt::black));
		QVERIFY2(dialog.problem().isEmpty(), "nothing speaks against it");
	}

	//A bridge which cannot take what was crossed is struck through and
	//cannot be chosen at all -- not with a warning, not at all
	void fullAndSmallBridgesAreStruckThrough()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY(terminals.size() >= 3);

		const QList<BridgeCrossing> crossings = crossingsOver(project, terminals.at(0), terminals.at(1));
		QVERIFY(crossings.size() >= 2);

		Bridge *good = project.newBridge();
		QVERIFY(good->maxConnections() == Bridge::unlimited_connections);

		Bridge *small = project.newBridge();
		small->setMaxConnections(crossings.size());
		small->addTerminal(terminalOf(terminals.at(2)));

		Bridge *full = project.newBridge();
		full->setMaxConnections(crossings.size());
		full->addTerminal(terminalOf(terminals.at(0)));
		full->addTerminal(terminalOf(terminals.at(1)));

		BridgeCreateDialog dialog(diagramOf(project, terminals.at(0)->element()), crossings);
		auto *list = dialog.findChild<QListWidget *>(QStringLiteral("bridge_list"));
		QVERIFY(list);
		QVERIFY2(list->count() == 1, "the two which cannot take them are hidden first");

		dialog.findChild<QCheckBox *>(QStringLiteral("bridge_hide_full_cb"))->setChecked(false);
		dialog.findChild<QCheckBox *>(QStringLiteral("bridge_hide_small_cb"))->setChecked(false);
		QCOMPARE(list->count(), 3);

		int choosable = 0;
		for (int i = 0 ; i < list->count() ; ++i)
		{
			const QListWidgetItem *item = list->item(i);
			const bool struck = item->font().strikeOut();
			const bool selectable = item->flags() & Qt::ItemIsSelectable;
			QVERIFY2(struck == !selectable, "struck through means not choosable");
			if (selectable) ++choosable;
		}
		QVERIFY2(choosable == 1, "only the bridge which can take them is choosable");
	}

	//Taking a bridge which can hold them: the line lands on it
	void acceptLandsOnTheChosenBridge()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY(terminals.size() >= 2);

		const QList<BridgeCrossing> crossings = crossingsOver(project, terminals.at(0), terminals.at(1));
		QVERIFY(crossings.size() >= 2);
		Bridge *existing = project.newBridge();

		BridgeCreateDialog dialog(diagramOf(project, terminals.at(0)->element()), crossings);
		dialog.findChild<QTabWidget *>(QStringLiteral("bridge_tabs"))->setCurrentIndex(1);
		auto *list = dialog.findChild<QListWidget *>(QStringLiteral("bridge_list"));
		QVERIFY(list);
		QCOMPARE(list->count(), 1);
		list->setCurrentRow(0);
		QVERIFY2(dialog.problem().isEmpty(), "that bridge can take them");

		dialog.accept();
		QCOMPARE(dialog.result(), int(QDialog::Accepted));
		QCOMPARE(dialog.bridge(), existing);
	}

	//A bridge which cannot hold what was crossed is refused: it cannot
	//be built in the cabinet, so it is not drawn at all
	void aLimitBelowWhatWasCrossedIsRefused()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY(terminals.size() >= 2);

		const QList<BridgeCrossing> crossings = crossingsOver(project, terminals.at(0), terminals.at(1));
		QVERIFY(crossings.size() >= 2);

		BridgeCreateDialog dialog(diagramOf(project, terminals.at(0)->element()), crossings);
		dialog.findChild<QSpinBox *>(QStringLiteral("bridge_max_sb"))->setValue(1);
		QVERIFY2(!dialog.problem().isEmpty(),
				 "a bridge which cannot take the terminals is refused");

		dialog.findChild<QSpinBox *>(QStringLiteral("bridge_max_sb"))
				->setValue(Bridge::unlimited_connections);
		QVERIFY2(dialog.problem().isEmpty(), "no limit is no problem");
	}

	//A line stretched by hand gives the bridge the terminals it crosses
	//now and takes back the ones it no longer runs over -- in one step,
	//so one undo puts the line and its connections back together
	void stretchingTheLineGivesAndTakesTerminals()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY(terminals.size() >= 3);
		Diagram *folio = diagramOf(project, terminals.at(0)->element());
		QVERIFY(folio);

		BridgePartData data;
		data.uuid = QUuid::createUuid();
		data.diagram = folio->uuid();
		data.p1 = terminals.at(0)->element()->scenePos();
		data.p2 = terminals.at(1)->element()->scenePos();

		Bridge *bridge = project.newBridge();
		bridge->addPart(data);
		bridge->setTerminals({terminalOf(terminals.at(0)), terminalOf(terminals.at(1))});

		auto *part = new BridgePart(data);
		part->setBridge(bridge);
		folio->addItem(part);

		const QLineF before_line = part->line();
		const QList<BridgeTerminal> before = bridge->terminals();

			//The end dragged from the second terminal onto the third
		const QLineF after_line(terminals.at(1)->element()->scenePos(),
							   terminals.at(2)->element()->scenePos());
		part->setLine(after_line.p1(), after_line.p2());
		const QList<BridgeTerminal> after = BridgeManager::terminalsOfBridge(&project, bridge);

		QVERIFY2(after.contains(terminalOf(terminals.at(2))),
				 "the line lands on the terminal it runs over now");
		QVERIFY2(!after.contains(terminalOf(terminals.at(0))),
				 "and drops the one it no longer runs over");

		folio->undoStack().push(new ChangeBridgeGeometryCommand(
									bridge, part, before_line, after_line, before, after,
									BridgeInformation(), BridgeInformation()));
		QCOMPARE(part->line(), after_line);
		QCOMPARE(bridge->terminals(), after);

		folio->undoStack().undo();
		QCOMPARE(part->line(), before_line);
		QCOMPARE(bridge->terminals(), before);
	}

	//What the window of a bridge changes -- colour and how many
	//terminals it may hold -- is one step as well
	void changingThePropertiesOfABridgeIsOneStep()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY(!terminals.isEmpty());
		Diagram *folio = diagramOf(project, terminals.at(0)->element());
		QVERIFY(folio);

		Bridge *bridge = project.newBridge();
		bridge->addTerminal(terminalOf(terminals.first()));
		QCOMPARE(bridge->color(), QColor(Qt::black));
		QCOMPARE(bridge->maxConnections(), Bridge::unlimited_connections);
		QCOMPARE(bridge->lineWidth(), Bridge::default_line_width);

		bridge->setName(QStringLiteral("B1"));
		folio->undoStack().push(new ChangeBridgePropertiesCommand(
									bridge, QColor(Qt::red), 3, 2.5, QStringLiteral("24V")));
		QCOMPARE(bridge->color(), QColor(Qt::red));
		QCOMPARE(bridge->maxConnections(), 3);
		QCOMPARE(bridge->lineWidth(), 2.5);
		QCOMPARE(bridge->name(), QStringLiteral("24V"));

		folio->undoStack().undo();
		QCOMPARE(bridge->color(), QColor(Qt::black));
		QCOMPARE(bridge->maxConnections(), Bridge::unlimited_connections);
		QCOMPARE(bridge->lineWidth(), Bridge::default_line_width);
		QCOMPARE(bridge->name(), QStringLiteral("B1"));
	}

	//A bridge which already holds terminals may not be given a limit
	//below them: that would cut off terminals which are on it
	void anEditMayNotCutOffTheTerminalsAlreadyOnTheBridge()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY(terminals.size() >= 2);

		Bridge *bridge = project.newBridge();
		bridge->addTerminal(terminalOf(terminals.at(0)));
		bridge->addTerminal(terminalOf(terminals.at(1)));

		BridgeCreateDialog dialog(bridge);
		auto *max_sb = dialog.findChild<QSpinBox *>(QStringLiteral("bridge_max_sb"));
		QVERIFY(max_sb);
		QCOMPARE(max_sb->value(), Bridge::unlimited_connections);

		max_sb->setValue(1);
		QVERIFY2(!dialog.problem().isEmpty(),
				 "a limit below what is already on the bridge is refused");
		max_sb->setValue(2);
		QVERIFY2(dialog.problem().isEmpty(), "exactly what it holds is allowed");
		max_sb->setValue(Bridge::unlimited_connections);
		QVERIFY(dialog.problem().isEmpty());
	}

	//Deleting a line of a bridge deletes that line and nothing else: a
	//bridge with other lines left goes on with them, and only the last
	//line takes the bridge with it -- what draws nothing is no bridge
	void deletingALineLeavesTheRestOfTheBridge()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		QVERIFY2(project.diagrams().size() >= 2, "the fixture has more than one folio");

		Diagram *first_folio = project.diagrams().at(0);
		Diagram *second_folio = project.diagrams().at(1);

		BridgePartData first_data;
		first_data.uuid = QUuid::createUuid();
		first_data.diagram = first_folio->uuid();
		first_data.p1 = QPointF(100, 100);
		first_data.p2 = QPointF(200, 100);

		BridgePartData second_data;
		second_data.uuid = QUuid::createUuid();
		second_data.diagram = second_folio->uuid();
		second_data.p1 = QPointF(300, 300);
		second_data.p2 = QPointF(400, 300);

		Bridge *bridge = project.newBridge();
		bridge->addPart(first_data);
		bridge->addPart(second_data);

		auto *first_part = new BridgePart(first_data);
		first_part->setBridge(bridge);
		first_folio->addItem(first_part);

		auto *second_part = new BridgePart(second_data);
		second_part->setBridge(bridge);
		second_folio->addItem(second_part);

			//Only the line he chose goes; the bridge draws on with its other
			//line, wherever that one stands
		first_folio->undoStack().push(new RemoveBridgeCommand({first_part}));
		QVERIFY(project.bridges().contains(bridge));
		QVERIFY(!first_part->scene());
		QVERIFY2(second_part->scene(), "the line he did not choose stays");
		QVERIFY(!bridge->hasPart(first_data.uuid));
		QVERIFY(bridge->hasPart(second_data.uuid));

		first_folio->undoStack().undo();
		QVERIFY(bridge->hasPart(first_data.uuid));
		QCOMPARE(first_part->scene(), static_cast<QGraphicsScene *>(first_folio));

			//Both lines gone: the last one takes the bridge with it
		first_folio->undoStack().push(new RemoveBridgeCommand({first_part, second_part}));
		QVERIFY(!project.bridges().contains(bridge));
		QVERIFY(!first_part->scene());
		QVERIFY(!second_part->scene());

		first_folio->undoStack().undo();
		QVERIFY(project.bridges().contains(bridge));
		QCOMPARE(second_part->scene(), static_cast<QGraphicsScene *>(second_folio));
	}

	//A terminal dropped onto a bridge which cannot take one more is
	//refused: what that would build cannot be built in the cabinet, so
	//the terminal stays at the mouse and is dropped somewhere else
	void aTerminalDroppedOnAFullBridgeIsRefused()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY(terminals.size() >= 3);
		Diagram *folio = diagramOf(project, terminals.at(0)->element());
		QVERIFY(folio);

		BridgePartData data;
		data.uuid = QUuid::createUuid();
		data.diagram = folio->uuid();
		data.p1 = terminals.at(0)->element()->scenePos();
		data.p2 = terminals.at(1)->element()->scenePos();

		Bridge *bridge = project.newBridge();
		bridge->setMaxConnections(2);
		bridge->addPart(data);
		bridge->setTerminals({terminalOf(terminals.at(0)), terminalOf(terminals.at(1))});

		Element *third = terminals.at(2)->element();
		QVERIFY(third);

			//Still where it belongs: nothing would be taken on
		QVERIFY(BridgeManager::bridgesOverfilledBy(&project, {third}).isEmpty());

			//Dropped onto the line of a bridge holding all it may hold
		third->setPos(terminals.at(0)->element()->pos());
		QVERIFY2(!BridgeManager::bridgesOverfilledBy(&project, {third}).isEmpty(),
				 "a bridge which cannot take one more refuses the terminal");

			//With room to spare there is nothing to refuse
		bridge->setMaxConnections(Bridge::unlimited_connections);
		QVERIFY(BridgeManager::bridgesOverfilledBy(&project, {third}).isEmpty());
	}

	//What a bridge lands on follows the drawing: a terminal moved off
	//one of its lines drops its connection, and one undo puts both the
	//connection and the terminals back
	void movingATerminalGivesAndTakesItsConnection()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY(terminals.size() >= 2);
		Diagram *folio = diagramOf(project, terminals.at(0)->element());
		QVERIFY(folio);

		BridgePartData data;
		data.uuid = QUuid::createUuid();
		data.diagram = folio->uuid();
		data.p1 = terminals.at(0)->element()->scenePos();
		data.p2 = terminals.at(1)->element()->scenePos();

		Bridge *bridge = project.newBridge();
		bridge->addPart(data);
		bridge->setTerminals({terminalOf(terminals.at(0)), terminalOf(terminals.at(1))});

			//The second terminal walks off the line
		Element *second = terminals.at(1)->element();
		const QPointF where_it_was = second->pos();
		second->setPos(where_it_was + QPointF(0, 400));

		const QList<BridgeTerminal> before = {terminalOf(terminals.at(0)), terminalOf(terminals.at(1))};
		const QList<BridgeTerminal> after = BridgeManager::terminalsOfBridge(&project, bridge);
		QVERIFY2(after.contains(terminalOf(terminals.at(0))), "the terminal which stays keeps its connection");
		QVERIFY2(!after.contains(terminalOf(terminals.at(1))),
				 "the one which walked off the line drops it");

		folio->undoStack().push(new ChangeBridgeTerminalsCommand(
									bridge, before, after,
									BridgeInformation(), BridgeInformation()));
		QCOMPARE(bridge->terminals(), after);
		folio->undoStack().undo();
		QCOMPARE(bridge->terminals(), before);
	}

	//What the bridge carries is the voltage of the wires at its pins --
	//and only that: a bridge crosses no function, so a separated function
	//stays separated however many bridges sit across it
	void aBridgeCarriesTheVoltageOnly()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY(terminals.size() >= 2);
		Diagram *folio = diagramOf(project, terminals.at(0)->element());
		QVERIFY(folio);

		Bridge *bridge = project.newBridge();
		QVERIFY(bridge->tensionText().isEmpty());

			//Nothing to take: nobody is asked and the bridge carries nothing
		bool ok = false;
		const BridgeInformation nothing = BridgeCreateDialog::askForInformation(
					QList<BridgeInformation>(), &ok);
		QVERIFY2(ok, "nothing to take is no question to ask");
		QVERIFY(nothing.tension.isEmpty());

			//One thing to take: nobody is asked and the bridge takes it
		const BridgeInformation only{QStringLiteral("24VDC")};
		const BridgeInformation taken = BridgeCreateDialog::askForInformation(
					{BridgeInformation(), only}, &ok);
		QVERIFY(ok);
		QVERIFY2(taken == only, "a bridge which carries nothing takes what is there");

			//And what the bridge carries is one undo step like the rest
		bridge->setTensionText(taken.tension);
		const BridgeInformation before{bridge->tensionText()};
		const BridgeInformation after{QStringLiteral("230V")};
		folio->undoStack().push(new ChangeBridgeTerminalsCommand(
										bridge, bridge->terminals(), bridge->terminals(),
										before, after));
		QCOMPARE(bridge->tensionText(), QStringLiteral("230V"));
		folio->undoStack().undo();
		QCOMPARE(bridge->tensionText(), before.tension);
	}

	//Bridge and wires are one circle: what the bridge carries goes onto
	//the wires it sits on -- the voltage and nothing else -- and when the
	//wires say something else later the bridge follows them
	void theWiresSayWhatTheBridgeSays()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY(!terminals.isEmpty());
		Diagram *folio = diagramOf(project, terminals.at(0)->element());
		QVERIFY(folio);

			//A terminal with a wire wired to it
		QSharedPointer<RealTerminal> wired;
		for (const QSharedPointer<RealTerminal> &terminal : terminals) {
			Element *element = terminal->element();
			if (element && !element->terminals().isEmpty()
				&& !element->terminals().first()->conductors().isEmpty()) {
				wired = terminal;
				break;
			}
		}
		if (!wired) {
			QSKIP("the fixture has no wired terminal");
		}
		Conductor *conductor = wired->element()->terminals().first()->conductors().first();
		const QString original_tension = conductor->properties().m_tension_protocol;

			//Every wire connected -- at every terminal of the symbol and on
			//through the potential -- and what each of them says of its own
		//function before anything is written to them
			//The pin the bridge lands on and the potential it belongs to
			//-- a pin beside it is a potential of its own and says what it
			//always said
		QList<Conductor *> all;
		for (Conductor *c : conductor ? QList<Conductor *>{conductor} : QList<Conductor *>()) {
			if (!all.contains(c)) {
				all << c;
			}
		}
		for (int i = 0 ; i < all.size() ; ++i) {
			const auto related = all.at(i)->relatedPotentialConductors();
			for (Conductor *c : related) {
				if (c && !all.contains(c)) {
					all << c;
				}
			}
				//And over a function separation: the voltage crosses it
				//the way the wires themselves let it
			const auto separated = all.at(i)->functionSeparatedConductors();
			for (Conductor *c : separated) {
				if (c && !all.contains(c)) {
					all << c;
				}
			}
		}
		QVERIFY2(!all.isEmpty(), "there are wires wired to it");
		QHash<Conductor *, QString> functions_before;
		for (Conductor *c : std::as_const(all)) {
			functions_before.insert(c, c->properties().m_function);
		}

			//What the wires say is what a bridge reads
		ConductorProperties with_function = conductor->properties();
		with_function.m_function = QStringLiteral("PE");
		conductor->setProperties(with_function);
		functions_before.insert(conductor, QStringLiteral("PE"));

		Bridge *bridge = project.newBridge();
		bridge->addTerminal(terminalOf(wired));

		const BridgeInformation information{QStringLiteral("230V")};
			//The very command the drawing pushes, so the wires go with it
			//the way they do in the drawing
		const BridgeInformation carried{bridge->tensionText()};
		auto *step = new ChangeBridgeTerminalsCommand(bridge, bridge->terminals(),
												  bridge->terminals(), carried, information);
		for (Terminal *pin : wired->element()->terminals()) {
		}
		BridgeManager::writeInformation(&project, bridge->terminals(), information, step);
		folio->undoStack().push(step);

		for (Conductor *c : std::as_const(all)) {
			QCOMPARE(c->properties().m_tension_protocol, QStringLiteral("230V"));
				//The function is the terminal's own business: a bridge
				//crosses the voltage and nothing else. Each wire keeps its
			//its own
			QCOMPARE(c->properties().m_function, functions_before.value(c));
		}

			//...and the bridge carries it because its wires do
		QCOMPARE(bridge->tensionText(), QStringLiteral("230V"));

			//Change the wire later and the bridge follows, like every
			//other wire of that potential
		ConductorProperties changed = conductor->properties();
		changed.m_tension_protocol = QStringLiteral("0V");
		conductor->setProperties(changed);
		QCOMPARE(bridge->tensionText(), QStringLiteral("0V"));

			//And one undo puts the wires back the way they were
		folio->undoStack().undo();
		QCOMPARE(conductor->properties().m_tension_protocol, original_tension);

			//The very first step of a drawn bridge writes the wires as
			//well: the command the drawing pushes runs the steps under it
		const BridgeInformation chosen{QStringLiteral("24VDC")};
		BridgePartData data;
		data.uuid = QUuid::createUuid();
		data.diagram = folio->uuid();
		data.p1 = wired->element()->scenePos();
		data.p2 = wired->element()->scenePos() + QPointF(60, 0);

		Bridge *drawn = project.newBridge();
		drawn->addPart(data);
		drawn->addTerminal(terminalOf(wired));
		drawn->setTensionText(chosen.tension);

		auto *item = new BridgePart(data);
		item->setBridge(drawn);
		folio->addItem(item);

		auto *command = new AddBridgeCommand(drawn, item, folio, true);
		BridgeManager::writeInformation(&project, drawn->terminals(), chosen, command);
		folio->undoStack().push(command);

		QCOMPARE(conductor->properties().m_tension_protocol, QStringLiteral("24VDC"));
		QCOMPARE(drawn->tensionText(), QStringLiteral("24VDC"));

		folio->undoStack().undo();
		QCOMPARE(conductor->properties().m_tension_protocol, original_tension);
	}

	//A name, a place and what it carries: two bridges are told apart
	//the way places are named on a drawing -- which sheet, which column
	//and which row
	void aBridgeCanBeToldApart()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY(terminals.size() >= 2);

		Bridge *bridge = project.newBridge();
		bridge->addTerminal(terminalOf(terminals.at(0)));
		bridge->setName(QStringLiteral("24V"));

		QCOMPARE(bridge->name(), QStringLiteral("24V"));
		const QString location = BridgeManager::locationOf(&project, bridge);
		QVERIFY2(!location.isEmpty(),
				 "the bridge names its place: sheet, column and row");

			//And the name travels with the project
		QDomDocument document;
		const QDomElement written = bridge->toXml(document);
		Bridge read_back;
		read_back.fromXml(written);
		QCOMPARE(read_back.name(), QStringLiteral("24V"));
	}

	//A sheet reference a bridge is carried on by is no Klemme: it never
	//counts as one, and never fills one of the places the bridge holds
	void aSheetReferenceIsNoKlemme()
	{
		const QString fixture = QFINDTESTDATA("fixtures/linked_reports.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		Element *report = nullptr;
		for (Diagram *diagram : project.diagrams()) {
			for (Element *element : diagram->elements()) {
				if ((element->linkType() & Element::AllReport)
					&& !element->terminals().isEmpty()) {
					report = element;
					break;
				}
			}
			if (report) break;
		}
		if (!report) {
			QSKIP("the fixture has no sheet reference with a connection");
		}

		Bridge *bridge = project.newBridge();
		bridge->setMaxConnections(1);
		bridge->addTerminal(BridgeTerminal{report->uuid(),
										   report->terminals().first()->stableUuid().toString(),
										   true});
		QCOMPARE(bridge->terminalCount(), 1);
		QVERIFY2(bridge->klemmCount() == 0,
				  "a sheet reference is no Klemme");
		QVERIFY2(!bridge->wouldExceed(1),
				 "and it never fills one of the places the bridge holds");
	}

	//A bridge lands on the connection of a sheet reference the way a
	//wire does: that is what carries it on to the sheet the other half
	//of the reference stands on
	void aBridgeLandsOnASheetReference()
	{
		const QString fixture = QFINDTESTDATA("fixtures/linked_reports.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		Element *report = nullptr;
		for (Diagram *diagram : project.diagrams()) {
			for (Element *element : diagram->elements()) {
				if (element->linkType() & Element::AllReport) {
					report = element;
					break;
				}
			}
			if (report) break;
		}
		if (!report || report->terminals().isEmpty()) {
			QSKIP("the fixture has no sheet reference with a connection");
		}
		Diagram *diagram = report->diagram();
		QVERIFY(diagram);

			//A line over its connection is recognized, the way a line
			//over a terminal symbol is
		const QPointF at = report->terminals().first()->scenePos();
		const QPointF middle = report->sceneBoundingRect().center();
		const QLineF line(at, middle + (middle - at));
		const auto crossings = BridgeManager::crossedTerminals(diagram, line);
		bool lands = false;
		for (const BridgeCrossing &crossing : crossings) {
			if (crossing.element == report) lands = true;
		}
		QVERIFY2(lands, "the line over the connection of a sheet reference is recognized");

		Bridge *bridge = project.newBridge();
		bridge->addTerminal(BridgeTerminal{report->uuid(),
										   report->terminals().first()->stableUuid().toString()});
		QVERIFY2(BridgeManager::bridgeLandsOn(&project, report),
				 "and the bridge lands on it");
	}

	//A bridge and a wire may not be joined through a link: what one
	//side carries and what the other carries are two different things
	void aLinkMayNotJoinABridgeToAWire()
	{
		const QString fixture = QFINDTESTDATA("fixtures/linked_reports.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		QList<Element *> reports;
		for (Diagram *diagram : project.diagrams()) {
			for (Element *element : diagram->elements()) {
				if ((element->linkType() & Element::AllReport)
					&& !element->terminals().isEmpty()) {
					reports << element;
				}
			}
		}
		if (reports.size() < 2) {
			QSKIP("the fixture has fewer than two sheet references");
		}
		Element *first = reports.at(0);
		Element *second = reports.at(1);

			//Nothing on either side: a link between wires is fine
		QVERIFY(!BridgeManager::linkJoinsBridgeToWire(&project, first, second));

			//A bridge on one side only: the link would join a bridge to
			//a wire, and that is refused where the link is made
		Bridge *bridge = project.newBridge();
		bridge->addTerminal(BridgeTerminal{first->uuid(),
										   first->terminals().first()->stableUuid().toString()});
		QVERIFY2(BridgeManager::linkJoinsBridgeToWire(&project, first, second),
				 "a bridge and a wire may not be joined through a link");
		QVERIFY(BridgeManager::linkJoinsBridgeToWire(&project, second, first));

			//A bridge on both sides: the bridge continues on the other
			//sheet, and the link is free
		bridge->addTerminal(BridgeTerminal{second->uuid(),
										   second->terminals().first()->stableUuid().toString()});
		QVERIFY2(!BridgeManager::linkJoinsBridgeToWire(&project, first, second),
				 "a bridge may continue on the other sheet");
	}

	//A symbol may carry several pins and each of them its own voltage:
	//the question a bridge asks sees every one of them, and what it
	//carries goes to every one of them
	void thePinsOfASymbolAreAskedApart()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY(!terminals.isEmpty());
		Diagram *folio = diagramOf(project, terminals.at(0)->element());
		QVERIFY(folio);

			//A symbol with two wired pins
		Element *element = nullptr;
		for (const QSharedPointer<RealTerminal> &terminal : terminals) {
			Element *candidate = terminal->element();
			if (!candidate || candidate->terminals().size() < 2) {
				continue;
			}
			int wired = 0;
			for (Terminal *pin : candidate->terminals()) {
				if (!pin->conductors().isEmpty()) {
					++wired;
				}
			}
			if (wired >= 2) {
				element = candidate;
				break;
			}
		}
		if (!element) {
			QSKIP("the fixture has no symbol with two wired pins");
		}

		QList<Terminal *> wired_pins;
		for (Terminal *pin : element->terminals()) {
			if (!pin->conductors().isEmpty()) {
				wired_pins << pin;
			}
		}

		ConductorProperties first_properties = wired_pins.at(0)->conductors().first()->properties();
		first_properties.m_tension_protocol = QStringLiteral("11V");
		wired_pins.at(0)->conductors().first()->setProperties(first_properties);

		ConductorProperties second_properties = wired_pins.at(1)->conductors().first()->properties();
		second_properties.m_tension_protocol = QStringLiteral("22V");
		wired_pins.at(1)->conductors().first()->setProperties(second_properties);

			//The question sees both of them: two pins, two voltages, two
			//candidates to choose from
		const BridgeTerminal on_the_symbol{
				element->uuid(), wired_pins.at(0)->stableUuid().toString()};
		const QList<BridgeInformation> sets =
				BridgeManager::informationSets(&project, {on_the_symbol});
		QList<QString> tensions;
		for (const BridgeInformation &set : sets) {
			tensions << set.tension;
		}
		QVERIFY2(tensions.contains(QStringLiteral("11V")), "the one pin is offered");
		QVERIFY2(tensions.contains(QStringLiteral("22V")), "and the pin beside it");

			//And what the bridge carries goes to every pin of the symbol:
		//a bridge sitting across a terminal reaches all of them
		Bridge *bridge = project.newBridge();
		bridge->addTerminal(on_the_symbol);
		auto *step = new QUndoCommand();
		BridgeManager::writeInformation(&project, bridge->terminals(),
										BridgeInformation{QStringLiteral("230V")}, step);
		folio->undoStack().push(step);

		for (Terminal *pin : wired_pins) {
			QCOMPARE(pin->conductors().first()->properties().m_tension_protocol,
					 QStringLiteral("230V"));
		}

		folio->undoStack().undo();
		QCOMPARE(wired_pins.at(0)->conductors().first()->properties().m_tension_protocol,
				 QStringLiteral("11V"));
		QCOMPARE(wired_pins.at(1)->conductors().first()->properties().m_tension_protocol,
				 QStringLiteral("22V"));
	}

	//A bridge takes an edit over to the wires of every terminal it lands
	//on: the voltage crosses it, and only the voltage
	void aWireEditCrossesTheBridge()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY(terminals.size() >= 2);

			//Two wired pins the bridge lands on
		QList<Conductor *> wires;
		QList<BridgeTerminal> lands_on;
		for (const QSharedPointer<RealTerminal> &terminal : terminals) {
			Element *element = terminal->element();
			if (!element || element->terminals().isEmpty()
				|| element->terminals().first()->conductors().isEmpty()) {
				continue;
			}
			wires << element->terminals().first()->conductors().first();
			lands_on << terminalOf(terminal);
			if (lands_on.size() == 2) {
				break;
			}
		}
		if (lands_on.size() < 2) {
			QSKIP("the fixture has fewer than two wired terminals");
		}

		Bridge *bridge = project.newBridge();
		bridge->setTerminals(lands_on);

		const QList<Conductor *> over =
				BridgeManager::conductorsOverBridges(&project, {wires.first()});
		QVERIFY2(over.contains(wires.last()),
				 "the edit reaches the wires of the other end of the bridge");

			//The bridge sits on the terminal itself, not on the pin the
			//line happened to run closest to: a wire on another pin of
			//the same terminal reaches the bridge all the same
		Element *first_element = BridgeManager::elementFor(&project, lands_on.first());
		QVERIFY(first_element);
		if (first_element->terminals().size() < 2) {
			QSKIP("the fixture has no symbol with two pins");
		}
		const BridgeTerminal another_pin{
					first_element->uuid(),
					first_element->terminals().last()->stableUuid().toString()};
		QVERIFY2(another_pin != lands_on.first(),
				 "the bridge names the other pin of that terminal");

		Bridge *drawn = project.newBridge();
		drawn->setTerminals({another_pin, lands_on.last()});

		const QList<Conductor *> from_the_other_pin =
				BridgeManager::conductorsOverBridges(&project, {wires.first()});
		QVERIFY2(from_the_other_pin.contains(wires.last()),
				 "an edit on a pin the line did not cross reaches the bridge too");
	}

	//The sort button brings the bridges of the drawing into the strip:
	//what is drawn becomes a bridge of the strip, spanning every
	//terminal it lies across, and one undo takes it back
	void sortingBringsTheBridgesOfTheDrawing()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY2(terminals.size() >= 3, "the strip X1 has terminals to lie across");
		QVector<TerminalStrip *> strips = project.terminalStrip();
		QVERIFY(!strips.isEmpty());
		TerminalStrip *strip = strips.first();

			//A drawn bridge over the first and the third terminal
		Bridge *drawn = project.newBridge();
		drawn->setColor(QColor(Qt::red));
		drawn->addTerminal(terminalOf(terminals.at(0)));
		drawn->addTerminal(terminalOf(terminals.at(2)));

		project.diagrams().first()->undoStack().push(new SortTerminalStripCommand(strip));

		const auto bridges = strip->automaticBridges();
		QVERIFY2(bridges.size() == 1, "what is drawn becomes a bridge of the strip");
		QCOMPARE(bridges.first().source, drawn->uuid());
		QCOMPARE(bridges.first().color, QColor(Qt::red));
		QVERIFY2(bridges.first().real_terminals.size() == 2,
				 "the bar is plugged into the terminals it was drawn over "
				 "-- and into no other");

			//A bridge the drawing brought here is not his to take away
		QVERIFY(!strip->canUnBridge(bridges.first().real_terminals));

		project.diagrams().first()->undoStack().undo();
		QVERIFY2(strip->automaticBridges().isEmpty(),
				 "one undo takes the bridges back with the sorting");
	}

	//Two bridges lying across the same terminals run side by side:
	//each takes its own groove, the one the drawing gave it
	void everyBridgeTakesItsOwnGroove()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY2(terminals.size() >= 4, "the strip X1 has four terminals");
		QVector<TerminalStrip *> strips = project.terminalStrip();
		QVERIFY(!strips.isEmpty());
		TerminalStrip *strip = strips.first();

			//One bridge over the first and the third, one over the second
			//and the fourth: they lie across each other
		Bridge *first = project.newBridge();
		first->addTerminal(terminalOf(terminals.at(0)));
		first->addTerminal(terminalOf(terminals.at(2)));
		first->setColumn(BridgeManager::freeColumn(&project, first->terminals()));

		Bridge *second = project.newBridge();
		second->addTerminal(terminalOf(terminals.at(1)));
		second->addTerminal(terminalOf(terminals.at(3)));
		second->setColumn(BridgeManager::freeColumn(&project, second->terminals()));

		QCOMPARE(first->column(), 1);
		QVERIFY2(second->column() == 2, "the second bridge runs beside the first");

		project.diagrams().first()->undoStack().push(new SortTerminalStripCommand(strip));

		const auto bridges = strip->automaticBridges();
		QCOMPARE(bridges.size(), 2);
		QList<int> tracks;
		for (const auto &bridge : bridges) {
			tracks << bridge.track;
		}
			//The fixture's own bridge takes a groove over these terminals
			//already; what matters is that the two bars lying across each
			//other never share one
		QVERIFY2(tracks.at(0) != tracks.at(1),
				 "two bars lying across each other run side by side");
		for (int track : std::as_const(tracks)) {
			QVERIFY(track >= 1 && track <= 4);
		}
	}

	//The four columns of the table are the four grooves of the rail: a
	//bar is drawn in the column of its own groove, so two bars lying
	//across the same terminals stand side by side instead of one inside
	//the other
	void theCellsFollowTheGrooveOfTheBridge()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY2(terminals.size() >= 4, "the strip X1 has four terminals");
		QVector<TerminalStrip *> strips = project.terminalStrip();
		QVERIFY(!strips.isEmpty());
		TerminalStrip *strip = strips.first();

			//Two bars lying across each other: the first over the first
			//and the third terminal, the second over the second and the
			//fourth
		Bridge *first = project.newBridge();
		first->addTerminal(terminalOf(terminals.at(0)));
		first->addTerminal(terminalOf(terminals.at(2)));
		Bridge *second = project.newBridge();
		second->addTerminal(terminalOf(terminals.at(1)));
		second->addTerminal(terminalOf(terminals.at(3)));
		project.diagrams().first()->undoStack().push(new SortTerminalStripCommand(strip));

		TerminalStripModel model(strip);
		model.buildBridgePixmap(QSize(24, 24)); // what the editor does at startup

			//Every bar of the strip, wherever it comes from, with the
			//span of terminals it lies across and the groove it sits in
		struct Bar { int first = -1; int last = -1; int groove = -1; };
		QList<Bar> bars;
		const auto order = strip->realTerminals();
		for (const auto &real_t : order) {
			for (const auto &bridge : strip->bridgesOf(real_t)) {
				if (!bridge) continue;
				bool known = false;
				for (const Bar &bar : std::as_const(bars)) {
					if (bar.groove == bridge->track()) { known = true; break; }
				}
				if (known) continue;
				Bar bar;
				bar.groove = bridge->track();
				for (const auto &of_bridge : bridge->realTerminals()) {
					const int pos = of_bridge->physicalTerminal()->pos();
					bar.first = (bar.first < 0) ? pos : qMin(bar.first, pos);
					bar.last = (bar.last < 0) ? pos : qMax(bar.last, pos);
				}
				bars << bar;
			}
		}

		for (const Bar &bar : bars)
		{
			const int column = TerminalStripModel::Level0 + qBound(1, bar.groove, 4) - 1;
			for (int row = bar.first ; row <= bar.last ; ++row)
			{
				const auto cell = model.data(model.index(row, column), Qt::DecorationRole);
				QVERIFY2(!cell.value<QPixmap>().isNull(),
						 "the bar is drawn in the column of its own groove");
			}

				//And nowhere outside its own span
			for (int row = 0 ; row < model.rowCount() ; ++row) {
				if (row >= bar.first && row <= bar.last) {
					continue;
				}
				bool covered = false;
				for (const Bar &other : std::as_const(bars)) {
					if (other.groove != bar.groove) continue;
					if (row >= other.first && row <= other.last) covered = true;
				}
				if (covered) continue;
				const auto cell = model.data(model.index(row, column), Qt::DecorationRole);
				QVERIFY2(cell.value<QPixmap>().isNull(),
						 "and it leaves the other rows of that column alone");
			}
		}
	}

	//The bridges he set here by hand are his own: the sorting never
	//touches them
	void sortingLeavesTheBridgesHeSetByHand()
	{
		const QString fixture = QFINDTESTDATA("fixtures/bridge_tracks.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		const auto terminals = stripTerminals(project);
		QVERIFY2(terminals.size() >= 4, "the strip X1 has four terminals");
		QVector<TerminalStrip *> strips = project.terminalStrip();
		QVERIFY(!strips.isEmpty());
		TerminalStrip *strip = strips.first();

			//The two the fixture does not bridge yet
		QVector<QSharedPointer<RealTerminal>> by_hand;
		by_hand << terminals.at(2) << terminals.at(3);
		QVERIFY(strip->setBridge(by_hand));

		project.diagrams().first()->undoStack().push(new SortTerminalStripCommand(strip));

		QVERIFY2(strip->isBridged(terminals.at(2)), "his bridge is still there");
		QVERIFY(strip->canUnBridge(by_hand));
		QVERIFY(strip->automaticBridges().isEmpty());
	}
};

QET_APP_TEST_MAIN(tst_bridgetool)
#include "tst_bridgetool.moc"
