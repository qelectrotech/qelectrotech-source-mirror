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
#include "addbridgecommand.h"

#include "../diagram.h"
#include "../qetproject.h"
#include "bridgepart.h"

/**
	@brief AddBridgeCommand::AddBridgeCommand
	@param bridge the bridge the line is drawn for
	@param part the line itself
	@param diagram the folio it stands on
	@param own_bridge true when this command brings the bridge into
	existence (a brand new bridge rather than a line on an existing one)
	@param terminals the terminals the line gives to the bridge
*/
AddBridgeCommand::AddBridgeCommand(Bridge *bridge,
								   BridgePart *part,
								   Diagram *diagram,
								   bool own_bridge,
								   const QList<BridgeTerminal> &terminals) :
	QUndoCommand(),
	m_bridge(bridge),
	m_part(part),
	m_diagram(diagram),
	m_terminals(terminals),
	m_own_bridge(own_bridge)
{
	setText(QObject::tr("Drawing a bridge", "undo command text"));
	if (m_part) {
		m_part_data.uuid = m_part->partUuid();
		m_part_data.diagram = diagram ? diagram->uuid() : QUuid();
		m_part_data.p1 = m_part->firstPoint();
		m_part_data.p2 = m_part->secondPoint();
	}
}

/**
	@brief AddBridgeCommand::project
	@return the project this command works in, or nullptr
*/
QETProject *AddBridgeCommand::project() const
{
	return m_diagram ? m_diagram->project() : nullptr;
}

/**
	@brief AddBridgeCommand::redo
	Put the line on the folio, together with everything the bridge
	holds for it.
*/
void AddBridgeCommand::redo()
{
	QETProject *proj = project();
	if (!proj || !m_bridge || !m_part || !m_diagram) return;

	if (m_own_bridge) {
			//The bridge was filled in before this command was made: it
			//only has to be held by the project again.
		proj->addBridge(m_bridge.data());
	} else {
		if (!m_bridge->hasPart(m_part_data.uuid)) {
			m_bridge->addPart(m_part_data);
		}
		for (const BridgeTerminal &terminal : std::as_const(m_terminals)) {
			m_bridge->addTerminal(terminal);
		}
	}

	if (m_part->scene() != m_diagram) {
		m_diagram->addItem(m_part);
	}
}

/**
	@brief AddBridgeCommand::undo
	Take the line back off the folio. An existing bridge loses the
	section and the terminals it was carrying on it; a brand new bridge
	is taken out of the project whole, keeping everything it knows, so
	redo finds it exactly as it was.
*/
void AddBridgeCommand::undo()
{
	QETProject *proj = project();
	if (!proj || !m_bridge || !m_part || !m_diagram) return;

	if (m_part->scene()) {
		m_diagram->removeItem(m_part);
	}

	if (m_own_bridge) {
		proj->removeBridge(m_bridge.data());
	} else {
		m_bridge->removePart(m_part_data.uuid);
		for (const BridgeTerminal &terminal : std::as_const(m_terminals)) {
			m_bridge->removeTerminal(terminal);
		}
	}
}
