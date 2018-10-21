/*
	Copyright 2006-2018 The QElectroTech Team
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
#ifndef CHANGETITLEBLOCKCOMMAND_H
#define CHANGETITLEBLOCKCOMMAND_H

#include <QUndoCommand>

#include "titleblockproperties.h"

class Diagram;
/**
 * @brief The ChangeTitleBlockCommand class
 * This command changes the title block properties for a particular diagram.
 */
class ChangeTitleBlockCommand : public QUndoCommand
{
	public:
		ChangeTitleBlockCommand(Diagram *, const TitleBlockProperties &, const TitleBlockProperties &, QUndoCommand * = nullptr);
		~ChangeTitleBlockCommand() override;
	private:
		ChangeTitleBlockCommand(const ChangeTitleBlockCommand &);
	
	public:
		void undo() override;
		void redo() override;
	
	private:
		Diagram *diagram;
		TitleBlockProperties old_titleblock;
		TitleBlockProperties new_titleblock;
};

#endif // CHANGETITLEBLOCKCOMMAND_H
