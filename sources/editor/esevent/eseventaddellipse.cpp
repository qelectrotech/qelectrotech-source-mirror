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

#include "eseventaddellipse.h"
#include "partellipse.h"
#include "editorcommands.h"
#include "elementscene.h"

/**
 * @brief ESEventAddEllipse::ESEventAddEllipse
 * @param scene
 */
ESEventAddEllipse::ESEventAddEllipse(ElementScene *scene) :
	ESEventInterface(scene),
	m_ellipse(nullptr)
{}

/**
 * @brief ESEventAddEllipse::~ESEventAddEllipse
 */
ESEventAddEllipse::~ESEventAddEllipse() {
	if (m_running || m_abort){
		delete m_ellipse;
	}
}

/**
 * @brief ESEventAddEllipse::mousePressEvent
 * @param event
 * @return
 */
bool ESEventAddEllipse::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	if (event -> button() == Qt::LeftButton) {
		if(!m_running) m_running = true;
		QPointF pos = m_scene->snapToGrid(event -> scenePos());

			//create new ellpise
		if (!m_ellipse) {
			m_ellipse = new PartEllipse(m_editor);
			m_scene -> addItem(m_ellipse);
			m_ellipse -> setPos(pos);
			m_origin = m_new_pos = pos;
			return true;
		}

			//Add ellipse to scene
		m_ellipse -> setRect(m_ellipse -> rect().normalized());
		m_scene -> undoStack().push(new AddPartCommand(QObject::tr("Ellipse"), m_scene, m_ellipse));

			//Set m_ellipse to nullptr for create new ellipse at next mouse press
		m_ellipse = nullptr;

		return true;
	}
	return false;
}

/**
 * @brief ESEventAddRect::mouseMoveEvent
 * @param event
 * @return
 */
bool ESEventAddEllipse::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	updateHelpCross(event -> scenePos());
	if (!m_ellipse) return false;

	QPointF pos = m_scene -> snapToGrid(event -> scenePos());
	if (pos == m_new_pos) return true;
	m_new_pos = pos;

	qreal width  = (m_new_pos.x() - m_origin.x())*2;
	qreal height = (m_new_pos.y() - m_origin.y())*2;
		//calculates the position of the rectangle so that its center is at position (0,0) of m_ellipse
	QPointF center(-width/2, -height/2);

	m_ellipse -> setRect(QRectF(center, QSizeF(width, height)));
	return true;
}

/**
 * @brief ESEventAddEllipse::mouseReleaseEvent
 * @param event
 * @return
 */
bool ESEventAddEllipse::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	if (event -> button() == Qt::RightButton) {
		if (m_ellipse) {delete m_ellipse; m_ellipse = nullptr;}
		else        {m_running = false;}
		return true;
	}
	return false;
}
