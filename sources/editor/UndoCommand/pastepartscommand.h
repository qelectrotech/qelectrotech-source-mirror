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
#ifndef PASTEPARTSCOMMAND_H
#define PASTEPARTSCOMMAND_H

#include "editorcommands.h"
#include "elementcontent.h"

class ElementView;

/**
	@brief The PastePartsCommand class
	Undo command for paste element primitive in an element editor
*/
class PastePartsCommand : public ElementEditionCommand
{
	public:
		PastePartsCommand(ElementView *view, const ElementContent &content_to_paste, QUndoCommand *parent = nullptr);
		~PastePartsCommand() override;
	private:
		PastePartsCommand(const PastePartsCommand &);

	public:
		void undo() override;
		void redo() override;
		virtual void setOffset(int old_offset_paste_count, const QPointF &old_start_top_left_corner, int new_offset_paste_count, const QPointF &new_start_top_left_corner);

	private:
		ElementContent m_pasted_content;

		int m_old_offset_paste_count,
			m_new_offset_paste_count;

		QPointF m_old_start_top_left_corner,
				m_new_start_top_left_corner;

		bool m_uses_offset = false,
			 m_first_redo = true;
};

#endif // PASTEPARTSCOMMAND_H
