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

#include "eseventaddtext.h"
#include "parttext.h"
#include "editorcommands.h"
#include "elementscene.h"

/**
 * @brief ESEventAddText::ESEventAddText
 * @param scene
 */
ESEventAddText::ESEventAddText(ElementScene *scene) :
	ESEventInterface(scene)
{
	m_text = new PartText(m_editor);
	m_scene -> addItem(m_text);
	m_running = true;
}

/**
 * @brief ESEventAddText::~ESEventAddText
 */
ESEventAddText::~ESEventAddText() {
		delete m_text;
}

/**
 * @brief ESEventAddText::mouseMoveEvent
 * @param event
 * @return
 */
bool ESEventAddText::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	QPointF pos = m_scene -> snapToGrid(event -> scenePos());
	updateHelpCross(pos);
	m_text->setPos(pos);
	return true;
}

/**
 * @brief ESEventAddText::mouseReleaseEvent
 * @param event
 * @return
 */
bool ESEventAddText::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	if (event->button() == Qt::LeftButton) {
		m_scene -> undoStack().push(new AddPartCommand(QObject::tr("Texte"), m_scene, m_text));

		//Set new text
		m_text = new PartText(m_editor);
		m_scene -> addItem(m_text);
		m_text -> setPos(m_scene -> snapToGrid(event -> scenePos()));

		return true;
	}
	else if (event->button() == Qt::RightButton) {
		m_running = false;
		return true;
	}

	return false;
}
