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
#include "groupterminalscommand.h"

/**
 * @brief GroupTerminalsCommand::GroupTerminalsCommand
 * @param strip : The parent strip of terminal to group
 * @param receiver : The terminal where other terminals will be grouped
 * @param to_group : Terminals to group
 */
GroupTerminalsCommand::GroupTerminalsCommand(TerminalStrip *strip,
											 const PhysicalTerminalData &receiver_,
											 const QVector<RealTerminalData> &to_group,
											 QUndoCommand *parent):
	QUndoCommand(parent),
	m_terminal_strip(strip),
	m_receiver(receiver_),
	m_to_group(to_group)
{
	setText("Grouper un ensemble de bornes");
}

void GroupTerminalsCommand::undo() {
	if (m_terminal_strip) {
		m_terminal_strip->unGroupTerminals(m_to_group);
	}
}

void GroupTerminalsCommand::redo() {
	if (m_terminal_strip) {
		m_terminal_strip->groupTerminals(m_receiver, m_to_group);
	}
}
