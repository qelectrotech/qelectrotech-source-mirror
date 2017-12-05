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
#ifndef ROTATESELECTIONCOMMAND_H
#define ROTATESELECTIONCOMMAND_H

#include <QUndoCommand>
#include <QPointer>

class Diagram;
class Element;
class QGraphicsObject;
class ElementTextItemGroup;
class DiagramTextItem;
class DiagramImageItem;

/**
 * @brief The RotateSelectionCommand class
 * Rotate the selected items in the given diagram
 */
class RotateSelectionCommand : public QUndoCommand
{
	public:
		RotateSelectionCommand(Diagram *diagram, qreal angle=90, QUndoCommand *parent=nullptr);
		void undo() override;
		void redo() override;
		
		bool isValid();
	
	private:
		Diagram *m_diagram =nullptr;
		qreal m_angle;
		
		QList<QPointer<Element>> m_element;
		QList<QPointer<DiagramImageItem>> m_image;
		QList<QPointer<ElementTextItemGroup>> m_group;
		QList<QPointer<DiagramTextItem>> m_text;
		QHash<DiagramTextItem *, bool> m_rotate_by_user;
		
};

#endif // ROTATESELECTIONCOMMAND_H
