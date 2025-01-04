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
#ifndef CHANGEELEMENTDATACOMMAND_H
#define CHANGEELEMENTDATACOMMAND_H

#include <../properties/elementdata.h>
#include <QUndoCommand>

class Element;

class ChangeElementDataCommand : public QUndoCommand
{
	public:
		ChangeElementDataCommand(Element *element, ElementData new_data, QUndoCommand *parent = nullptr);
		void undo() override;
		void redo() override;

	private:
		QPointer<Element> m_element;
		ElementData m_old_data, m_new_data;
};

#endif // CHANGEELEMENTDATACOMMAND_H
