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
#include "elementtextsmover.h"

#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "diagram.h"
#include "qetapp.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/elementtextitemgroup.h"
#include "qetdiagrameditor.h"
#include "textgrid.h"

#include <QObject>
#include <QSettings>

/**
	@brief ElementTextsMover::ElementTextsMover
*/
ElementTextsMover::ElementTextsMover()
{}

/**
	@brief ElementTextsMover::isReady
	@return true if this ElementTextsMover is ready to process a new movement.
	False if this ElementTextsMover is actually process a movement
*/
bool ElementTextsMover::isReady() const
{
	return(!m_movement_running);
}

/**
	@brief ElementTextsMover::beginMovement
	Begin a movement
	@param diagram : diagram where the movement is apply
	@param driver_item : item moved by the mouse
	@return : the number of moved text (driver_item include), or -1 if this ElementTextsMover can't begin a movement
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

	m_status_bar.clear();
	if (!diagram->views().isEmpty())
		if (const auto qde = QETApp::diagramEditorAncestorOf(diagram->views().at(0)))
			m_status_bar = qde->statusBar();
	if (m_status_bar)
	{
		const qreal divisor = QSettings().value(TextGrid::settings_key, 1).toReal();
		m_status_bar->showMessage(divisor > 0
			? QObject::tr("Grille des textes %1. Relâcher Maj et maintenir Ctrl pour placer librement.")
				  .arg(TextGrid::ratioLabel(divisor))
			: QObject::tr("Grille des textes désactivée."));
	}
	
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
		event->modifiers() == Qt::ControlModifier ? qgi->setPos(new_pos) : qgi->setPos(Diagram::snapToTextGrid(new_pos));
	}
}

/**
	@brief ElementTextsMover::endMovement
	Finish the movement by pushing an undo command to the parent diagram of text item
*/
void ElementTextsMover::endMovement()
{	
		//No movement or no items to move
	if (m_status_bar)
		m_status_bar->clearMessage();

	if(!m_movement_running || m_items_hash.isEmpty())
		return;
			
		//Movement is null
	QGraphicsItem *qgi = m_items_hash.keys().first();
	if(qgi->pos() == m_items_hash.value(qgi))
	{
		m_movement_running = false;
		return;
	}
					 
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
	QStringList parts;
	if (m_text_count)
		parts << QObject::tr("%n texte(s) d'élément", "", m_text_count);
	if (m_group_count)
		parts << QObject::tr("%n groupe(s) de textes", "", m_group_count);

	if (parts.isEmpty())
		return QString(); // should never occur

	return QObject::tr("Déplacer %1").arg(QLocale(QETApp::interfaceLanguage()).createSeparatedList(parts));
}
