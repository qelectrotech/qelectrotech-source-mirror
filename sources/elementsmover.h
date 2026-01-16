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

#include <QPointF>
#include <QPointer>
#include "diagramcontent.h"

class ConductorTextItem;
class Diagram;
class QStatusBar;

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
	
		// attributes
	private:
		bool m_movement_running{false};
		QPointF m_current_movement;
		Diagram *m_diagram{nullptr};
		QGraphicsItem *m_movement_driver{nullptr};
		DiagramContent m_moved_content;
		QPointer<QStatusBar> m_status_bar;

};
#endif
