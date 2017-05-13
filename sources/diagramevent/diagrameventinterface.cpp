/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "diagrameventinterface.h"
#include "diagram.h"
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>


DiagramEventInterface::DiagramEventInterface(Diagram *diagram) :
	m_diagram(diagram),
	m_running(false),
	m_abort(false)
{
	m_diagram -> clearSelection();
}

DiagramEventInterface::~DiagramEventInterface() {};

bool DiagramEventInterface::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	Q_UNUSED (event);
	return false;
}

bool DiagramEventInterface::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	Q_UNUSED (event);
	return false;
}

bool DiagramEventInterface::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	Q_UNUSED (event);
	return false;
}

bool DiagramEventInterface::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	Q_UNUSED (event);
	return false;
}

bool DiagramEventInterface::wheelEvent(QGraphicsSceneWheelEvent *event) {
	Q_UNUSED (event);
	return false;
}

/**
 * @brief DiagramEventInterface::keyPressEvent
 * By default, press escape key abort the curent action
 * @param event
 * @return
 */
bool DiagramEventInterface::keyPressEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key_Escape) {
		m_running = false;
		m_abort = true;
		emit finish();
		return true;
	}
	return false;
}

bool DiagramEventInterface::keyReleaseEvent(QKeyEvent *event) {
	Q_UNUSED (event);
	return false;
}

bool DiagramEventInterface::isRunning() const {
	return m_running;
}

void DiagramEventInterface::init()
{}
