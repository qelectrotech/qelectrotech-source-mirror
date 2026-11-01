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
#include "bridgeundocommands.h"

#include "../diagram.h"
#include "../qetproject.h"
#include "bridgepart.h"
#include "bridgemanager.h"
#include "bridgemanager.h"

/**
	@brief ChangeBridgePropertiesCommand::ChangeBridgePropertiesCommand
	@param bridge
	@param color the colour the bridge is drawn in from now on
	@param max_connections how many terminals it may hold from now on
	@param line_width how thick its lines are drawn from now on
	@param name the name the bridge carries from now on
	@param parent
*/
ChangeBridgePropertiesCommand::ChangeBridgePropertiesCommand(Bridge *bridge,
															const QColor &color,
															int max_connections,
															qreal line_width,
															const QString &name,
															QUndoCommand *parent) :
	QUndoCommand(parent),
	m_bridge(bridge),
	m_color_before(bridge ? bridge->color() : QColor()),
	m_color_after(color),
	m_max_before(bridge ? bridge->maxConnections() : 0),
	m_max_after(qMax(max_connections, 0)),
	m_width_before(bridge ? bridge->lineWidth() : Bridge::default_line_width),
	m_width_after(qMax(line_width, 0.1)),
	m_name_before(bridge ? bridge->name() : QString()),
	m_name_after(name)
{
	setText(QObject::tr("Changing a bridge", "undo command text"));
}

/**
	@brief ChangeBridgePropertiesCommand::redo
*/
void ChangeBridgePropertiesCommand::redo()
{
	if (!m_bridge) return;
	m_bridge->setColor(m_color_after);
	m_bridge->setMaxConnections(m_max_after);
	m_bridge->setLineWidth(m_width_after);
	m_bridge->setName(m_name_after);
	QUndoCommand::redo();
}

/**
	@brief ChangeBridgePropertiesCommand::undo
*/
void ChangeBridgePropertiesCommand::undo()
{
	if (!m_bridge) return;
	m_bridge->setColor(m_color_before);
	m_bridge->setMaxConnections(m_max_before);
	m_bridge->setLineWidth(m_width_before);
	m_bridge->setName(m_name_before);
	QUndoCommand::undo();
}

/**
	@brief ChangeBridgeGeometryCommand::ChangeBridgeGeometryCommand
	@param bridge
	@param part the line which was moved
	@param line_before where it stood before
	@param line_after where it stands now
	@param terminals_before what the bridge landed on before
	@param terminals_after what it lands on now
	@param information_before what it carried before
	@param information_after what it carries now
	@param parent
*/
ChangeBridgeGeometryCommand::ChangeBridgeGeometryCommand(Bridge *bridge,
														 BridgePart *part,
														 const QLineF &line_before,
														 const QLineF &line_after,
														 const QList<BridgeTerminal> &terminals_before,
														 const QList<BridgeTerminal> &terminals_after,
														 const BridgeInformation &information_before,
														 const BridgeInformation &information_after,
														 QUndoCommand *parent) :
	QUndoCommand(parent),
	m_bridge(bridge),
	m_part(part),
	m_line_before(line_before),
	m_line_after(line_after),
	m_terminals_before(terminals_before),
	m_terminals_after(terminals_after),
	m_information_before(information_before),
	m_information_after(information_after)
{
	setText(QObject::tr("Changing the length of a bridge", "undo command text"));
	if (m_part) {
		m_part_uuid = m_part->partUuid();
	}
}

/**
	@brief ChangeBridgeGeometryCommand::redo
*/
void ChangeBridgeGeometryCommand::redo()
{
	apply(m_line_after, m_terminals_after, m_information_after);
	QUndoCommand::redo();
}

/**
	@brief ChangeBridgeGeometryCommand::undo
*/
void ChangeBridgeGeometryCommand::undo()
{
	apply(m_line_before, m_terminals_before, m_information_before);
	QUndoCommand::undo();
}

