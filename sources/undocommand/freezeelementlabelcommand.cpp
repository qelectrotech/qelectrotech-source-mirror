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
#include "freezeelementlabelcommand.h"

#include "../qetgraphicsitem/element.h"

FreezeElementLabelCommand::FreezeElementLabelCommand(Element *element,
													 bool old_frozen,
													 bool new_frozen,
													 QUndoCommand *parent) :
	QUndoCommand(new_frozen ? QObject::tr("Freeze element name")
							: QObject::tr("Unfreeze element name"),
				 parent),
	m_element(element),
	m_old_frozen(old_frozen),
	m_new_frozen(new_frozen)
{}

void FreezeElementLabelCommand::undo()
{
	if (m_element) {
		m_element->freezeLabel(m_old_frozen);
	}
}

void FreezeElementLabelCommand::redo()
{
	if (m_element) {
		m_element->freezeLabel(m_new_frozen);
	}
}
