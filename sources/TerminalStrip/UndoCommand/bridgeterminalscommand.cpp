/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "../terminalstripbridge.h"

BridgeTerminalsCommand::BridgeTerminalsCommand(TerminalStrip *strip,
											   QVector<QSharedPointer<RealTerminal>> real_terminal,
											   QUndoCommand *parent):
	QUndoCommand(parent),
	m_strip(strip),
	m_real_terminal_vector(real_terminal)
{
	setText(QObject::tr("Ponter des bornes entre-elles"));
}

void BridgeTerminalsCommand::undo()
{
	if (m_strip) {
		m_strip->unBridge(m_real_terminal_vector);
	}
}

void BridgeTerminalsCommand::redo()
{
	if (m_strip)
	{
		if (m_bridge) {
			m_strip->setBridge(m_bridge, m_real_terminal_vector);
		} else {
			m_strip->setBridge(m_real_terminal_vector);
			m_bridge = m_strip->isBridged(m_real_terminal_vector.first());
		}
	}
}

UnBridgeTerminalsCommand::UnBridgeTerminalsCommand(TerminalStrip *strip,
												   QVector<QSharedPointer<RealTerminal>> real_terminal,
												   QUndoCommand *parent):
	QUndoCommand(parent),
	m_strip(strip)
{
	setText(QObject::tr("Supprimer des ponts de bornes"));

	if (strip->canUnBridge(real_terminal))
	{
		m_bridge = strip->isBridged(real_terminal.first());

			//If bridge have one more terminals than @real_terminal
			//that mean every terminals of the bridge must be unbridged by this undo command,
			//else the single terminal who's not umbridged by this undo command
			//continue to have a bridge (to nothing) and this nowhere bridge is visible
			//in the terminal strip graphic item and terminal strip editor dialog.
		if (m_bridge->realTerminals().size() == real_terminal.size() + 1) {
			m_terminals = m_bridge->realTerminals();
		} else {
			m_terminals = real_terminal;
		}
	}
}

void UnBridgeTerminalsCommand::undo()
{
	if (m_strip && m_bridge) {
		m_strip->setBridge(m_bridge, m_terminals);
	}
}

void UnBridgeTerminalsCommand::redo()
{
	if (m_strip) {
		m_strip->unBridge(m_terminals);
	}
}
