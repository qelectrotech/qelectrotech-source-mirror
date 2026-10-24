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
#include "bridgemanager.h"

#include "../TerminalStrip/physicalterminal.h"
#include "../TerminalStrip/realterminal.h"
#include "../TerminalStrip/terminalstrip.h"
#include "../diagram.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminalelement.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetinformation.h"
#include "../qetproject.h"
#include "../conductorproperties.h"
#include "../properties/elementdata.h"
#include "bridgepart.h"

#include <QPainterPath>
#include <algorithm>

namespace {

	/**
		Every element of @p project which is a terminal symbol, by
		identity: the bridge names the terminals it lands on that way,
		so it follows them wherever the drawing moves them.
	*/
	QHash<QUuid, Element *> terminalElements(QETProject *project)
	{
		QHash<QUuid, Element *> hash;
		if (!project) {
			return hash;
		}
		const auto diagrams = project->diagrams();
		for (Diagram *diagram : diagrams) {
			const auto elements = diagram->elements();
			for (Element *element : elements) {
				if (element && element->elementData().m_type == ElementData::Terminal) {
					hash.insert(element->uuid(), element);
				}
			}
		}
		return hash;
	}

	/**
		Which terminals of the strip @p strip lie between @p from and
		@p to -- inclusive, whatever their level: a bridge bar lies
		across the whole physical terminal on its way, so it takes its
		parallel track there as well.
	*/
	void spanInStrip(TerminalStrip *strip, int from, int to, QSet<QUuid> &into)
	{
		if (!strip) {
			return;
		}
		const int first = qMin(from, to);
		const int last = qMax(from, to);
		for (int pos = first ; pos <= last ; ++pos) {
			const QSharedPointer<PhysicalTerminal> physical = strip->physicalTerminal(pos);
			if (!physical) {
				continue;
			}
			const auto real_terminals = physical->realTerminals();
			for (const QSharedPointer<RealTerminal> &real_t : real_terminals) {
				if (real_t && real_t->element()) {
					into.insert(real_t->element()->uuid());
				}
			}
		}
	}
}

/**
	@brief BridgeManager::crossedTerminals
	@param diagram
	@param line the line about to be drawn
	@return every terminal the line crosses, in the order it crosses
	them. A terminal crossed is a terminal the bridge lands on: what is
	not touched is not taken.
*/
QList<BridgeCrossing> BridgeManager::crossedTerminals(Diagram *diagram, const QLineF &line)
{
	QList<BridgeCrossing> crossings;
	if (!diagram) {
		return crossings;
	}

	QPainterPath path(line.p1());
	path.lineTo(line.p2());

	const QList<QGraphicsItem *> items = diagram->items(path);
	for (QGraphicsItem *item : items) {
		Element *element = qgraphicsitem_cast<Element *>(item);
		if (!element
			|| element->elementData().m_type != ElementData::Terminal
			|| element->terminals().isEmpty()) {
			continue;
		}
		BridgeCrossing crossing;
		crossing.element = element;
		crossing.terminal = element->terminals().first()->name();
		crossing.position = element->scenePos();
		crossings.append(crossing);
	}

		//Along the line, the way they are crossed: the first one crossed
		//is the first one of the bridge
	const QPointF direction = line.p2() - line.p1();
	const qreal length = QLineF(line.p1(), line.p2()).length();
	if (length > 0) {
		std::sort(crossings.begin(), crossings.end(),
				  [&direction](const BridgeCrossing &a, const BridgeCrossing &b)
		{
			const qreal ta = QPointF::dotProduct(a.position, direction);
			const qreal tb = QPointF::dotProduct(b.position, direction);
			return ta < tb;
		});
	}

	return crossings;
}

