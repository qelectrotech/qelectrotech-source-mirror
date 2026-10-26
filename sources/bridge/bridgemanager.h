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
#ifndef BRIDGEMANAGER_H
#define BRIDGEMANAGER_H

#include "bridge.h"

#include "../TerminalStrip/terminalstrip.h"

#include <QLineF>
#include <QList>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QUuid>

class Conductor;
class Diagram;
class Element;
class QETProject;
class QUndoCommand;

/**
	@brief One place where a drawn bridge line crosses a terminal.
*/
struct BridgeCrossing
{
	Element *element = nullptr;
		///Name of the terminal of that symbol the bridge lands on
	QString terminal;
		///True when the symbol is a sheet reference rather than a
		///Klemme: a bridge carried on to another sheet is no terminal
		///connection
	bool reference = false;
		/// Scene coordinates of the crossing, where the line is drawn
	QPointF position;
};

/**
	@brief The BridgeManager works out everything which follows from the
	bridges of a project, so nothing has to be maintained by hand.

	It detects which terminals a line about to be drawn crosses, which
	parallel track a new bridge takes at those terminals, and what a
	bridge carries as information -- the function and the potential of
	the first terminal it lands on, read live so that the bridge follows
	them the way the cores of a cable follow their wires.
*/
class BridgeManager
{
	public:
			//Drawing
		static QList<BridgeCrossing> crossedTerminals(Diagram *diagram, const QLineF &line);

			//Where a bridge runs at the terminals it crosses
		static int freeColumn(QETProject *project, const QList<BridgeTerminal> &terminals);
		static QSet<QUuid> occupiedTerminals(QETProject *project,
											 const QList<BridgeTerminal> &terminals);

			//Lookup
		static Bridge *bridgeByUuid(QETProject *project, const QUuid &uuid);
		static Element *elementFor(QETProject *project, const BridgeTerminal &terminal);
		static bool bridgeLandsOn(QETProject *project, Element *element);
		static QString locationOf(QETProject *project, Bridge *bridge);
		static bool linkJoinsBridgeToWire(QETProject *project,
										  Element *first,
										  Element *second);

			/**
				The terminals the bridge lands on at this very moment,
				worked out from its lines as they are drawn now: what a
				line crosses is what it takes, and a line moved onto or
				off a terminal takes it on or drops it again.
			*/
		static QList<BridgeTerminal> terminalsOfBridge(QETProject *project, Bridge *bridge);

			/**
				The bridges which cannot take what a movement would give
				them: a terminal moved onto one of their lines joins the
				bridge, and one which would fill it beyond its limit
				cannot be built in the cabinet.

				The elements are asked at the place they stand right now:
				this is asked after a movement was applied to see what it
				would build, and the answer decides whether it is kept or
				taken back -- the terminal then stays at the mouse and is
				dropped somewhere else.
			*/
		static QList<Bridge *> bridgesOverfilledBy(QETProject *project,
												   const QList<Element *> &moved);

			//What the bridge takes over from its first terminal, read live
		static QString tensionOf(QETProject *project, Bridge *bridge);

			//What one terminal carries as information, and the distinct
			//sets the terminals of a bridge carry: one when they agree,
			//more when the user has to be asked which the bridge takes.
			//Only the voltage/protocol -- a bridge crosses that and
			//nothing else
		static BridgeInformation informationOf(QETProject *project, const BridgeTerminal &terminal);
		static QList<BridgeInformation> informationSets(QETProject *project,
													   const QList<BridgeTerminal> &terminals);

			/**
				The wires a bridge takes an edit to: a bridge sits across
				the terminals it lands on, so changing the voltage of one
				of their wires changes it for every wire of every terminal
				of the bridges over it. Only the voltage and the protocol
				cross a bridge -- never the function, a separated function
				stays separated however many bridges sit across it.
				@param project
				@param edited the wires being edited
				@return the wires the bridges reach from them, once
			*/
		static QList<Conductor *> conductorsOverBridges(QETProject *project,
													   const QList<Conductor *> &edited);

			/**
				What the drawing says about the bridges of @p strip: every
				drawn bridge whose terminals sit in it, as the span it lies
				across. A bridge bar sits on every terminal between its
				ends -- which is exactly why two of them take their own
				track where they run beside each other.
			*/
		static QList<TerminalStrip::DrawingBridge> bridgesForStrip(QETProject *project,
																   TerminalStrip *strip);

			/**
				Give the wires wired to those terminals what the bridge
				carries, so the bridge and the wires it sits on say the
				same thing: one circle, never half of one. A wire which
				already says it is left alone; the rest goes with whatever
				was being done (@p parent), so it is one undo step.
			*/
		static void writeInformation(QETProject *project,
									 const QList<BridgeTerminal> &terminals,
									 const BridgeInformation &information,
									 QUndoCommand *parent);
};

#endif // BRIDGEMANAGER_H
