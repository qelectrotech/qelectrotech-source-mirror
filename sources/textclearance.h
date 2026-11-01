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
#ifndef TEXTCLEARANCE_H
#define TEXTCLEARANCE_H

#include <QList>
#include <QPointF>
#include <QPointer>

class Diagram;
class DiagramTextItem;
class QUndoCommand;

/**
	Keep texts clear of wires. A symbol's label or a free text that sits on
	a wire is moved the shortest way that leaves a gap between its box and
	the wire. The plan is made first, so the dialog can say how many texts
	would move; applying it is one undo step.

	Only wires are obstacles. Texts of a wire, texts in a group and texts
	that are hidden are left where they are.
*/
namespace TextClearance
{
	enum Direction {
		Any,        ///< whichever way is shortest
		Horizontal, ///< left or right only
		Vertical    ///< up or down only
	};

	struct Settings {
		qreal distance = 4;  ///< gap kept between a text box and a wire
		qreal maxMove = 30;  ///< a text is not moved further than this
		Direction direction = Any;
	};

	struct Move {
		QPointer<DiagramTextItem> text;
		QPointF before; ///< position of the text, in its parent
		QPointF after;
	};

	struct Plan {
		QList<Move> moves;
		int considered = 0;   ///< texts looked at
		int clear = 0;        ///< already clear of every wire
		int unresolved = 0;   ///< on a wire, no free place within the limit
	};

	Plan plan(Diagram *diagram, const Settings &settings, bool selectedOnly);
	QUndoCommand *command(const Plan &plan);
}

#endif // TEXTCLEARANCE_H
