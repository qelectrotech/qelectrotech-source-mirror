/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "diagrameventaddshape.h"
#include "diagram.h"
#include "diagramcommands.h"

/**
 * @brief DiagramEventAddShape::DiagramEventAddShape
 * Default constructor
 * @param diagram : the diagram where this event must operate
 * @param shape_type : the type of shape to draw
 */
DiagramEventAddShape::DiagramEventAddShape(Diagram *diagram, QetShapeItem::ShapeType shape_type) :
	DiagramEventInterface(diagram),
	m_shape_type (shape_type),
	m_shape_item (nullptr),
	m_help_horiz (nullptr),
	m_help_verti (nullptr)
{
	m_running = true;
	init();
}

/**
 * @brief DiagramEventAddShape::~DiagramEventAddShape
 */
DiagramEventAddShape::~DiagramEventAddShape()
{
	if ((m_running || m_abort) && m_shape_item)
	{
		m_diagram->removeItem(m_shape_item);
		delete m_shape_item;
	}
	delete m_help_horiz;
	delete m_help_verti;

	foreach (QGraphicsView *v, m_diagram->views())
		v->setContextMenuPolicy(Qt::DefaultContextMenu);
}

/**
 * @brief DiagramEventAddShape::mousePressEvent
 * Action when mouse is pressed
 * @param event : event of mouse press
 * @return : true if this event is managed, otherwise false
 */
bool DiagramEventAddShape::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (Q_UNLIKELY(m_diagram->isReadOnly())) return false;

	QPointF pos = event->scenePos();
	if (event->modifiers() != Qt::ControlModifier)
		pos = Diagram::snapToGrid(pos);

		//Action for left mouse click
	if (event->button() == Qt::LeftButton)
	{
			//Create shape item
		if (!m_shape_item)
		{
			m_shape_item = new QetShapeItem(pos, pos, m_shape_type);
			m_diagram->addItem (m_shape_item);
			return true;
		}

			//If current item isn't a polyline, add it with an undo command
		if (m_shape_type != QetShapeItem::Polygon)
		{
			m_shape_item->setP2 (pos);
			m_diagram->undoStack().push (new AddItemCommand<QetShapeItem *> (m_shape_item, m_diagram));
			m_shape_item = nullptr; //< set to nullptr for create new shape at next left clic
		}
			//Else add a new point to polyline
		else
		{
			m_shape_item->setNextPoint (pos);
		}

		return true;
	}

	if (event->button() == Qt::RightButton)
		return true;

	return false;

}

/**
 * @brief DiagramEventAddShape::mouseMoveEvent
 * Action when mouse move
 * @param event : event of mouse move
 * @return : true if this event is managed, otherwise false
 */
bool DiagramEventAddShape::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	updateHelpCross(event->scenePos());

	if (m_shape_item && event->buttons() == Qt::NoButton)
	{
		QPointF pos = event->scenePos();
		if (event->modifiers() != Qt::ControlModifier)
			pos = Diagram::snapToGrid(pos);

		m_shape_item->setP2 (pos);
		return true;
	}

	return false;
}

/**
 * @brief DiagramEventAddShape::mouseReleaseEvent
 * Action when mouse button is released
 * @param event : event of mouse release
 * @return : true if this event is managed, otherwise false
 */
bool DiagramEventAddShape::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::RightButton)
	{
			//If shape is created, we manage right click
		if (m_shape_item)
		{
				//Shape is a polyline and have three points or more we just remove the last point
			if (m_shape_type == QetShapeItem::Polygon && (m_shape_item->pointsCount() >= 3) )
			{
				m_shape_item->removePoints();

				QPointF pos = event->scenePos();
				if (event->modifiers() != Qt::ControlModifier)
					pos = Diagram::snapToGrid(pos);

				m_shape_item->setP2(pos); //Set the new last point under the cursor
				return true;
			}

				//For other case, we remove item from scene
			m_diagram->removeItem(m_shape_item);
			delete m_shape_item;
			m_shape_item = nullptr;
			return true;
		}

			//Else (no shape), we set to false the running status
			//for indicate to the owner of this event that everything is done
		m_running = false;
		emit finish();
		return true;
	}

	return false;
}

/**
 * @brief DiagramEventAddShape::mouseDoubleClickEvent
 * Action when mouse button is double clicked
 * @param event : event of mouse double click
 * @return : true if this event is managed, otherwise false
 */
bool DiagramEventAddShape::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
		//If current item is a polyline, add it with an undo command
	if (m_shape_item && m_shape_type == QetShapeItem::Polygon && event->button() == Qt::LeftButton)
	{
			//<double clic is used to finish polyline, but they also add two points at the same pos
			//<(double clic is a double press event), so we remove the last point of polyline
		m_shape_item->removePoints();
		m_diagram->undoStack().push (new AddItemCommand<QetShapeItem *> (m_shape_item, m_diagram));
		m_shape_item = nullptr; //< set to nullptr for create new shape at next left clic
		return true;
	}

	return false;
}

void DiagramEventAddShape::init()
{
	foreach (QGraphicsView *v, m_diagram->views())
		v->setContextMenuPolicy(Qt::NoContextMenu);
}

/**
 * @brief DiagramEventAddShape::updateHelpCross
 * Create and update the position of the cross to help user for draw new shape
 * @param p : the center of the cross
 */
void DiagramEventAddShape::updateHelpCross(const QPointF &p)
{
		//If line isn't created yet, we create it.
	if (!m_help_horiz || !m_help_verti)
	{
		QPen pen;
		pen.setWidthF(0.4);
		pen.setCosmetic(true);
		pen.setColor(Diagram::background_color == Qt::darkGray ? Qt::lightGray : Qt::darkGray);

		QRectF rect = m_diagram->border_and_titleblock.insideBorderRect();

		if (!m_help_horiz)
		{
			m_help_horiz = new QGraphicsLineItem(rect.topLeft().x(), 0, rect.topRight().x(), 0);
			m_help_horiz->setPen(pen);
			m_diagram->addItem(m_help_horiz);
		}

		if (!m_help_verti)
		{
			m_help_verti = new QGraphicsLineItem(0, rect.topLeft().y(), 0, rect.bottomLeft().y());
			m_help_verti->setPen(pen);
			m_diagram->addItem(m_help_verti);
		}
	}

		//Update the position of the cross
	QPointF point = Diagram::snapToGrid(p);

	m_help_horiz->setY(point.y());
	m_help_verti->setX(point.x());
}
