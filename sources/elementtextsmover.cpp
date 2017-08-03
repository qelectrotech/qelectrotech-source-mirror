/*
	Copyright 2006-2012 Xavier Guerrin
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
#include "elementtextsmover.h"
#include "elementtextitem.h"
#include "diagram.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "dynamicelementtextitem.h"

/**
 * @brief ElementTextsMover::ElementTextsMover
 */
ElementTextsMover::ElementTextsMover() {}

/**
 * @brief ElementTextsMover::isReady
 * @return true if this ElementTextsMover is ready to process a new movement.
 * False if this ElementTextsMover is actually process a movement
 */
bool ElementTextsMover::isReady() const {
	return(!m_movement_running);
}

/**
 * @brief ElementTextsMover::beginMovement
 * Begin a movement
 * @param diagram : diagram where the movement is apply
 * @param driver_item : item moved by the mouse
 * @return : the number of moved text (driver_item include), or -1 if this ElementTextsMover can't begin a movement
 */
int ElementTextsMover::beginMovement(Diagram *diagram, QGraphicsItem *driver_item)
{
	if (m_movement_running || !diagram) return(-1);

	m_diagram = diagram;
	m_movement_driver = driver_item;
	m_texts_item_H.clear();

	for(QGraphicsItem *item : diagram->selectedItems())
	{
		if (item->type() == ElementTextItem::Type || item->type() == DynamicElementTextItem::Type)
		{
			DiagramTextItem *dti = static_cast<DiagramTextItem *> (item);
			m_texts_item_H.insert(dti, dti->pos());
		}
	}
	
	if (!m_texts_item_H.size()) return(-1);
	
	m_movement_running = true;
	
	return(m_texts_item_H.size());
}

/**
 * @brief ElementTextsMover::continueMovement
 * Add @movement to the current movement
 * The movement must be in scene coordinate.
 * @param movement
 */
void ElementTextsMover::continueMovement(const QPointF &movement)
{
	if (!m_movement_running || movement.isNull()) return;
	
	for(DiagramTextItem *text_item : m_texts_item_H.keys())
	{
		if (text_item == m_movement_driver)
			continue;
		QPointF applied_movement = text_item->mapMovementToParent(text_item->mapMovementFromScene(movement));
		text_item->setPos(text_item->pos() + applied_movement);
	}
}

/**
 * @brief ElementTextsMover::endMovement
 * Finish the movement by pushing an undo command to the parent diagram of text item
 */
void ElementTextsMover::endMovement()
{
		//No movement running, or no text to move
	if (!m_movement_running || m_texts_item_H.isEmpty())
		return;
	
		//Movement is null
	DiagramTextItem *dti = m_texts_item_H.keys().first();
	if (dti->pos() == m_texts_item_H.value(dti))
		return;
	
	QUndoCommand *undo = new QUndoCommand(m_texts_item_H.size() == 1 ? QString(QObject::tr("Déplacer un texte d'élément"))  :
																	   QString(QObject::tr("Déplacer %1 textes d'élément").arg(m_texts_item_H.size())));
	
	for (DiagramTextItem *dti : m_texts_item_H.keys())
	{
		QPropertyUndoCommand *child_undo = new QPropertyUndoCommand(dti, "pos", m_texts_item_H.value(dti), dti->pos(), undo);
		child_undo->enableAnimation();

	}

	m_diagram->undoStack().push(undo);
	
	m_movement_running = false;
}
