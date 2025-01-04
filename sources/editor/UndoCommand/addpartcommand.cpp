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
#include "addpartcommand.h"
#include "../elementscene.h"

/**
 * @brief AddPartCommand::AddPartCommand
 * @param text : text of the undo command
 * @param scene : scene where @a part must be added
 * @param part : part to add
 * @param parent : parent undo command
 */
AddPartCommand::AddPartCommand(const QString &text,
							   QPointer<ElementScene> scene,
							   QGraphicsItem *part,
							   QUndoCommand *parent) :
	QUndoCommand{parent},
	m_scene{scene},
	m_part{part}
{
	setText(text);
	m_scene->qgiManager().manage(part);
}

/**
 * @brief AddPartCommand::~AddPartCommand
 */
AddPartCommand::~AddPartCommand()
{
	if (m_scene) {
		m_scene->qgiManager().release(m_part.first());
	}
}

/**
 * @brief AddPartCommand::undo
 */
void AddPartCommand::undo()
{
	if (m_scene) {
		m_scene->removeItems(m_part);
	}
}

/**
 * @brief AddPartCommand::redo
 */
void AddPartCommand::redo()
{
	if (!m_scene) {
		return;
	}

	if (m_first_redo)
	{
		auto part_{m_part.first()};
		if (!part_->zValue())
		{
			// the added part has no specific zValue already defined, we put it
			// above existing items (but still under terminals)
			const auto existing_items = m_scene->zItems(ElementScene::SortByZValue | ElementScene::SelectedOrNot);
			const auto z_{existing_items.count() ? existing_items.last()->zValue() + 1 : 1};
			part_->setZValue(z_);
		}
		m_scene->clearSelection();
		m_first_redo = false;
	}
	m_scene->addItems(m_part);
}
