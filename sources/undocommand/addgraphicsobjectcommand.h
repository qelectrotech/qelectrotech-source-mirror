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
#ifndef ADDGRAPHICSOBJECTCOMMAND_H
#define ADDGRAPHICSOBJECTCOMMAND_H

#include <QUndoCommand>
#include <QPointF>
#include <QPointer>

class QGraphicsObject;
class Diagram;

/**
 * @brief The AddGraphicsObjectCommand class
 * Undo command to used to add item to a diagram.
 */
class AddGraphicsObjectCommand : public QUndoCommand
{
   public:
		AddGraphicsObjectCommand(QGraphicsObject *qgo, Diagram *diagram,
								 const QPointF &pos = QPointF(),
								 QUndoCommand *parent = nullptr);
		~AddGraphicsObjectCommand() override;

		void undo() override;
		void redo() override;

	private:
		QString itemText() const;

	private:
		QPointer<QGraphicsObject> m_item;
		Diagram *m_diagram = nullptr;
		QPointF m_pos;
};

#endif // ADDGRAPHICSOBJECTCOMMAND_H
