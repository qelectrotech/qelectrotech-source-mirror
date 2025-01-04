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
#include "changeterminalstripdata.h"

ChangeTerminalStripData::ChangeTerminalStripData(TerminalStrip *strip,
												 const TerminalStripData &data,
												 QUndoCommand *parent) :
	QUndoCommand(parent),
	m_strip(strip),
	m_new_data(data)
{
	setText(QObject::tr("Modifier les proriétés d'un groupe de bornes"));
	m_old_data = strip->data();
}

void ChangeTerminalStripData::undo()
{
	if (m_strip) {
		m_strip->setData(m_old_data);
	}
}

void ChangeTerminalStripData::redo()
{
	if (m_strip) {
		m_strip->setData(m_new_data);
	}
}
