/*
		Copyright 2006-2021 The QElectroTech Team
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
#include "bridgeterminalscommand.h"
#include "../terminalstrip.h"

BridgeTerminalsCommand::BridgeTerminalsCommand(TerminalStrip *strip,
											   QVector<QUuid> real_terminal_uuid,
											   QUndoCommand *parent):
	QUndoCommand(parent),
	m_strip(strip),
	m_uuid_vector(real_terminal_uuid)
{
	setText(QObject::tr("Ponter des bornes entre-elles"));
}

void BridgeTerminalsCommand::undo()
{
	if (m_strip) {
		m_strip->unBridge(m_uuid_vector);
	}
}

void BridgeTerminalsCommand::redo()
{
	if (m_strip) {
		m_strip->setBridge(m_uuid_vector);
	}
}

UnBridgeTerminalsCommand::UnBridgeTerminalsCommand(TerminalStrip *strip,
												   QVector<QUuid> real_terminal_uuid,
												   QUndoCommand *parent):
	QUndoCommand(parent),
	m_strip(strip)
{
	setText(QObject::tr("Supprimer des ponts de bornes"));

	for (const auto &t_uuid : real_terminal_uuid)
	{
		auto bridge = m_strip->bridgeFor(t_uuid);
		if (bridge) {
			m_bridge_terminal_map.insert(bridge->uuid_, t_uuid);
		}
	}
}

void UnBridgeTerminalsCommand::undo()
{
	if (m_strip) {
		for (const auto &bridge_uuid : m_bridge_terminal_map.uniqueKeys()) {
			auto terminal_list = m_bridge_terminal_map.values(bridge_uuid);
			m_strip->setBridge(bridge_uuid, terminal_list.toVector());
		}
	}
}

void UnBridgeTerminalsCommand::redo()
{
	if (m_strip) {
		m_strip->unBridge(m_bridge_terminal_map.values().toVector());
	}
}
