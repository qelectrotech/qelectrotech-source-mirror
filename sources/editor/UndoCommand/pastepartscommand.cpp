/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "pastepartscommand.h"
#include "elementview.h"
#include "partterminal.h"

/**
	@brief PastePartsCommand::PastePartsCommand
	@param view : view where this command work
	@param content_to_paste : content to paste
	@param parent : parent undo command
	@note all terminal stored in content_to_paste get a new uuid
	in the constructor of this class to avoid have several terminal
	of an element with the same uuid.
*/
PastePartsCommand::PastePartsCommand(
		ElementView *view,
		const ElementContent &content_to_paste,
		QUndoCommand *parent) :
	ElementEditionCommand(view ? view -> scene() : nullptr, view, parent)
{
	for (auto qgi : content_to_paste)
	{
		if (qgi->type() == PartTerminal::Type) {
			auto part_terminal = static_cast<PartTerminal*>(qgi);
			part_terminal->setNewUuid();
		}
		m_pasted_content.append(qgi);
	}
	setText(QObject::tr("Coller"));
	m_scene->qgiManager().manage(m_pasted_content);
}

/**
	@brief PastePartsCommand::~PastePartsCommand
*/
PastePartsCommand::~PastePartsCommand()
{
	m_scene->qgiManager().release(m_pasted_content);
}

/**
	@brief PastePartsCommand::undo
*/
void PastePartsCommand::undo()
{
	m_scene->blockSignals(true);
	for (auto qgi : m_pasted_content) {
		m_scene->removeItem(qgi);
	}
	m_scene->blockSignals(false);

	if (m_uses_offset)
	{
		m_view->offset_paste_count_    = m_old_offset_paste_count;
		m_view->start_top_left_corner_ = m_old_start_top_left_corner;
	}

	m_view->adjustSceneRect();
}

/**
	@brief PastePartsCommand::redo
*/
void PastePartsCommand::redo()
{
	if (m_first_redo) {
		m_first_redo = false;
	} else {
		m_scene->blockSignals(true);
		for (auto qgi : m_pasted_content) {
			m_scene->addItem(qgi);
		}
		m_scene->blockSignals(false);

		if (m_uses_offset)
		{
			m_view->offset_paste_count_ = m_new_offset_paste_count;
			m_view->start_top_left_corner_ = m_new_start_top_left_corner;
		}
	}

	m_scene->slot_select(m_pasted_content);
	m_view->adjustSceneRect();
}

/**
	@brief PastePartsCommand::setOffset
	Describe the offset to use with this undo command
	@param old_offset_paste_count
	@param old_start_top_left_corner
	@param new_offset_paste_count
	@param new_start_top_left_corner
*/
void PastePartsCommand::setOffset(int old_offset_paste_count, const QPointF &old_start_top_left_corner, int new_offset_paste_count, const QPointF &new_start_top_left_corner)
{
	m_old_offset_paste_count    = old_offset_paste_count;
	m_old_start_top_left_corner = old_start_top_left_corner;
	m_new_offset_paste_count    = new_offset_paste_count;
	m_new_start_top_left_corner = new_start_top_left_corner;
	m_uses_offset = true;
}
