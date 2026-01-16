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
#include "changeterminallevel.h"
#include "../realterminal.h"

ChangeTerminalLevel::ChangeTerminalLevel(TerminalStrip *strip,
										 const QWeakPointer<RealTerminal> &real_terminal,
										 int level,
										 QUndoCommand *parent) :
	QUndoCommand(parent),
	m_strip(strip),
	m_real_terminal(real_terminal),
	m_new_level(level),
	m_old_level(real_terminal.toStrongRef()->level())
{}

void ChangeTerminalLevel::undo()
{
	if (m_strip) {
		m_strip->setLevel(m_real_terminal, m_old_level);
	}
}

void ChangeTerminalLevel::redo()
{
	if (m_strip) {
		m_strip->setLevel(m_real_terminal, m_new_level);
	}
}
