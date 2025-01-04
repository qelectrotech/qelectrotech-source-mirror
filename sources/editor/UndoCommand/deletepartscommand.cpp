/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "deletepartscommand.h"
#include "../elementscene.h"

/**
 * @brief DeletePartsCommand::DeletePartsCommand
 * @param scene
 * @param parts
 * @param parent
 */
DeletePartsCommand::DeletePartsCommand(QPointer<ElementScene> scene,
									   const QVector<QGraphicsItem *> &parts,
									   QUndoCommand *parent) :
	QUndoCommand{parent},
	m_scene{scene},
	m_parts{parts}
{
	setText(QObject::tr("suppression", "undo caption"));
	m_scene->qgiManager().manage(parts);
}

/**
 * @brief DeletePartsCommand::~DeletePartsCommand
 */
DeletePartsCommand::~DeletePartsCommand()
{
	if (m_scene) {
		m_scene->qgiManager().release(m_parts);
	}
}

/**
 * @brief DeletePartsCommand::undo
 */
void DeletePartsCommand::undo()
{
	if (m_scene) {
		m_scene->addItems(m_parts);
	}
}

/**
 * @brief DeletePartsCommand::redo
 */
void DeletePartsCommand::redo()
{
	if (m_scene) {
		m_scene->removeItems(m_parts);
	}
}
