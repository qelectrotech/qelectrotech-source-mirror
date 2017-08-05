/*
	Copyright 2006-2017 The QElectroTech Team
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
#ifndef DELETEQGRAPHICSITEMCOMMAND_H
#define DELETEQGRAPHICSITEMCOMMAND_H

#include <QUndoCommand>
#include "diagramcontent.h"

class Diagram;

class DeleteQGraphicsItemCommand : public QUndoCommand
{
	public:
		DeleteQGraphicsItemCommand(Diagram *diagram, const DiagramContent &content, QUndoCommand * parent = nullptr);
		~DeleteQGraphicsItemCommand() override;
	private:
		DeleteQGraphicsItemCommand(const DeleteQGraphicsItemCommand &);

	public:
		void undo() override;
		void redo() override;
		
		// attributes
	private:
		DiagramContent m_removed_contents;
		Diagram *m_diagram;
		QHash <Element *, QList<Element *> > m_link_hash; /// keep linked element for each removed element linked to other element.
		QHash <DynamicElementTextItem *, Element *> m_elmt_text_hash; /// Keep the parent element of each deleted dynamic element text item
};

#endif // DELETEQGRAPHICSITEMCOMMAND_H
