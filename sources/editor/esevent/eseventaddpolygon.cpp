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
#include <QObject>

#include "eseventaddpolygon.h"
#include "elementscene.h"
#include "partpolygon.h"
#include "editorcommands.h"

/**
 * @brief ESEventAddPolygon::ESEventAddPolygon
 * @param scene
 */
ESEventAddPolygon::ESEventAddPolygon(ElementScene *scene) :
	ESEventInterface(scene),
	m_polygon(nullptr)
{}

/**
 * @brief ESEventAddPolygon::~ESEventAddPolygon
 */
ESEventAddPolygon::~ESEventAddPolygon() {
	if (m_running || m_abort)
		delete m_polygon;
}

/**
 * @brief ESEventAddPolygon::mousePressEvent
 * @param event
 * @return
 */
bool ESEventAddPolygon::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	if (event -> button() == Qt::LeftButton) {
		if(!m_running) m_running = true;
		QPointF pos = m_scene->snapToGrid(event -> scenePos());

		//create new polygon
		if (!m_polygon) {
			m_polygon = new PartPolygon(m_editor);
			m_scene -> addItem(m_polygon);
			m_polygon -> addPoint(pos);
		}

		m_polygon -> addPoint(pos);
		return true;
	}
	return false;
}

/**
 * @brief ESEventAddPolygon::mouseMoveEvent
 * @param event
 * @return
 */
bool ESEventAddPolygon::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	updateHelpCross(event -> scenePos());
	if (!m_polygon) return false;

	m_polygon -> setLastPoint(m_scene -> snapToGrid(event -> scenePos()));
	return true;
}

/**
 * @brief ESEventAddPolygon::mouseReleaseEvent
 * @param event
 * @return
 */
bool ESEventAddPolygon::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	if (event -> button() == Qt::RightButton) {
		if (m_polygon) {
			m_polygon -> removeLastPoint();

			if (m_polygon -> polygon().size() > 1)
				{ m_polygon -> setLastPoint(m_scene -> snapToGrid(event -> scenePos())); }
			else
				{ delete m_polygon; m_polygon = nullptr; }
		}
		else
			{ m_running = false; }

		return true;
	}
	return false;
}

/**
 * @brief ESEventAddPolygon::mouseDoubleClickEvent
 * @param event
 * @return
 */
bool ESEventAddPolygon::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if (event -> button() == Qt::LeftButton)
	{
		if (m_polygon)
		{
			m_polygon->removeLastPoint();
			m_scene   -> undoStack().push(new AddPartCommand(QObject::tr("Polygone"), m_scene, m_polygon));

				//Set m_polygon to nullptr for create new polygon at next mouse press
			m_polygon = nullptr;
			return true;
		}
	}
	return false;
}
