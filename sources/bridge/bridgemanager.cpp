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
#include "bridgeundocommands.h"

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../TerminalStrip/physicalterminal.h"
#include "../TerminalStrip/realterminal.h"
#include "../TerminalStrip/terminalstrip.h"
#include "../diagram.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/dynamicelementtextitem.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminalelement.h"
#include "../qetgraphicsitem/terminal.h"
#include "../autoNum/assignvariables.h"
#include "../qetinformation.h"
#include "../undocommand/linkelementcommand.h"
#include "../qetproject.h"
#include "../conductorproperties.h"
#include "../properties/elementdata.h"
#include "bridgepart.h"

#include <QPainterPath>
#include <algorithm>

namespace {

	/**
		Every element of @p project a bridge may land on, by identity: a
		terminal symbol, or the connection of a sheet reference -- what
		carries a bridge on to the sheet the other half of the reference
		stands on. The bridge names them that way, so it follows them
		wherever the drawing moves them.
	*/
	QHash<QUuid, Element *> bridgeableElements(QETProject *project)
	{
		QHash<QUuid, Element *> hash;
		if (!project) {
			return hash;
		}
		const auto diagrams = project->diagrams();
		for (Diagram *diagram : diagrams) {
			const auto elements = diagram->elements();
			for (Element *element : elements) {
				if (element
					&& (element->elementData().m_type == ElementData::Terminal
						|| (element->linkType() & Element::AllReport))) {
					hash.insert(element->uuid(), element);
				}
			}
		}
		return hash;
	}

