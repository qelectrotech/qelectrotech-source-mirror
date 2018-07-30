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
#include "diagram.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "dynamicelementtextitem.h"
#include "elementtextitemgroup.h"
#include <QObject>

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
	if (m_movement_running || !diagram)
		return(-1);

	m_diagram = diagram;
	m_movement_driver = driver_item;
	m_items_hash.clear();
	m_text_count = m_group_count =0;
	
	for(QGraphicsItem *item : diagram->selectedItems())
	{
		 if(item->type() == DynamicElementTextItem::Type)
		 {
			 m_items_hash.insert(item, item->pos());
			 m_text_count++;
		 }
		 else if(item->type() == QGraphicsItemGroup::Type)
		 {
			 if(dynamic_cast<ElementTextItemGroup *>(item))
			 {
				 m_items_hash.insert(item, item->pos());
				 m_group_count++;
			 }
		 }
	}
	
	
	if(m_items_hash.isEmpty())
		return -1;
	
	m_movement_running = true;
	
	return m_items_hash.size();
}

void ElementTextsMover::continueMovement(QGraphicsSceneMouseEvent *event)
{
	if(!m_movement_running)
		return;

	for(QGraphicsItem *qgi : m_items_hash.keys())
	{
		if(qgi == m_movement_driver)
			continue;
		
		QPointF current_parent_pos;
		QPointF button_down_parent_pos;

		current_parent_pos = qgi->mapToParent(qgi->mapFromScene(event->scenePos()));
		button_down_parent_pos = qgi->mapToParent(qgi->mapFromScene(event->buttonDownScenePos(Qt::LeftButton)));
		
		QPointF new_pos = m_items_hash.value(qgi) + current_parent_pos - button_down_parent_pos;
		event->modifiers() == Qt::ControlModifier ? qgi->setPos(new_pos) : qgi->setPos(Diagram::snapToGrid(new_pos));
	}
}

/**
 * @brief ElementTextsMover::endMovement
 * Finish the movement by pushing an undo command to the parent diagram of text item
 */
void ElementTextsMover::endMovement()
{	
		//No movement or no items to move
	if(!m_movement_running || m_items_hash.isEmpty())
		return;
			
		//Movement is null
	QGraphicsItem *qgi = m_items_hash.keys().first();
	if(qgi->pos() == m_items_hash.value(qgi))
		return;
					 
	QUndoCommand *undo = new QUndoCommand(undoText());
	
	for (QGraphicsItem *qgi : m_items_hash.keys())
	{
		if(QObject *object = dynamic_cast<QObject *>(qgi))
		{
			QPropertyUndoCommand *child_undo = new QPropertyUndoCommand(object, "pos", m_items_hash.value(qgi), qgi->pos(), undo);
			child_undo->enableAnimation();
		}
	}

	m_diagram->undoStack().push(undo);
	
	m_movement_running = false;
}

QString ElementTextsMover::undoText() const
{
	QString undo_text;
	
	if(m_text_count == 1)
		undo_text.append(QObject::tr("Déplacer un texte d'élément"));
	else if(m_text_count > 1)
		undo_text.append(QObject::tr("Déplacer %1 textes d'élément").arg(m_items_hash.size()));
	
	if(m_group_count >= 1)
	{
		if(undo_text.isEmpty())
			undo_text.append(QObject::tr("Déplacer"));
		else
			undo_text.append(QObject::tr(" et"));
		
		if(m_group_count == 1)
			undo_text.append(QObject::tr(" un groupe de texte"));
		else
			undo_text.append(QObject::tr((" %1 groupes de textes")).arg(m_group_count));
	}
	
	return undo_text;
}
