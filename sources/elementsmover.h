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
#ifndef ELEMENTS_MOVER_H
#define ELEMENTS_MOVER_H

#include <QHash>
#include <QPointF>
#include <QPointer>
#include "diagramcontent.h"

class CablePart;
class ConductorTextItem;
class Diagram;
class QStatusBar;
class QUndoCommand;

/**
	This class manages the interactive movement of different items (elements,
	conductors, text items etc...) on a particular diagram.

	A movement work in 3 steps:
	1: beginMovement    -> init a new movement
	2: continueMovement -> continue the current movement
	3: endMovement      -> finish the current movement

	A movement in progress must finish before starting a new movement. We can
	know if element mover is ready for a new movement by calling isReady().
*/
class ElementsMover {
		// constructors, destructor
	public:
		ElementsMover();
		virtual ~ElementsMover();
	private:
		ElementsMover(const ElementsMover &);
	
	// methods
	public:
		bool isReady() const;
		int  beginMovement(Diagram *, QGraphicsItem * = nullptr);
		void continueMovement(const QPointF &);
		void endMovement();
		bool holds(const QGraphicsItem *item) const;
			/**
				Steps of items this mover does not move itself -- the
				cable lines the user dragged himself, which know how to
				carry their own colour labels along -- handed over so
				that the end of this movement pushes them together with
				its own: one gesture, one undo step, whatever the
				selection is made of.
				@param steps the commands, given away with them
			*/
		void addExtraCommands(const QList<QUndoCommand *> &steps);
			/**
				Call the whole movement off without writing anything:
				every element goes back to the place the gesture found it
				and every line of the selection puts itself back too (see
				CablePart), so nothing at all has to be undone.

				Answering "Annuler" to a question asked while the lines
				settle is what calls it: one gesture is written as a
				whole or not at all.
			*/
		void cancelMovement();
	
		// attributes
	private:
		bool m_movement_running{false};
		QPointF m_current_movement;
		Diagram *m_diagram{nullptr};
		QGraphicsItem *m_movement_driver{nullptr};
		bool m_driver_held{false};
		DiagramContent m_moved_content;
		QPointer<QStatusBar> m_status_bar;
			///The cable lines marked together with the elements, moved
			///by this mover from one gesture to the next (see
			///CablePart::beginLineGesture) -- empty whenever the finger
			///is on a line itself, since a line then carries its own
		QList<CablePart *> m_moved_cables;
			///Steps handed over by a line the user dragged himself, to
			///be pushed together with the rest of the movement
		QList<QUndoCommand *> m_extra_commands;
			///Where every item of the movement stood when it began, so
			///that calling it off puts them all back exactly there
		QHash<QGraphicsItem *, QPointF> m_start_positions;

};
#endif
