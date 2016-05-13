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

#include "parttextfield.h"
#include "editorcommands.h"
#include "elementscene.h"
#include "eseventaddtextfield.h"

/**
 * @brief ESEventAddTextField::ESEventAddTextField
 * @param scene
 */
ESEventAddTextField::ESEventAddTextField(ElementScene *scene) :
	ESEventInterface(scene)
{
	m_text = new PartTextField(m_editor);
	m_scene -> addItem(m_text);
	m_running = true;
}

/**
 * @brief ESEventAddTextField::~ESEventAddTextField
 */
ESEventAddTextField::~ESEventAddTextField() {
		delete m_text;
}

/**
 * @brief ESEventAddTextField::ESEventAddTextField
 * @param event
 * @return
 */
bool ESEventAddTextField::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	QPointF pos = m_scene -> snapToGrid(event -> scenePos());
	updateHelpCross(pos);
	m_text->setPos(pos);
	return true;
}

/**
 * @brief ESEventAddTextField::ESEventAddTextField
 * @param event
 * @return
 */
bool ESEventAddTextField::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	if (event->button() == Qt::LeftButton) {
		m_scene -> undoStack().push(new AddPartCommand(QObject::tr("Champ texte"), m_scene, m_text));

		//Set new text
		m_text = new PartTextField(m_editor);
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
