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
#include "changeelementdatacommand.h"
#include "../qetgraphicsitem/element.h"

ChangeElementDataCommand::ChangeElementDataCommand(Element *element, ElementData new_data, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_element(element),
	m_old_data(element->elementData()),
	m_new_data(new_data)
{
	setText(QObject::tr("Modifier les propriétés d'un élement"));
}

void ChangeElementDataCommand::undo() {
	if (m_element) {
		m_element.data()->setElementData(m_old_data);
	}
}

void ChangeElementDataCommand::redo() {
	if (m_element) {
		m_element.data()->setElementData(m_new_data);
	}
}
