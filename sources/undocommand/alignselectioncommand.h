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
#ifndef ALIGNSELECTIONCOMMAND_H
#define ALIGNSELECTIONCOMMAND_H

#include <QPointer>
#include <QUndoCommand>

class Diagram;
class DiagramContent;

/**
	@brief The AlignSelectionCommand class
	Moves each selected item by its own amount, as one undo step.
	Symbols, pictures, free texts and shapes take part; locked items are left
	where they are and counted, so the caller can say so.
*/
class AlignSelectionCommand : public QUndoCommand
{
	public:
		enum Mode {
			SnapToGrid, ///< put each item where a drag would have left it
			AlignLeft,
			AlignHCenter,
			AlignRight,
			AlignTop,
			AlignVCenter,
			AlignBottom
		};

		AlignSelectionCommand(Diagram *diagram, Mode mode, QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;

		bool isValid() const;
		int movedCount() const;
		int lockedCount() const;
		int itemCount() const;

		static int unitCount(const DiagramContent &dc);

	private:
		QPointer<Diagram> m_diagram;
		int m_locked_count = 0;
		int m_item_count = 0;
};

#endif // ALIGNSELECTIONCOMMAND_H