/**
	@brief BridgeManager::occupiedTerminals
	@param project
	@param terminals the terminals a bridge would land on
	@return every terminal a bridge over @p terminals takes its
	parallel track at: the ones it lands on plus, when they all belong
	to one terminal strip, every terminal of that strip lying between
	them -- the bar of the bridge lies across those too, which is why
	two bridges side by side do not share a track even where only one
	of them lands.
*/
QSet<QUuid> BridgeManager::occupiedTerminals(QETProject *project, const QList<BridgeTerminal> &terminals)
{
	QSet<QUuid> occupied;
	const QHash<QUuid, Element *> elements = terminalElements(project);

	TerminalStrip *strip = nullptr;
	int first = -1;
	int last = -1;
	for (const BridgeTerminal &terminal : terminals) {
		Element *element = elements.value(terminal.element);
		if (!element) {
			continue;
		}
		occupied.insert(terminal.element);

		auto *terminal_element = dynamic_cast<TerminalElement *>(element);
		if (!terminal_element) {
			continue;
		}
		const QSharedPointer<RealTerminal> real_t = terminal_element->realTerminal();
		TerminalStrip *own_strip = real_t ? real_t->parentStrip() : nullptr;
		if (!own_strip || !real_t->physicalTerminal()) {
			continue;
		}
		const int pos = real_t->physicalTerminal()->pos();
		if (!strip) {
			strip = own_strip;
			first = last = pos;
		} else if (strip == own_strip) {
			first = qMin(first, pos);
			last = qMax(last, pos);
		}
			//Terminals of two strips: no span is worked out, only the
			//terminals themselves are taken
	}

	if (strip && first >= 0) {
		spanInStrip(strip, first, last, occupied);
	}
	return occupied;
}

/**
	@brief BridgeManager::freeColumn
	@param project
	@param terminals the terminals a new bridge would land on
	@return the first parallel track (1 to Bridge::max_columns) no
	other bridge takes at any terminal this one would run across, 0
	when there is none left: four bridges side by side are all a
	terminal takes.
*/
int BridgeManager::freeColumn(QETProject *project, const QList<BridgeTerminal> &terminals)
{
	if (!project) {
		return 0;
	}

	const QSet<QUuid> needed = occupiedTerminals(project, terminals);

	bool taken[Bridge::max_columns + 1] = {false, false, false, false, false};
	const auto bridges = project->bridges();
	for (Bridge *bridge : bridges) {
		if (!bridge) {
			continue;
		}
		const int column = bridge->column();
		if (column < 1 || column > Bridge::max_columns) {
			continue;
		}
		if (occupiedTerminals(project, bridge->terminals()).intersects(needed)) {
			taken[column] = true;
		}
	}

	for (int column = 1 ; column <= Bridge::max_columns ; ++column) {
		if (!taken[column]) {
			return column;
		}
	}
	return 0;
}

/**
	@brief BridgeManager::terminalsOfBridge
	@param project
	@param bridge
	@return the terminals the bridge lands on at this very moment,
	worked out from its lines as they are drawn now: what a line crosses
	is what it takes. A line moved onto or off a terminal takes it on or
	drops it again, and the order of the list follows the drawing -- the
	first terminal of it is the one the bridge takes its information
	from.
*/
QList<BridgeTerminal> BridgeManager::terminalsOfBridge(QETProject *project, Bridge *bridge)
{
	QList<BridgeTerminal> result;
	if (!project || !bridge) {
		return result;
	}

	for (const BridgePartData &data : bridge->parts())
	{
		Diagram *folio = project->diagramByUuid(data.diagram);
		if (!folio) {
			continue;
		}

			//The line as it is drawn right now: the item is ahead of the
			//data while the user drags an end around
		QLineF line(data.p1, data.p2);
		const auto items = folio->items();
		for (QGraphicsItem *item : items) {
			if (auto *part = qgraphicsitem_cast<BridgePart *>(item)) {
				if (part->partUuid() == data.uuid) {
					line = part->line();
					break;
				}
			}
		}

		const auto crossings = crossedTerminals(folio, line);
		for (const BridgeCrossing &crossing : crossings) {
			BridgeTerminal terminal;
			terminal.element = crossing.element->uuid();
			terminal.terminal = crossing.terminal;
			if (!result.contains(terminal)) {
				result << terminal;
			}
		}
	}
	return result;
}

