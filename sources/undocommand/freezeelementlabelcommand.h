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
#ifndef FREEZEELEMENTLABELCOMMAND_H
#define FREEZEELEMENTLABELCOMMAND_H

#include <QPointer>
#include <QUndoCommand>

class Element;

/**
	@brief The FreezeElementLabelCommand class
	Freezes or unfreezes the label of an element: a frozen label is not
	touched by the numbering of elements, and its number is not given to
	another element.
*/
class FreezeElementLabelCommand : public QUndoCommand
{
	public:
		FreezeElementLabelCommand(Element *element,
								  bool old_frozen,
								  bool new_frozen,
								  QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;

	private:
		QPointer<Element> m_element;
		bool m_old_frozen;
		bool m_new_frozen;
};

#endif // FREEZEELEMENTLABELCOMMAND_H
