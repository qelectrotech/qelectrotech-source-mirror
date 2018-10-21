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

#include "changetitleblockcommand.h"
#include "diagram.h"

/**
 * @brief ChangeTitleBlockCommand::ChangeTitleBlockCommand
 * @param d
 * @param old_ip
 * @param new_ip
 * @param parent
 */
ChangeTitleBlockCommand::ChangeTitleBlockCommand(
	Diagram *d,
	const TitleBlockProperties &old_ip,
	const TitleBlockProperties &new_ip,
	QUndoCommand *parent
) :
	QUndoCommand(QObject::tr("modifier le cartouche", "undo caption"), parent),
	diagram(d),
	old_titleblock(old_ip),
	new_titleblock(new_ip)
{}

ChangeTitleBlockCommand::~ChangeTitleBlockCommand() {}

void ChangeTitleBlockCommand::undo()
{
	diagram -> showMe();
	diagram -> border_and_titleblock.importTitleBlock(old_titleblock);
	diagram -> invalidate(diagram -> border_and_titleblock.borderAndTitleBlockRect());
}

void ChangeTitleBlockCommand::redo()
{
	diagram -> showMe();
	diagram -> border_and_titleblock.importTitleBlock(new_titleblock);
	diagram -> invalidate(diagram -> border_and_titleblock.borderAndTitleBlockRect());
}
