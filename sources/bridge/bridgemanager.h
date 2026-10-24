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

#include <QLineF>
#include <QList>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QUuid>

class Diagram;
class Element;
class QETProject;

/**
	@brief One place where a drawn bridge line crosses a terminal.
*/
struct BridgeCrossing
{
	Element *element = nullptr;
		///Name of the terminal of that symbol the bridge lands on
	QString terminal;
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
		static QString functionOf(QETProject *project, Bridge *bridge);
		static QString tensionOf(QETProject *project, Bridge *bridge);

			//What one terminal carries as information, and the distinct
			//sets the terminals of a bridge carry: one when they agree,
			//more when the user has to be asked which the bridge takes
		static BridgeInformation informationOf(QETProject *project, const BridgeTerminal &terminal);
		static QList<BridgeInformation> informationSets(QETProject *project,
													   const QList<BridgeTerminal> &terminals);
};

#endif // BRIDGEMANAGER_H