/**
	@brief BridgeManager::bridgesOverfilledBy
	@param project
	@param moved the elements being moved
	@return the bridges which cannot take what the movement would give
	them: a terminal of @p moved landing on one of their lines joins the
	bridge, and one which would fill it beyond its limit cannot be
	built. Asked at the place the elements stand right now.
*/
QList<Bridge *> BridgeManager::bridgesOverfilledBy(QETProject *project,
												   const QList<Element *> &moved)
{
	QList<Bridge *> overfilled;
	if (!project || moved.isEmpty()) {
		return overfilled;
	}

		//Only the folios the elements are on can hold a line they land on
	QSet<QUuid> folios;
	for (Element *element : moved) {
		if (element && element->diagram()) {
			folios.insert(element->diagram()->uuid());
		}
	}
	if (folios.isEmpty()) {
		return overfilled;
	}

	const auto bridges = project->bridges();
	for (Bridge *bridge : bridges)
	{
		if (!bridge) {
			continue;
		}
		bool on_these_folios = false;
		const auto parts = bridge->parts();
		for (const BridgePartData &part : parts) {
			if (folios.contains(part.diagram)) {
				on_these_folios = true;
				break;
			}
		}
		if (!on_these_folios) {
			continue;
		}

		int gained = 0;
		const auto after = terminalsOfBridge(project, bridge);
		for (const BridgeTerminal &terminal : after) {
			if (!bridge->hasTerminal(terminal)) {
				++gained;
			}
		}
		if (gained > 0 && bridge->wouldExceed(gained)) {
			overfilled << bridge;
		}
	}
	return overfilled;
}

/**
	@brief BridgeManager::bridgeByUuid
	@param project
	@param uuid
	@return the bridge with that identity, or nullptr
*/
Bridge *BridgeManager::bridgeByUuid(QETProject *project, const QUuid &uuid)
{
	if (!project || uuid.isNull()) {
		return nullptr;
	}
	const auto bridges = project->bridges();
	for (Bridge *bridge : bridges) {
		if (bridge && bridge->uuid() == uuid) {
			return bridge;
		}
	}
	return nullptr;
}

/**
	@brief BridgeManager::elementFor
	@param project
	@param terminal
	@return the terminal symbol that terminal names, or nullptr
*/
Element *BridgeManager::elementFor(QETProject *project, const BridgeTerminal &terminal)
{
	return terminalElements(project).value(terminal.element);
}

/**
	@brief BridgeManager::informationOf
	@param project
	@param terminal
	@return what that terminal carries as information: its function
	text and the voltage/protocol of a wire wired to it. Read live, so
	the bridge follows the way the cores of a cable follow their wires.
*/
BridgeInformation BridgeManager::informationOf(QETProject *project, const BridgeTerminal &terminal)
{
	BridgeInformation information;
	Element *element = elementFor(project, terminal);
	if (!element) {
		return information;
	}
	information.function = element->elementInformations().value(QETInformation::ELMT_FUNCTION).toString();

	const auto terminals = element->terminals();
	if (!terminals.isEmpty()) {
		const auto conductors = terminals.first()->conductors();
		if (!conductors.isEmpty()) {
			information.tension = conductors.first()->properties().m_tension_protocol;
		}
	}
	return information;
}

/**
	@brief BridgeManager::informationSets
	@param project
	@param terminals
	@return the distinct sets of information those terminals carry: one
	when they agree, more when they do not and the user has to be asked
	which the bridge takes.
*/
QList<BridgeInformation> BridgeManager::informationSets(QETProject *project,
														const QList<BridgeTerminal> &terminals)
{
	QList<BridgeInformation> sets;
	for (const BridgeTerminal &terminal : terminals) {
		const BridgeInformation information = informationOf(project, terminal);
		if (!sets.contains(information)) {
			sets << information;
		}
	}
	return sets;
}

/**
	@brief BridgeManager::functionOf
	@param project
	@param bridge
	@return the function text the bridge carries
*/
QString BridgeManager::functionOf(QETProject *project, Bridge *bridge)
{
	Q_UNUSED(project)
	return bridge ? bridge->functionText() : QString();
}

/**
	@brief BridgeManager::tensionOf
	@param project
	@param bridge
	@return the voltage/protocol the bridge carries
*/
QString BridgeManager::tensionOf(QETProject *project, Bridge *bridge)
{
	Q_UNUSED(project)
	return bridge ? bridge->tensionText() : QString();
}
