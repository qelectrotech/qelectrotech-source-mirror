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
#include "changeterminalstripcolor.h"
#include "../terminalstripbridge.h"

/**
 * @brief ChangeTerminalStripColor::ChangeTerminalStripColor
 * @param bridge
 * @param color
 * @param parent
 */
ChangeTerminalStripColor::ChangeTerminalStripColor(QSharedPointer<TerminalStripBridge> bridge,
												   const QColor &color,
												   QUndoCommand *parent):
	QUndoCommand(parent),
	m_bridge(bridge),
	m_new_color(color)
{
	if (m_bridge) {
		m_old_color = m_bridge->color();
	}
	setText(QObject::tr("Modifier la couleur d'un pont de bornes"));
}

void ChangeTerminalStripColor::redo()
{
	if (m_bridge) {
		m_bridge->setColor(m_new_color);
	}
}

void ChangeTerminalStripColor::undo()
{
	if (m_bridge) {
		m_bridge->setColor(m_old_color);
	}
}
