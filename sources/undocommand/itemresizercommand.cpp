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
#include "itemresizercommand.h"
#include "qetgraphicsitem.h"
#include "diagram.h"

/**
 * @brief ItemResizerCommand::ItemResizerCommand
 * Change the size of @qgi
 * @param qgi item to resize
 * @param old_ old size
 * @param new_ new size
 * @param text text to display
 * @param parent undo parent
 */
ItemResizerCommand::ItemResizerCommand (QetGraphicsItem *qgi, const qreal &old_, const qreal &new_, const QString &text, QUndoCommand *parent):
	QUndoCommand(parent),
	m_qgi (qgi),
	m_old_size (old_),
	m_new_size (new_),
	m_diagram (qgi->diagram()),
	m_text (text),
	m_first_redo(true)
{
	m_animation.setTargetObject(m_qgi);
	m_animation.setPropertyName("scale");
	m_animation.setStartValue(m_old_size);
	m_animation.setEndValue(m_new_size);
}

/**
 * @brief ItemResizerCommand::~ItemResizerCommand
 */
ItemResizerCommand::~ItemResizerCommand() {}

/**
 * @brief ItemResizerCommand::mergeWith
 * Try to merge this command with other command
 * @param other
 * @return true if was merged, else false
 */
bool ItemResizerCommand::mergeWith(const QUndoCommand *other)
{
	if (id() != other->id() || other->childCount()) return false;
	ItemResizerCommand const *undo = static_cast<const ItemResizerCommand *>(other);
	if (m_qgi != undo->m_qgi) return false;
	m_new_size = undo->m_new_size;
	m_animation.setEndValue(m_new_size);
	return true;
}

/**
 * @brief ItemResizerCommand::undo
 */
void ItemResizerCommand::undo()
{
	m_diagram -> showMe();
	m_animation.setDirection(QAnimationGroup::Backward);
	m_animation.start();
	QUndoCommand::undo();
}

/**
 * @brief ItemResizerCommand::redo
 */
void ItemResizerCommand::redo()
{
	m_diagram -> showMe();
	setText(m_old_size < m_new_size ? QObject::tr("Agrandire %1 à %2 %").arg(m_text).arg(m_new_size*100) :
									  QObject::tr("Réduire %1 à %2 %").arg(m_text).arg(m_new_size*100));
	if(m_first_redo)
	{
		m_qgi -> setScale(m_new_size);
		m_first_redo = false;
	}
	else
	{
		m_animation.setDirection(QAnimationGroup::Forward);
		m_animation.start();
	}
	QUndoCommand::redo();
}