/**
	@brief ChangeBridgeGeometryCommand::apply
	Put the line, the terminals and the information of the bridge where
	they belong: the geometry first, so that anything watching the bridge
	sees the two agree once the step is done.
	@param line
	@param terminals
	@param information
*/
void ChangeBridgeGeometryCommand::apply(const QLineF &line,
										const QList<BridgeTerminal> &terminals,
										const BridgeInformation &information)
{
	if (m_bridge) {
		m_bridge->setPartGeometry(m_part_uuid, line.p1(), line.p2());
		m_bridge->setTerminals(terminals);
		m_bridge->setTensionText(information.tension);
	}
	if (m_part) {
		m_part->setLine(line.p1(), line.p2());
	}
		//Where the ends stand now, whatever the step did to them: an end
		//on a pin sticks to it, an end away from every pin stands on its
		//own
	if (m_bridge) {
		QETProject *project = qobject_cast<QETProject *>(m_bridge->parent());
		BridgeManager::redock(project, m_bridge.data(), m_part_uuid);
	}
}

/**
	@brief MergeBridgesCommand::MergeBridgesCommand
	@param from the bridge which gives up what it holds
	@param into the bridge the reference already carries
	@param parent the step this goes with
*/
MergeBridgesCommand::MergeBridgesCommand(Bridge *from,
										 Bridge *into,
										 QUndoCommand *parent) :
	QUndoCommand(parent),
	m_from(from),
	m_into(into)
{
	setText(QObject::tr("Joining a bridge over a sheet reference", "undo command text"));
}

/**
	@brief MergeBridgesCommand::redo
	One unit: everything @p from held goes onto @p into, and what it
	carried as its voltage goes with it when the unit carried none of
	its own -- the reference one bridge stands on is the reference of
	both halves of it.
*/
void MergeBridgesCommand::redo()
{
	if (!m_from || !m_into) return;
	QETProject *proj = qobject_cast<QETProject *>(m_into->parent());
	if (!proj) return;

		//What is moved is what the other bridge does not hold already:
		//a terminal both of them land on is no terminal to move back
	m_parts.clear();
	const auto parts = m_from->parts();
	for (const BridgePartData &part : parts) {
		if (!m_into->hasPart(part.uuid)) {
			m_parts << part;
		}
	}
	m_terminals.clear();
	const auto landings = m_from->landings();
	for (const BridgeTerminal &landing : landings) {
		if (!m_into->hasTerminal(landing)) {
			m_terminals << landing;
		}
	}

	for (const BridgePartData &part : std::as_const(m_parts)) {
		m_into->addPart(part);
		m_from->removePart(part.uuid);
	}
	for (const BridgeTerminal &terminal : std::as_const(m_terminals)) {
		m_into->addTerminal(terminal);
		m_from->removeTerminal(terminal);
	}
		//What is drawn follows its bridge: the line items belong to the
		//unit from now on, so clicking one speaks of it
	const auto items = BridgePart::allParts();
	for (BridgePart *item : items) {
		if (!item) {
			continue;
		}
		for (const BridgePartData &part : std::as_const(m_parts)) {
			if (item->partUuid() == part.uuid) {
				item->setBridge(m_into.data());
			}
		}
	}
	proj->removeBridge(m_from.data());

	QUndoCommand::redo();
}

/**
	@brief MergeBridgesCommand::undo
	Two again: the one which came gives back everything it was made to
	give, and the project holds both of them as they stood.
*/
void MergeBridgesCommand::undo()
{
	if (!m_from || !m_into) return;
	QETProject *proj = qobject_cast<QETProject *>(m_into->parent());
	if (!proj) return;

	proj->addBridge(m_from.data());
	for (const BridgeTerminal &terminal : std::as_const(m_terminals)) {
		m_into->removeTerminal(terminal);
		m_from->addTerminal(terminal);
	}
	for (const BridgePartData &part : std::as_const(m_parts)) {
		m_into->removePart(part.uuid);
		m_from->addPart(part);
	}
	const auto items = BridgePart::allParts();
	for (BridgePart *item : items) {
		if (!item) {
			continue;
		}
		for (const BridgePartData &part : std::as_const(m_parts)) {
			if (item->partUuid() == part.uuid) {
				item->setBridge(m_from.data());
			}
		}
	}

	QUndoCommand::undo();
}

