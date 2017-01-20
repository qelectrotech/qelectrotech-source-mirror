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
#include "eseventinterface.h"
#include "elementscene.h"
#include "qetelementeditor.h"
#include <QGraphicsSceneMouseEvent>

ESEventInterface::ESEventInterface(ElementScene *scene) :
	m_scene      (scene),
	m_editor     (scene->editor()),
	m_help_horiz (nullptr),
	m_help_verti (nullptr),
	m_running    (false),
	m_abort      (false)
{
	init();
}

/**
 * @brief ESEventInterface::init
 * Init this event interface
 */
void ESEventInterface::init()
{
	m_scene->setBehavior(ElementScene::Behavior::AddPart);
	m_editor->slot_setNoDragToView();
}

/**
 * @brief ESEventInterface::~ESEventInterface
 * Destructor
 */
ESEventInterface::~ESEventInterface()
{
	delete m_help_horiz;
	delete m_help_verti;

	m_scene->setBehavior(ElementScene::Behavior::Normal);
	m_editor->slot_setRubberBandToView();
}

bool ESEventInterface::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	Q_UNUSED (event);
	return false;
}

bool ESEventInterface::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	Q_UNUSED (event);
	return false;
}

bool ESEventInterface::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	Q_UNUSED (event);
	return false;
}

bool ESEventInterface::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	Q_UNUSED (event);
	return false;
}

bool ESEventInterface::wheelEvent(QGraphicsSceneWheelEvent *event) {
	Q_UNUSED (event);
	return false;
}

/**
 * @brief ESEventInterface::keyPressEvent
 * By default, press escape key abort the curent action
 * @param event
 * @return
 */
bool ESEventInterface::keyPressEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key_Escape) {
		m_running = false;
		m_abort = true;
		return true;
	}
	return false;
}

bool ESEventInterface::KeyReleaseEvent(QKeyEvent *event) {
	Q_UNUSED (event);
	return false;
}

bool ESEventInterface::isRunning() const {
	return m_running;
}

bool ESEventInterface::isFinish() const {
	return !m_running;
}

void ESEventInterface::updateHelpCross(const QPointF &p) {
	//If line isn't created yet, we create it.
	if (!m_help_horiz || !m_help_verti) {
		QPen pen;
		pen.setWidthF(0.4);
		pen.setCosmetic(true);
		pen.setColor(Qt::darkGray);
		if (!m_help_horiz) {
			m_help_horiz = new QGraphicsLineItem(-10000, 0, 10000, 0);
			m_help_horiz -> setPen(pen);
			m_scene -> addItem(m_help_horiz);
		}
		if (!m_help_verti) {
			m_help_verti = new QGraphicsLineItem(0, -10000, 0, 10000);
			m_help_verti -> setPen(pen);
			m_scene -> addItem(m_help_verti);
		}
	}

	//Update the position of the cross
	QPointF point = m_scene -> snapToGrid(p);

	m_help_horiz -> setY(point.y());
	m_help_verti -> setX(point.x());

}
