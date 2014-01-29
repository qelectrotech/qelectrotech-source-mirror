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
#ifndef GHOST_ELEMENT_H
#define GHOST_ELEMENT_H
#include "customelement.h"
class Diagram;
class QGraphicsItem;
class ElementsLocation;
class Terminal;
/**
	The GhostElement class inherits CustomElement. A GhostElement aims at
	visually replacing a CustomElement whose definition could not be loaded.
	This way, instead of not loading an element, thus potentially losing its
	position, its orientation, its child text items and conductors, one can
	substitute a GhostElement. The GhostElement will extrapolate the position
	of terminals and text items from the rest of the diagram. It is visually
	rendered using a simple rectangle.
*/
class GhostElement : public CustomElement {
	
	Q_OBJECT
	
	// constructor, destructor
	public:
	GhostElement(const ElementsLocation &, QGraphicsItem * = 0, Diagram * = 0);
	virtual ~GhostElement();
	
	// methods
	public:
	virtual bool fromXml(QDomElement &, QHash<int, Terminal *> &, bool = false);
	virtual int linkType() const {return Simple;}
	
	protected:
	QRectF minimalBoundingRect() const;
	bool terminalsFromXml(QDomElement &, QHash<int, Terminal *> &);
	void generateDrawings();
	void generateDrawing(QPainter *);
};
#endif