/**
	@brief ChangeBridgeTerminalsCommand::ChangeBridgeTerminalsCommand
	@param bridge
	@param before what the bridge landed on before
	@param after what it lands on now
	@param information_before what it carried before
	@param information_after what it carries now
	@param parent
*/
ChangeBridgeTerminalsCommand::ChangeBridgeTerminalsCommand(Bridge *bridge,
														   const QList<BridgeTerminal> &before,
														   const QList<BridgeTerminal> &after,
														   const BridgeInformation &information_before,
														   const BridgeInformation &information_after,
														   QUndoCommand *parent) :
	QUndoCommand(parent),
	m_bridge(bridge),
	m_terminals_before(before),
	m_terminals_after(after),
	m_information_before(information_before),
	m_information_after(information_after)
{
	setText(QObject::tr("Changing the terminals of a bridge", "undo command text"));
}

/**
	@brief ChangeBridgeTerminalsCommand::redo
*/
void ChangeBridgeTerminalsCommand::redo()
{
	if (!m_bridge) return;
	m_bridge->setTerminals(m_terminals_after);
	m_bridge->setTensionText(m_information_after.tension);
	QUndoCommand::redo();
}

/**
	@brief ChangeBridgeTerminalsCommand::undo
*/
void ChangeBridgeTerminalsCommand::undo()
{
	if (!m_bridge) return;
	m_bridge->setTerminals(m_terminals_before);
	m_bridge->setTensionText(m_information_before.tension);
	QUndoCommand::undo();
}

/**
	@brief RemoveBridgeCommand::RemoveBridgeCommand
	@param parts the drawn lines to take away
*/
RemoveBridgeCommand::RemoveBridgeCommand(const QList<BridgePart *> &parts)
{
	setText(QObject::tr("Deleting a line of a bridge", "undo command text"));

	for (BridgePart *part : parts)
	{
		if (!part || !part->bridge()) {
			continue;
		}
		bool known = false;
		for (const Entry &entry : std::as_const(m_entries)) {
			if (entry.part == part) {
				known = true;
				break;
			}
		}
		if (known) {
			continue;
		}

		Entry entry;
		entry.part = part;
		entry.folio = part->diagram();
		entry.bridge = part->bridge();
			//The whole section as the bridge holds it, docks included:
			//an end standing on a pin sticks to it again when the undo
			//puts the line back, and only the drawn ends are said anew
		entry.data.uuid = part->partUuid();
		entry.data.diagram = entry.folio ? entry.folio->uuid() : QUuid();
		entry.data.p1 = part->firstPoint();
		entry.data.p2 = part->secondPoint();
		const auto held = entry.bridge ? entry.bridge->parts() : QList<BridgePartData>();
		for (const BridgePartData &data : held) {
			if (data.uuid == entry.data.uuid) {
				entry.data.dock1 = data.dock1;
				entry.data.dock2 = data.dock2;
				break;
			}
		}
		m_entries << entry;
	}

		//What each bridge lands on before the lines go, and what is left
		//of it afterwards: what only those lines crossed goes with them
	for (const Entry &entry : std::as_const(m_entries)) {
		if (entry.bridge && !m_terminals_before.contains(entry.bridge.data())) {
			m_terminals_before.insert(entry.bridge.data(), entry.bridge->terminals());
		}
	}

	for (auto it = m_terminals_before.constBegin();
		 it != m_terminals_before.constEnd(); ++it)
	{
		Bridge *bridge = it.key();
		if (!bridge) {
			continue;
		}

		int lines_left = 0;
		const auto parts_of_bridge = bridge->parts();
			//Which sections of this bridge go with the delete and which
			//stay
		QList<BridgePartData> staying;
		for (const BridgePartData &data : parts_of_bridge)
		{
			bool goes = false;
			for (const Entry &entry : std::as_const(m_entries)) {
				if (entry.bridge == bridge && entry.data.uuid == data.uuid) {
					goes = true;
					break;
				}
			}
			if (goes) {
				continue;
			}
			++lines_left;
			staying << data;
		}

			//What the staying lines cross on the folios which can
			//answer: a folio not built at the open holds no items to ask
			//-- what the bridge holds on one comes from that folio's own
			//kept XML instead, else deleting one section takes the
			//Klemmen of another one's folio away on redo, the very loss
			//this branch fixes elsewhere (review of #1651)
		QETProject *proj = project();
		const QSet<QUuid> cold_elements = proj
				? BridgeManager::elementsOnUnbuiltFolios(proj, staying)
				: QSet<QUuid>();
		QList<BridgeTerminal> crossed_now;
		for (const BridgePartData &data : staying)
		{
			Diagram *folio = proj ? proj->diagramByUuid(data.diagram) : nullptr;
			if (!folio || !folio->isLoaded()) {
				continue;
			}
				//What this line which stays crosses: the bridge goes on
				//landing on it
			const auto crossings =
					BridgeManager::crossedTerminals(folio, QLineF(data.p1, data.p2));
			for (const BridgeCrossing &crossing : crossings) {
				BridgeTerminal terminal;
				terminal.element = crossing.element->uuid();
				terminal.terminal = crossing.terminal;
				terminal.reference = crossing.reference;
				if (!crossed_now.contains(terminal)) {
					crossed_now << terminal;
				}
			}
		}

			//The bridge keeps the order it held its Klemmen in -- the
			//first of them is where it takes its information from --
			//and whatever the staying lines cross anew is appended
		const QList<BridgeTerminal> held = m_terminals_before.value(bridge);
		QList<BridgeTerminal> after;
		for (const BridgeTerminal &terminal : held) {
			if (cold_elements.contains(terminal.element)
				|| crossed_now.contains(terminal)) {
				after << terminal;
			}
		}
		for (const BridgeTerminal &terminal : crossed_now) {
			if (!held.contains(terminal)) {
				after << terminal;
			}
		}
		m_terminals_after.insert(bridge, after);

			//What draws nothing is no bridge of this project
		if (!lines_left) {
			m_bridges << bridge;
		}
	}
}

