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
#include "changeshapestylecommand.h"
#include "qetshapeitem.h"
#include "diagram.h"

/**
 * @brief ChangeShapeStyleCommand::ChangeShapeStyleCommand
 * Constructor
 * @param item : shape to change
 * @param old_ps : old style
 * @param new_ps : new style
 * @param parent : parent undo
 */
ChangeShapeStyleCommand::ChangeShapeStyleCommand(QetShapeItem *item, const Qt::PenStyle &old_ps, const Qt::PenStyle new_ps, QUndoCommand *parent):
	QUndoCommand(parent),
	m_shape(item),
	m_old_ps(old_ps),
	m_new_ps(new_ps)
{
	setText(QObject::tr("Changer le style d'une primitive"));
}

/**
 * @brief ChangeShapeStyleCommand::mergeWith
 * Try to merge this command with other
 * @param other
 * @return true if was merged
 */
bool ChangeShapeStyleCommand::mergeWith(const QUndoCommand *other)
{
	if (id() != other->id() || other->childCount()) return false;
	ChangeShapeStyleCommand const *undo = static_cast<const ChangeShapeStyleCommand*>(other);
	if(m_shape != undo->m_shape) return false;
	m_new_ps = undo->m_new_ps;
	return true;
}

/**
 * @brief ChangeShapeStyleCommand::undo
 * undo this command
 */
void ChangeShapeStyleCommand::undo()
{
	if (m_shape->diagram()) m_shape->diagram()->showMe();
	m_shape->setStyle(m_old_ps);
	QUndoCommand::undo();
}

/**
 * @brief ChangeShapeStyleCommand::redo
 * redo this command
 */
void ChangeShapeStyleCommand::redo()
{
	if (m_shape->diagram()) m_shape->diagram()->showMe();
	m_shape->setStyle(m_new_ps);
	QUndoCommand::redo();
}
