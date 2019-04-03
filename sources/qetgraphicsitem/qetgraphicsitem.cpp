/*
	Copyright 2006-2019 The QElectroTech Team
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
#include "qetgraphicsitem.h"
#include "diagram.h"

/**
 * @brief QetGraphicsItem::QetGraphicsItem Default constructor
 * @param uuid, uuid of the item
 * @param diagram, diagram aka QGraphicsScene of the item
 * @param parent, Parent Item
 */
QetGraphicsItem::QetGraphicsItem(QGraphicsItem *parent):
	QGraphicsObject(parent),
	is_movable_(true),
	m_first_move(true),
	snap_to_grid_(true)
{}

QetGraphicsItem::~QetGraphicsItem()
{}

/**
 * @brief QetGraphicsItem::diagram
 *return the diagram of this item
 */
Diagram* QetGraphicsItem::diagram() const{
	return(qobject_cast<Diagram *>(scene()));
}

/**
 * @brief QetGraphicsItem::setPos
 *set the position of the item to p
 * @param p the new position of item
 */
void QetGraphicsItem::setPos(const QPointF &p) {
	QPointF pp = Diagram::snapToGrid(p);
	if (pp == pos() || !is_movable_)
		return;
	QGraphicsItem::setPos(pp);
}

/**
 * @brief QetGraphicsItem::setPos
 *set the position of the item
 * @param x new abscisse of item
 * @param y new ordonne of item
 */
void QetGraphicsItem::setPos(qreal x, qreal y) {
	setPos(QPointF(x, y));
}

/**
 * @brief QetGraphicsItem::state
 * @return the current state of this item 
 */
QET::GraphicsItemState QetGraphicsItem::state() const {
	return m_state;
}

/**
 * @brief QetGraphicsItem::mousePressEvent
 *handle the mouse click
 * @param event
 */
void QetGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_first_move = true;
		if (event->modifiers() & Qt::ControlModifier) {
			setSelected(!isSelected());
		}
	}

	QGraphicsItem::mousePressEvent(event);
}

/**
 * @brief QetGraphicsItem::mouseDoubleClickEvent
 *handle the mouse double click
 * @param event
 */
void QetGraphicsItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	editProperty();
	event->accept();
}

/**
 * @brief QetGraphicsItem::mouseMoveEvent
 *handle mouse movement
 * @param event
 */
void QetGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (isSelected() && event->buttons() & Qt::LeftButton)
	{
			//Item is moving
		if(diagram() && m_first_move) {
				//It's the first movement, we signal it to parent diagram
			diagram()->elementsMover().beginMovement(diagram(), this);
		}

			//we apply the mouse movement
		QPointF old_pos = pos();
		if (m_first_move) {
			m_mouse_to_origin_movement = old_pos - event -> buttonDownScenePos(Qt::LeftButton);
		}
		QPointF expected_pos = event->scenePos() + m_mouse_to_origin_movement;
		setPos(expected_pos); // setPos() will snap the expected position to the grid

			//we calcul the real movement apply by setPos()
		QPointF effective_movement = pos() - old_pos;
		if (diagram()) {
				//we signal the real movement apply to diagram,
				//who he apply to other selected item
			diagram()->elementsMover().continueMovement(effective_movement);
		}
		event->accept();
	}
	else {
		event->ignore();
	}

	if (m_first_move) {
		m_first_move = false;
	}
}

/**
 * @brief QetGraphicsItem::mouseReleaseEvent
 *handle mouse release click
 * @param e
 */
void QetGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (diagram()) {
		diagram()->elementsMover().endMovement();
		event->accept();
	}
}
