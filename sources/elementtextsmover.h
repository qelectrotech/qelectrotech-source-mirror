/*
	Copyright 2006-2012 Xavier Guerrin
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
	Cette classe permet de gerer le deplacement des ElementTextItems d'un schema
	electrique.
*/
class ElementTextsMover {
	// constructeurs, destructeur
	public:
	ElementTextsMover();
	virtual ~ElementTextsMover();
	private:
	ElementTextsMover(const ElementTextsMover &);
	
	// methodes
	public:
	bool isReady() const;
	int  beginMovement(Diagram *, QGraphicsItem * = 0);
	void continueMovement(const QPointF &);
	void endMovement();
	
	// attributs
	private:
	bool movement_running_;
	QPointF current_movement_;
	Diagram *diagram_;
	QGraphicsItem *movement_driver_;
	QSet<ElementTextItem *> moved_texts_;
};
#endif