	/**
		How far @p point stands from the line @p line: the pin the line
		really runs across is the one nearest to it.
	*/
	qreal distanceToLine(const QLineF &line, const QPointF &point)
	{
		const QPointF a = line.p1();
		const QPointF ab = line.p2() - a;
		const qreal length_squared = QPointF::dotProduct(ab, ab);
		if (length_squared <= 0.0) {
			return QLineF(a, point).length();
		}
		const qreal at = qBound(0.0, QPointF::dotProduct(point - a, ab) / length_squared, 1.0);
		return QLineF(a + at * ab, point).length();
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
		if (!element || element->terminals().isEmpty()) {
			continue;
		}
			//A terminal symbol, or the connection of a sheet reference:
			//a bridge may land on either, and a sheet reference is what
			//carries it on to the sheet the other half of it stands on
		const bool is_terminal =
				element->elementData().m_type == ElementData::Terminal;
		const bool is_sheet_reference =
				element->linkType() & Element::AllReport;
		if (!is_terminal && !is_sheet_reference) {
			continue;
		}
			//The pin the line runs across: a symbol may carry several
			//terminals and each of them is its own potential, so the
			//nearest one to the line is the one the bridge lands on
		Terminal *pin = nullptr;
		qreal nearest = 0.0;
		const auto pins = element->terminals();
		for (Terminal *candidate : pins)
		{
			const qreal distance = distanceToLine(line, candidate->dockConductor());
			if (!pin || distance < nearest) {
				pin = candidate;
				nearest = distance;
			}
		}
		if (!pin) {
			continue;
		}

		BridgeCrossing crossing;
		crossing.element = element;
		crossing.terminal = pin->stableUuid().toString();
		crossing.reference = is_sheet_reference;
		crossing.position = pin->dockConductor();
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
	const QHash<QUuid, Element *> elements = bridgeableElements(project);

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
			terminal.reference = crossing.reference;
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

namespace {
	/**
		Where the pin named @p dock of @p moved stands now, and whether
		that pin belongs to @p moved at all.
	*/
	QPointF pinPosition(Bridge *bridge,
						const QString &dock,
						Element *moved,
						bool *found)
	{
		*found = false;
		const auto lands_on = bridge->landings();
		for (const BridgeTerminal &landing : lands_on)
		{
			const auto element = landing.element;
			if (!moved || moved->uuid() != element) {
				continue;
			}
			for (Terminal *pin : moved->terminals()) {
				if (pin->stableUuid().toString() == dock) {
					*found = true;
					return pin->dockConductor();
				}
			}
		}
		return QPointF();
	}
}

/**
	@brief BridgeManager::dockAt
	@param project
	@param bridge
	@param point where a drawn end stands
	@param tolerance how far a pin may stand from it to be docked to it
	@return the pin of one of the bridge's landings the end is docked to,
	empty when it stands on its own
*/
QString BridgeManager::dockAt(QETProject *project,
							  Bridge *bridge,
							  const QPointF &point,
							  qreal tolerance)
{
	if (!project || !bridge) {
		return {};
	}
	QString pin_uuid;
	qreal nearest = tolerance;
	const auto lands_on = bridge->landings();
	for (const BridgeTerminal &landing : lands_on)
	{
		Element *element = elementFor(project, landing);
		if (!element) {
			continue;
		}
		for (Terminal *pin : element->terminals())
		{
			const qreal distance = QLineF(pin->dockConductor(), point).length();
			if (distance <= nearest) {
				nearest = distance;
				pin_uuid = pin->stableUuid().toString();
			}
		}
	}
	return pin_uuid;
}

/**
	@brief BridgeManager::bridgeCarriedBy
	@param project
	@param element a sheet reference
	@return the bridge that reference is carried on by, or nullptr
*/
Bridge *BridgeManager::bridgeCarriedBy(QETProject *project, Element *element)
{
	if (!project || !element) {
		return nullptr;
	}
	const auto bridges = project->bridges();
	for (Bridge *bridge : bridges)
	{
		if (!bridge) {
			continue;
		}
		const auto lands_on = bridge->landings();
		for (const BridgeTerminal &landing : lands_on) {
			if (elementFor(project, landing) == element) {
				return bridge;
			}
		}
	}
	return nullptr;
}

/**
	@brief BridgeManager::joinOverLink
	@param project
	@param one
	@param other the two sheet references being linked
	@param parent the step this goes with
*/
void BridgeManager::joinOverLink(QETProject *project,
								 Element *one,
								 Element *other,
								 QUndoCommand *parent)
{
	if (!project || !one || !other || !parent) {
		return;
	}
	Bridge *first = bridgeCarriedBy(project, one);
	Bridge *second = bridgeCarriedBy(project, other);
	if (!first || !second || first == second) {
		return;
	}

		//The unit is the one which carries the voltage -- the one the
		//other half is packed onto, the way the GUI does it -- and where
		//both or neither of them do, the one holding more of the drawing
	Bridge *into = first;
	if (second->tensionText() != first->tensionText()) {
		if (!second->tensionText().isEmpty()) {
			into = second;
		}
	} else if (second->klemmCount() > first->klemmCount()) {
		into = second;
	}
	Bridge *from = (into == first) ? second : first;

	new MergeBridgesCommand(from, into, parent);

		//What the unit says goes onto the wires of the whole of it --
		//the list as it stands when the two are one, so the far half
		//says it too
	QList<BridgeTerminal> unit = into->terminals();
	const auto from_landings = from->landings();
	for (const BridgeTerminal &landing : from_landings) {
		if (!unit.contains(landing)) {
			unit << landing;
		}
	}
	writeInformation(project, unit,
					 BridgeInformation{into->tensionText()}, parent);
}

/**
	@brief BridgeManager::joinTarget
	@param project
	@param landings where a line comes to
	@param self the bridge asking, never the answer
	@return the bridge one of those landings reaches through a sheet
	reference -- the one the reference, or the one it is linked to, is
	already carried on by -- or nullptr when there is none. What is one
	bridge carried on over the sheets is one unit: a line landing on
	one half of it joins the whole of it.
*/
Bridge *BridgeManager::joinTarget(QETProject *project,
								  const QList<BridgeTerminal> &landings,
								  Bridge *self)
{
	if (!project) {
		return nullptr;
	}
	for (const BridgeTerminal &landing : landings)
	{
		Element *element = elementFor(project, landing);
		if (!element || !(element->linkType() & Element::AllReport)) {
			continue;
		}
			//The reference itself, and the one it is linked to: the
			//half of a bridge carried on over the sheets stands on the
			//other sheet behind that link
		QList<Element *> system;
		system << element;
		const auto linked = element->linkedElements();
		for (Element *other : linked) {
			system << other;
		}
		for (Element *member : std::as_const(system))
		{
			const auto bridges = project->bridges();
			for (Bridge *bridge : bridges)
			{
				if (!bridge || bridge == self) {
					continue;
				}
				const auto lands_on = bridge->landings();
				for (const BridgeTerminal &on : lands_on) {
					if (elementFor(project, on) == member) {
						return bridge;
					}
				}
			}
		}
	}
	return nullptr;
}

/**
	@brief BridgeManager::refreshReferences
	@param project
	@param landings where a bridge comes or goes
	Every sheet reference among them reads again what hangs on it: what
	a reference says follows the drawing, and a bridge landed on one is
	as much a thing hung on it as a wire is.
*/
void BridgeManager::refreshReferences(QETProject *project,
									  const QList<BridgeTerminal> &landings)
{
	if (!project) {
		return;
	}
	for (const BridgeTerminal &landing : landings)
	{
		Element *element = elementFor(project, landing);
		if (!element || !(element->linkType() & Element::AllReport)) {
			continue;
		}
		const auto texts = element->dynamicTextItems();
		for (DynamicElementTextItem *text : texts) {
			if (text) {
				text->refreshConnection();
			}
		}
	}
}

/**
	@brief BridgeManager::pairReferences
	@param project
	@param bridge
	@param parent the step this goes with
*/
void BridgeManager::pairReferences(QETProject *project, Bridge *bridge, QUndoCommand *parent)
{
	if (!project || !bridge || !parent) {
		return;
	}

	QList<Element *> free_references;
	const auto lands_on = bridge->references();
	for (const BridgeTerminal &landing : lands_on)
	{
		Element *element = elementFor(project, landing);
		if (element && element->isFree()) {
			free_references << element;
		}
	}

		//A going arrow is linked with a coming one and never with its
		//own kind: what the drawing pairs is the way out of a sheet
		//and the way into it
	for (Element *out : std::as_const(free_references))
	{
		if (!out->isFree()) {
			continue;
		}
		for (Element *in : std::as_const(free_references))
		{
			if (in == out || !in->isFree()) {
				continue;
			}
			const bool pair_ok =
					(out->linkType() == Element::NextReport
					 && in->linkType() == Element::PreviousReport)
					|| (out->linkType() == Element::PreviousReport
						&& in->linkType() == Element::NextReport);
			if (!pair_ok) {
				continue;
			}
			auto *link = new LinkElementCommand(out, parent);
			link->setLink(in);
			break;
		}
	}
}

/**
	@brief BridgeManager::redock
	@param project
	@param bridge
	@param part the drawn line whose ends are worked out again
*/
void BridgeManager::redock(QETProject *project, Bridge *bridge, const QUuid &part)
{
	if (!project || !bridge) {
		return;
	}
	const auto parts = bridge->parts();
	for (const BridgePartData &data : parts) {
		if (data.uuid != part) {
			continue;
		}
		bridge->setPartDocks(part,
							 dockAt(project, bridge, data.p1),
							 dockAt(project, bridge, data.p2));
		return;
	}
}

/**
	@brief BridgeManager::followDocks
	@param project
	@param moved the symbol which was moved
*/
void BridgeManager::followDocks(QETProject *project, Element *moved)
{
	if (!project || !moved || !moved->diagram()) {
		return;
	}

	const auto bridges = project->bridges();
	for (Bridge *bridge : bridges)
	{
		if (!bridge) {
			continue;
		}
		for (const BridgePartData &data : bridge->parts())
		{
			QPointF p1 = data.p1;
			QPointF p2 = data.p2;
			bool follows = false;

			for (int end = 0 ; end < 2 ; ++end)
			{
				const QString dock = end ? data.dock2 : data.dock1;
				if (dock.isEmpty()) {
					continue;
				}
				bool found = false;
				const QPointF at = pinPosition(bridge, dock, moved, &found);
				if (!found) {
					continue;
				}
				if (end) {
					p2 = at;
				} else {
					p1 = at;
				}
				follows = true;
			}

			if (!follows) {
				continue;
			}
			bridge->setPartGeometry(data.uuid, p1, p2);
			for (BridgePart *part : BridgePart::allParts()) {
				if (part && part->partUuid() == data.uuid) {
					part->setLine(p1, p2);
				}
			}
		}
	}
}

/**
	@brief BridgeManager::wireOfBridgeAt
	@param project
	@param element
	@return the wire behind a bridge landing on that symbol, or nullptr
	when no bridge is plugged into it. The values a text of the symbol
	shows come from that wire: the bridge carries its voltage, and the
	wire is where it comes from.
*/
Conductor *BridgeManager::wireOfBridgeAt(QETProject *project, Element *element)
{
	if (!project || !element) {
		return nullptr;
	}
	const auto bridges = project->bridges();
	for (Bridge *bridge : bridges)
	{
		if (!bridge) {
			continue;
		}
		const auto lands_on = bridge->landings();
		bool lands_here = false;
		for (const BridgeTerminal &landing : lands_on) {
			if (elementFor(project, landing) == element) {
				lands_here = true;
				break;
			}
		}
		if (!lands_here) {
			continue;
		}
		for (const BridgeTerminal &landing : lands_on)
		{
			Element *landed = elementFor(project, landing);
			if (!landed) {
				continue;
			}
			for (Terminal *pin : landed->terminals()) {
				for (Conductor *wire : pin->conductors()) {
					if (wire) {
						return wire;
					}
				}
			}
		}
	}
	return nullptr;
}

/**
	@brief BridgeManager::locationOf
	@param project
	@param bridge
	@return where this bridge stands: which sheet, which column and
	which row of it -- the way a cross-reference names a place, so two
	bridges are told apart by where they are. Empty when the bridge
	lands on nothing of the drawing yet.
*/
QString BridgeManager::locationOf(QETProject *project, Bridge *bridge)
{
	if (!project || !bridge || bridge->terminals().isEmpty()) {
		return {};
	}
	Element *element = elementFor(project, bridge->terminals().first());
	if (!element || !element->diagram()) {
		return {};
	}
	return autonum::AssignVariables::genericXref(element);
}

/**
	@brief BridgeManager::bridgeLandsOn
	@param project
	@param element
	@return true when one of the bridges of the drawing lands on that
	symbol -- a terminal, or the connection of a sheet reference
*/
bool BridgeManager::bridgeLandsOn(QETProject *project, Element *element)
{
	if (!project || !element) {
		return false;
	}
	const auto bridges = project->bridges();
	for (Bridge *bridge : bridges)
	{
		if (!bridge) {
			continue;
		}
		const auto lands_on = bridge->terminals();
		for (const BridgeTerminal &terminal : lands_on) {
			if (elementFor(project, terminal) == element) {
				return true;
			}
		}
	}
	return false;
}

/**
	@brief BridgeManager::linkJoinsBridgeToWire
	@param project
	@param first one side of a link about to be made
	@param second the other side of it
	@return true when that link would join a bridge to a wire: a bar in
	the cabinet and a wire on the drawing are not one thing, whatever
	brings them to the same connection. Asked when the link is made --
	while a line is drawn nobody knows yet what it will meet.
*/
bool BridgeManager::linkJoinsBridgeToWire(QETProject *project,
										  Element *first,
										  Element *second)
{
	const bool first_on_bridge = bridgeLandsOn(project, first);
	const bool second_on_bridge = bridgeLandsOn(project, second);
	return first_on_bridge != second_on_bridge;
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
	return bridgeableElements(project).value(terminal.element);
}

/**
	@brief BridgeManager::informationOf
	@param project
	@param terminal
	@return what that very pin carries: the voltage/protocol of a wire
	wired to it, the very value the wires carry themselves. Read live, so
	the bridge follows them the way they follow each other. A symbol
	with several terminals is asked at the pin the bridge lands on:
	each of them may carry a different voltage and none of them carries
	a function across a bridge.
*/
BridgeInformation BridgeManager::informationOf(QETProject *project, const BridgeTerminal &terminal)
{
	BridgeInformation information;
	Element *element = elementFor(project, terminal);
	if (!element) {
		return information;
	}

	const auto pins = element->terminals();
	for (Terminal *pin : pins)
	{
		//Every pin of the terminal: a bridge sitting across a
		//terminal reaches all of them
		const auto conductors = pin->conductors();
		for (Conductor *conductor : conductors)
		{
			if (!conductor) {
				continue;
			}
			const QString tension = conductor->properties().m_tension_protocol;
			if (!tension.isEmpty()) {
				information.tension = tension;
				return information;
			}
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
	which the bridge takes. Every pin of every terminal is asked -- a
	symbol may carry several and each of them may say something else --
	so the question sees all of them and not only the one the line
	happened to run closest to. Only the voltage/protocol: a bridge
	crosses that and nothing else.
*/
QList<BridgeInformation> BridgeManager::informationSets(QETProject *project,
														const QList<BridgeTerminal> &terminals)
{
	QList<BridgeInformation> sets;
	for (const BridgeTerminal &terminal : terminals)
	{
		Element *element = elementFor(project, terminal);
		if (!element) {
			continue;
		}
		const auto pins = element->terminals();
		for (Terminal *pin : pins)
		{
			const auto conductors = pin->conductors();
			for (Conductor *conductor : conductors)
			{
				if (!conductor) {
					continue;
				}
				const QString tension = conductor->properties().m_tension_protocol;
				if (tension.isEmpty()) {
					continue;
				}
				const BridgeInformation information{tension};
				if (!sets.contains(information)) {
					sets << information;
				}
			}
		}
	}
	return sets;
}

/**
	@brief BridgeManager::writeInformation
	@param project
	@param terminals the terminals the bridge lands on
	@param information what the bridge carries
	@param parent the undo step this goes with

	Give the wires wired to those pins what the bridge carries, so the
	bridge and the wires it sits on say the same thing: one circle, never
	half of one. Only the voltage and the protocol cross a bridge -- never
	the function, which is the very business a separated function keeps
	to itself.

	Every wire which is connected goes with it: at the pin the bridge
	lands on, and on through the potential that pin belongs to, however
	far that reaches and however the function separation draws its lines.
	A wire which already says it is left alone; the rest goes with
	whatever was being done (@p parent), so it is one undo step.
*/
void BridgeManager::writeInformation(QETProject *project,
									 const QList<BridgeTerminal> &terminals,
									 const BridgeInformation &information,
									 QUndoCommand *parent)
{
	if (!project) {
		return;
	}

	QSet<Conductor *> conductors;
	for (const BridgeTerminal &terminal : terminals)
	{
		Element *element = elementFor(project, terminal);
		if (!element) {
			continue;
		}
		const auto pins = element->terminals();
		for (Terminal *pin : pins)
		{
			//Every pin of the terminal: a bridge sitting across a
			//terminal reaches all of them
			const auto attached = pin->conductors();
			for (Conductor *conductor : attached)
			{
				if (!conductor) {
					continue;
				}
				conductors << conductor;
				const auto related = conductor->relatedPotentialConductors();
				for (Conductor *other : related) {
					conductors << other;
				}
					//And over a function separation, the way the wires
					//themselves let the voltage cross it: the voltage
					//describes the node and not one side of it -- so an
					//edit reaches every terminal and every pin of the net
					//just as it does when made by hand on one wire
				const auto separated = conductor->functionSeparatedConductors();
				for (Conductor *other : separated) {
					conductors << other;
				}
			}
		}
	}

	for (Conductor *conductor : std::as_const(conductors))
	{
		if (!conductor) {
			continue;
		}
		const ConductorProperties before = conductor->properties();
		if (before.m_tension_protocol == information.tension) {
			continue;
		}
		ConductorProperties after = before;
		after.m_tension_protocol = information.tension;

		QVariant old_value, new_value;
		old_value.setValue(before);
		new_value.setValue(after);
		new QPropertyUndoCommand(conductor, "properties", old_value, new_value, parent);
	}
}

/**
	@brief BridgeManager::conductorsOverBridges
	@param project
	@param edited the wires being edited
	@return the wires the bridges reach from them.

	A bridge sits across the terminals it lands on, so a voltage reaching
	one of their wires reaches every wire of every terminal of the bridge
	-- and a wire of those reaches the next bridge sitting on its own
	terminal, however many bridges the way is paved with. Only the
	voltage and the protocol cross a bridge, never the function.
*/
QList<Conductor *> BridgeManager::conductorsOverBridges(QETProject *project,
													   const QList<Conductor *> &edited)
{
	QList<Conductor *> reached;
	if (!project || edited.isEmpty()) {
		return reached;
	}

	QSet<Conductor *> seen;
	QList<Conductor *> work;
	for (Conductor *conductor : edited) {
		if (conductor && !seen.contains(conductor)) {
			seen << conductor;
			work << conductor;
		}
	}

		//The pins reached so far: a bridge lands on a terminal whatever
		//pin of it the line ran closest to
	QSet<BridgeTerminal> touched;

	const auto bridges = project->bridges();
	while (!work.isEmpty())
	{
		Conductor *conductor = work.takeFirst();

		const auto ends = QList<Terminal *>{conductor->terminal1, conductor->terminal2};
		for (Terminal *end : ends)
		{
			Element *element = end ? end->parentElement() : nullptr;
			if (!element) {
				continue;
			}
			BridgeTerminal terminal;
			terminal.element = element->uuid();
			terminal.terminal = end->stableUuid().toString();
			touched << terminal;
		}

		const auto add = [&](QSet<Conductor *> others) {
			for (Conductor *other : others) {
				if (other && !seen.contains(other)) {
					seen << other;
					reached << other;
					work << other;
				}
			}
		};
		add(conductor->relatedPotentialConductors());
		add(conductor->functionSeparatedConductors());

		for (Bridge *bridge : bridges)
		{
			if (!bridge) {
				continue;
			}
			bool lands = false;
			const auto lands_on = bridge->terminals();
			for (const BridgeTerminal &terminal : lands_on) {
				for (const BridgeTerminal &mine : touched) {
					if (mine.element == terminal.element) {
						lands = true;
						break;
					}
				}
				if (lands) {
					break;
				}
			}
			if (!lands) {
				continue;
			}

			for (const BridgeTerminal &terminal : lands_on)
			{
				Element *element = elementFor(project, terminal);
				if (!element) {
					continue;
				}
				QSet<Conductor *> on_the_terminal;
				const auto pins = element->terminals();
				for (Terminal *pin : pins) {
					for (Conductor *wire : pin->conductors()) {
						if (wire && !seen.contains(wire)) {
							on_the_terminal << wire;
						}
					}
				}
				add(on_the_terminal);
			}
		}
	}
	return reached;
}

/**
	@brief BridgeManager::bridgesForStrip
	@param project
	@param strip
	@return what the drawing says about the bridges of that strip: every
	drawn bridge whose terminals sit in it, as the span it lies across --
	the bar sits on every terminal between its ends, so it takes its
	track there too.
*/
QList<TerminalStrip::DrawingBridge> BridgeManager::bridgesForStrip(QETProject *project,
																	TerminalStrip *strip)
{
	QList<TerminalStrip::DrawingBridge> list;
	if (!project || !strip) {
		return list;
	}

	const auto bridges = project->bridges();
	for (Bridge *bridge : bridges)
	{
		if (!bridge) {
			continue;
		}

			//The terminals of this strip the bridge lands on
		QVector<QSharedPointer<RealTerminal>> landed;
		const auto lands_on = bridge->terminals();
		for (const BridgeTerminal &terminal : lands_on)
		{
			Element *element = elementFor(project, terminal);
			auto *terminal_element = element ? dynamic_cast<TerminalElement *>(element) : nullptr;
			const QSharedPointer<RealTerminal> real_t =
					terminal_element ? terminal_element->realTerminal() : QSharedPointer<RealTerminal>();
			if (real_t && real_t->parentStrip() == strip) {
				landed << real_t;
			}
		}
		if (landed.isEmpty()) {
			continue;
		}

			//The bar lies across every terminal between its ends, but it
			//is plugged into the ones it was drawn over and no other: a
			//point on a terminal it only runs across would say the bridge
			//lands there, and it does not
		TerminalStrip::DrawingBridge drawn;
		drawn.source = bridge->uuid();
		drawn.real_terminals = landed;
		drawn.color = bridge->color();
			//The groove its bar sits in is the one the drawing gave it
		drawn.track = qBound(1, bridge->column(), 4);
		list << drawn;
	}
	return list;
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

