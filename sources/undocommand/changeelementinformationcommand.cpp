/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "changeelementinformationcommand.h"
#include "element.h"
#include <QObject>

/**
 * @brief ChangeElementInformationCommand::ChangeElementInformationCommand
 * Default constructor
 * @param elmt : element to change information
 * @param old_info : old info of element
 * @param new_info : new info of element
 */
ChangeElementInformationCommand::ChangeElementInformationCommand(Element *elmt, DiagramContext &old_info, DiagramContext &new_info, QUndoCommand *parent) :
	QUndoCommand (parent),
	m_element    (elmt),
	m_old_info   (old_info),
	m_new_info   (new_info)
{
	setText(QObject::tr("Modifier les informations de l'\351l\351ment : %1").arg(elmt -> name()));
}

/**
 * @brief ChangeElementInformationCommand::undo
 */
void ChangeElementInformationCommand::undo() {
	m_element -> setElementInformations(m_old_info);
}

/**
 * @brief ChangeElementInformationCommand::redo
 */
void ChangeElementInformationCommand::redo() {
	m_element -> setElementInformations(m_new_info);
}
