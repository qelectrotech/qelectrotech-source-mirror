/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "addgraphicsobjectcommand.h"
#include "../qetgraphicsitem/qetgraphicsitem.h"
#include "../qetgraphicsitem/independenttextitem.h"
#include "../qetgraphicsitem/conductor.h"
#include "../diagram.h"

/**
 * @brief AddGraphicsObjectCommand::AddGraphicsObjectCommand
 * Default constructor
 * @param qgo : QGraphicsObject to manage by this undo command
 * @param diagram : Diagram where the graphics object must be added
 * @param pos : position of the qgraphics object in the diagram in scene coordinate
 * @param parent : parent undo command of this class.
 */
AddGraphicsObjectCommand::AddGraphicsObjectCommand(QGraphicsObject *qgo, Diagram *diagram,
												   const QPointF &pos, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_item(qgo),
	m_diagram(diagram),
	m_pos(pos)
{
	setText(QObject::tr("Ajouter ") + itemText());
	m_diagram->qgiManager().manage(m_item);
}

/**
 * @brief AddGraphicsObjectCommand::~AddGraphicsObjectCommand
 */
AddGraphicsObjectCommand::~AddGraphicsObjectCommand() {
	m_diagram->qgiManager().release(m_item);
}

/**
 * @brief AddGraphicsObjectCommand::undo
 * Reimplemented from QUndoCommand
 */
void AddGraphicsObjectCommand::undo()
{
	if (m_item)
	{
		m_diagram->showMe();
		m_diagram->removeItem(m_item);
	}
	QUndoCommand::undo();
}

/**
 * @brief AddGraphicsObjectCommand::redo
 * Reimplemented from QUndoCommand
 */
void AddGraphicsObjectCommand::redo()
{
	if (m_item)
	{
		m_diagram->showMe();
		m_diagram->addItem(m_item);
		m_item->setPos(m_pos);
	}
	QUndoCommand::redo();
}

/**
 * @brief AddGraphicsObjectCommand::itemText
 * @return
 */
QString AddGraphicsObjectCommand::itemText() const
{
	if (auto qgi = dynamic_cast<QetGraphicsItem *>(m_item.data())) {
		return qgi->name();
	} else if (dynamic_cast<IndependentTextItem *>(m_item.data())) {
		return QObject::tr("un champ texte");
	} else if (dynamic_cast<Conductor *>(m_item.data())) {
		return QObject::tr("un conducteur");
	} else {
		return QObject::tr("un element graphique");
	}
}
