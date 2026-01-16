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
#ifndef MOVEGRAPHICSITEMCOMMAND_H
#define MOVEGRAPHICSITEMCOMMAND_H

#include <QUndoCommand>
#include <QParallelAnimationGroup>
#include <QPointer>

#include "../diagramcontent.h"

class Diagram;

/**
 * @brief The MoveGraphicsItemCommand class
 * An undo command used for move item(s) in a diagram
 */
class MoveGraphicsItemCommand : public QUndoCommand
{
	public:
		MoveGraphicsItemCommand(Diagram *diagram,
								const DiagramContent &content,
								const QPointF &movement,
								QUndoCommand *parent = nullptr);

		~MoveGraphicsItemCommand() {}

	private:
		MoveGraphicsItemCommand(const MoveGraphicsItemCommand &);

	public:
		void undo() override;
		void redo() override;

	private:
		void move(const QPointF &movement);
		void updateConductors(bool is_redo = false);
		void setupAnimation(QObject *target,
							const QByteArray &property_name,
							const QVariant &start,
							const QVariant &end);

	private:
		QPointer<Diagram> m_diagram;
		DiagramContent m_content;
		const QPointF m_movement;
		QParallelAnimationGroup m_anim_group;
		bool m_first_redo{true};
};

#endif // MOVEGRAPHICSITEMCOMMAND_H
