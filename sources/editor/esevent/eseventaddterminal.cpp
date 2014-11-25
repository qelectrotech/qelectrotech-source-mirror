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
#include <QObject>

#include "partterminal.h"
#include "editorcommands.h"
#include "elementscene.h"
#include "eseventaddterminal.h"

/**
 * @brief ESEventAddTerminal::ESEventAddTerminal
 * @param scene
 */
ESEventAddTerminal::ESEventAddTerminal(ElementScene *scene) :
	ESEventInterface(scene)
{
	m_terminal = new PartTerminal(m_editor, 0, m_scene);
	m_running  = true;
}

/**
 * @brief ESEventAddTerminal::~ESEventAddTerminal
 */
ESEventAddTerminal::~ESEventAddTerminal() {
	delete m_terminal;
}

/**
 * @brief ESEventAddTerminal::mouseMoveEvent
 * @param event
 * @return
 */
bool ESEventAddTerminal::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	QPointF pos = m_scene -> snapToGrid(event -> scenePos());
	updateHelpCross(pos);
	m_terminal -> setPos(pos);
	return true;
}

/**
 * @brief ESEventAddTerminal::mouseReleaseEvent
 * @param event
 * @return
 */
bool ESEventAddTerminal::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	if (event -> button() == Qt::LeftButton) {
		m_scene -> undoStack().push(new AddPartCommand(QObject::tr("Borne"), m_scene, m_terminal));

		//Set new terminal with same rotation
		Qet::Orientation ori = m_terminal -> orientation();
		m_terminal = new PartTerminal(m_editor, 0, m_scene);
		m_terminal -> setOrientation(ori);
		m_terminal -> setPos(m_scene -> snapToGrid(event -> scenePos()));

		return true;
	}
	else if (event -> button() == Qt::RightButton) {
		m_running = false;
		return true;
	}

	return false;
}

/**
 * @brief ESEventAddTerminal::keyPressEvent
 * @param event
 * @return
 */
bool ESEventAddTerminal::keyPressEvent(QKeyEvent *event) {
	if (event -> key() == Qt::Key_Space) {
		switch (m_terminal->orientation()) {
			case Qet::North :
				m_terminal -> setOrientation(Qet::East);
				break;
			case Qet::East :
				m_terminal -> setOrientation(Qet::South);
				break;
			case Qet::South :
				m_terminal -> setOrientation(Qet::West);
				break;
			case Qet::West :
				m_terminal -> setOrientation(Qet::North);
				break;
			default :
				m_terminal -> setOrientation(Qet::North);
				break;
		}
		return true;
	}
	return (ESEventInterface::keyPressEvent(event));
}
