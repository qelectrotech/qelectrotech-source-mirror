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

/**
	@brief ChangeBridgePropertiesCommand::ChangeBridgePropertiesCommand
	@param bridge
	@param color the colour the bridge is drawn in from now on
	@param max_connections how many terminals it may hold from now on
	@param line_width how thick its lines are drawn from now on
	@param parent
*/
ChangeBridgePropertiesCommand::ChangeBridgePropertiesCommand(Bridge *bridge,
															const QColor &color,
															int max_connections,
															qreal line_width,
															QUndoCommand *parent) :
	QUndoCommand(parent),
	m_bridge(bridge),
	m_color_before(bridge ? bridge->color() : QColor()),
	m_color_after(color),
	m_max_before(bridge ? bridge->maxConnections() : 0),
	m_max_after(qMax(max_connections, 0)),
	m_width_before(bridge ? bridge->lineWidth() : Bridge::default_line_width),
	m_width_after(qMax(line_width, 0.1))
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
}

/**
	@brief ChangeBridgeGeometryCommand::undo
*/
void ChangeBridgeGeometryCommand::undo()
{
	apply(m_line_before, m_terminals_before, m_information_before);
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
		m_bridge->setFunctionText(information.function);
		m_bridge->setTensionText(information.tension);
	}
	if (m_part) {
		m_part->setLine(line.p1(), line.p2());
	}
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
	m_bridge->setFunctionText(m_information_after.function);
	m_bridge->setTensionText(m_information_after.tension);
}

/**
	@brief ChangeBridgeTerminalsCommand::undo
*/
void ChangeBridgeTerminalsCommand::undo()
{
	if (!m_bridge) return;
	m_bridge->setTerminals(m_terminals_before);
	m_bridge->setFunctionText(m_information_before.function);
	m_bridge->setTensionText(m_information_before.tension);
}

/**
	@brief RemoveBridgeCommand::RemoveBridgeCommand
	@param parts any lines of the bridges to take away
*/
RemoveBridgeCommand::RemoveBridgeCommand(const QList<BridgePart *> &parts)
{
	setText(QObject::tr("Deleting a bridge", "undo command text"));

	for (BridgePart *part : parts)
	{
		if (!part || !part->bridge()) {
			continue;
		}
		Bridge *bridge = part->bridge();
		if (m_bridges.contains(QPointer<Bridge>(bridge))) {
			continue;
		}
		m_bridges << bridge;

			//Every line of the bridge goes with it, on whichever folio it
			//stands: half a bridge is of no use to anybody.
		QETProject *proj = bridge->parent()
				? qobject_cast<QETProject *>(bridge->parent()) : nullptr;
		for (const BridgePartData &data : bridge->parts())
		{
			Entry entry;
			entry.part = nullptr;
			entry.folio = proj ? proj->diagramByUuid(data.diagram) : nullptr;
			if (entry.folio) {
				const auto items = entry.folio->items();
				for (QGraphicsItem *item : items) {
					if (auto *candidate = qgraphicsitem_cast<BridgePart *>(item)) {
						if (candidate->partUuid() == data.uuid) {
							entry.part = candidate;
							break;
						}
					}
				}
			}
			m_entries << entry;
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
	Take every line of the bridges off their folios and the bridges out
	of the project. They keep everything they know: undo finds them
	exactly as they were.
*/
void RemoveBridgeCommand::redo()
{
	for (const Entry &entry : std::as_const(m_entries))
	{
		if (entry.part && entry.part->scene()) {
			entry.part->scene()->removeItem(entry.part.data());
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
	Put the bridges back into the project and draw their lines again,
	exactly where they stood.
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
		if (entry.part && !entry.part->scene() && entry.folio) {
			entry.folio->addItem(entry.part.data());
		}
	}
}
