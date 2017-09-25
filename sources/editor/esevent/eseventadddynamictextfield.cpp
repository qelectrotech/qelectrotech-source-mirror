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
#include "eseventadddynamictextfield.h"
#include "elementscene.h"
#include "editorcommands.h"
#include "partdynamictextfield.h"

#include <QUndoStack>

/**
 * @brief ESEventAddDynamicTextField::ESEventAddDynamicTextField
 * @param scene
 */
ESEventAddDynamicTextField::ESEventAddDynamicTextField(ElementScene *scene) :
	ESEventInterface(scene)
{
	m_text = new PartDynamicTextField(m_editor);
	m_scene->addItem(m_text);
	m_running = true;
}

/**
 * @brief ESEventAddDynamicTextField::~ESEventAddDynamicTextField
 */
ESEventAddDynamicTextField::~ESEventAddDynamicTextField() {
	delete m_text;
}

/**
 * @brief ESEventAddDynamicTextField::mouseMoveEvent
 * @param event
 * @return 
 */
bool ESEventAddDynamicTextField::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	QPointF pos = m_scene->snapToGrid(event->scenePos());
	updateHelpCross(pos);
	m_text->setPos(pos);
	return true;
}

/**
 * @brief ESEventAddDynamicTextField::mouseReleaseEvent
 * @param event
 * @return 
 */
bool ESEventAddDynamicTextField::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_scene->undoStack().push(new AddPartCommand(QObject::tr("Champ texte dynamique"), m_scene, m_text));

			//Set new text
		m_text = new PartDynamicTextField(m_editor);
		m_scene->addItem(m_text);
		m_text->setPos(m_scene->snapToGrid(event->scenePos()));

		return true;
	}
	else if (event->button() == Qt::RightButton)
	{
		m_running = false;
		return true;
	}

	return false;
}
