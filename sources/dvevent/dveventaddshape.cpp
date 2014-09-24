/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "dveventaddshape.h"
#include "diagramview.h"
#include "qetshapeitem.h"
#include "diagram.h"
#include "diagramcommands.h"
#include <QMouseEvent>

/**
 * @brief DVEventAddShape::DVEventAddShape
 * Default constructor
 * @param dv, the diagram view where operate this event
 * @param shape_type, the shape type to draw
 */
DVEventAddShape::DVEventAddShape(DiagramView *dv, QetShapeItem::ShapeType shape_type) :
	DVEventInterface(dv),
	m_shape_type (shape_type),
	m_shape_item (nullptr)
{}

/**
 * @brief DVEventAddShape::~DVEventAddShape
 */
DVEventAddShape::~DVEventAddShape() {
	if (m_running) {
		m_diagram -> removeItem(m_shape_item);
		delete m_shape_item;
	}
	m_dv -> setContextMenuPolicy(Qt::DefaultContextMenu);
}

/**
 * @brief DVEventAddShape::mousePressEvent
 * Action when mouse is pressed
 * @param event : event of mouse press
 * @return : true if this event is managed, otherwise false
 */
bool DVEventAddShape::mousePressEvent(QMouseEvent *event) {

	if (!m_dv->isInteractive() && m_diagram->isReadOnly()) return false;

	QPointF pos = m_dv->mapToScene(event->pos());

	//@m_running false => shape isn't created yet, we create a new shape
	if (m_running == false && event -> button() == Qt::LeftButton) {
		m_shape_item = new QetShapeItem(pos, pos, m_shape_type);
		m_dv -> setContextMenuPolicy (Qt::NoContextMenu);
		m_diagram -> addItem (m_shape_item);
		m_running = true;
		return true;
	}

	//At this point m_shape_item must be created
	if (!m_shape_item) return false;

	// Next left click finish all shape item except the polyline
	if (m_shape_type != QetShapeItem::Polyline && event->button() == Qt::LeftButton) {
		m_shape_item -> setP2 (pos);
		m_diagram -> undoStack().push (new AddShapeCommand(m_diagram, m_shape_item, pos));
		m_dv -> setContextMenuPolicy(Qt::DefaultContextMenu);
		m_running = false;
		return true;
	}

	// Next left click create new segment for polyline
	if (m_shape_type == QetShapeItem::Polyline && event -> button() == Qt::LeftButton) {
		m_shape_item -> setNextPoint (Diagram::snapToGrid(pos)); //< this point is ok for pos
		m_shape_item -> setNextPoint (Diagram::snapToGrid(pos)); //< Add new point for next segment. the pos of this point
																 //< can be changed by calling QetShapItem::setP2()
		return true;
	}

	// If shape item is polyline and click is right button, the shape item is finish
	// m_running is set to false at the release of right button.
	if (m_shape_type == QetShapeItem::Polyline && event -> button() == Qt::RightButton) {
		m_shape_item -> setP2 (pos);
		m_diagram -> undoStack().push(new AddShapeCommand(m_diagram, m_shape_item, pos));
		return true;
	}

	return false;
}

/**
 * @brief DVEventAddShape::mouseMoveEvent
 * Action when mouse move
 * @param event : event of mouse move
 * @return : true if this event is managed, otherwise false
 */
bool DVEventAddShape::mouseMoveEvent(QMouseEvent *event) {
	if (!m_running) return false;
	if (m_shape_item && event -> buttons() == Qt::NoButton) {
		m_shape_item -> setP2 (m_dv -> mapToScene (event -> pos()));
		return true;
	}
	return false;
}

/**
 * @brief DVEventAddShape::mouseReleaseEvent
 * Action when mouse button is released
 * @param event : event of mouse release
 * @return : true if this event is managed, otherwise false
 */
bool DVEventAddShape::mouseReleaseEvent(QMouseEvent *event) {
	//When the shape is polyline, we set default context menu to diagram view
	//only when the right button is released
	if (m_shape_type == QetShapeItem::Polyline && event -> button() == Qt::RightButton ) {
		m_dv -> setContextMenuPolicy(Qt::DefaultContextMenu);
		m_running = false;
		return true;
	}
	return false;
}