/**
	@brief RemoveBridgeCommand::project
	@return the project this command works in, or nullptr
*/
QETProject *RemoveBridgeCommand::project() const
{
	for (const Entry &entry : m_entries) {
		if (entry.folio && entry.folio->project()) {
			return entry.folio->project();
		}
	}
	return nullptr;
}

/**
	@brief RemoveBridgeCommand::redo
	Take the selected lines off their folios and what they crossed off
	their bridges. A bridge with no line left goes out of the project.
*/
void RemoveBridgeCommand::redo()
{
	for (const Entry &entry : std::as_const(m_entries))
	{
		if (entry.bridge) {
			entry.bridge->removePart(entry.data.uuid);
		}
		if (entry.part && entry.part->scene()) {
			entry.part->scene()->removeItem(entry.part.data());
		}
	}

	for (auto it = m_terminals_after.constBegin();
		 it != m_terminals_after.constEnd(); ++it)
	{
		if (it.key()) {
			it.key()->setTerminals(it.value());
		}
	}

	QETProject *proj = project();
	if (!proj) {
		return;
	}
	for (const QPointer<Bridge> &bridge : std::as_const(m_bridges))
	{
		if (bridge && proj->bridges().contains(bridge.data())) {
			proj->removeBridge(bridge.data());
		}
	}
}

/**
	@brief RemoveBridgeCommand::undo
	Put the lines back on their folios, the sections back in their
	bridges and the terminals back on them: exactly the state the delete
	action took away.
*/
void RemoveBridgeCommand::undo()
{
	QETProject *proj = project();
	if (proj)
	{
		for (const QPointer<Bridge> &bridge : std::as_const(m_bridges))
		{
			if (bridge && !proj->bridges().contains(bridge.data())) {
				proj->addBridge(bridge.data());
			}
		}
	}

	for (const Entry &entry : std::as_const(m_entries))
	{
		if (entry.bridge && !entry.bridge->hasPart(entry.data.uuid)) {
			entry.bridge->addPart(entry.data);
		}
		if (entry.part && !entry.part->scene() && entry.folio) {
			entry.folio->addItem(entry.part.data());
		}
	}

	for (auto it = m_terminals_before.constBegin();
		 it != m_terminals_before.constEnd(); ++it)
	{
		if (it.key()) {
			it.key()->setTerminals(it.value());
		}
	}
}
