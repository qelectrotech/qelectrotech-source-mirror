/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef ELEMENT_TEXTS_MOVER_H
#define ELEMENT_TEXTS_MOVER_H
#include <QtGui>
#include "diagramcontent.h"
class ElementTextItem;
class Diagram;
/**
	This class manages the interactive movement of element text items on a
	particular diagram.
*/
class ElementTextsMover {
	// constructors, destructor
	public:
	ElementTextsMover();
	virtual ~ElementTextsMover();
	private:
	ElementTextsMover(const ElementTextsMover &);
	
	// methods
	public:
	bool isReady() const;
	int  beginMovement(Diagram *, QGraphicsItem * = 0);
	void continueMovement(const QPointF &);
	void endMovement();
	
	// attributes
	private:
	bool movement_running_;
	QPointF current_movement_;
	Diagram *diagram_;
	QGraphicsItem *movement_driver_;
	QSet<ElementTextItem *> moved_texts_;
};
#endif
