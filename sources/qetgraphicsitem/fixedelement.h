/*
	Copyright 2006-2016 The QElectroTech Team
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
#ifndef ELEMENTFIXE_H
#define ELEMENTFIXE_H

#include "element.h"

/**
	This class represents an element having a fixed number of terminals.
*/
class FixedElement : public Element {
	
	Q_OBJECT
	
	// constructors, destructor
	public:
	FixedElement(QGraphicsItem * = 0);
	virtual ~FixedElement();
	
	// methods
	public:
	int minTerminalsCount() const;
	int maxTerminalsCount() const;
	virtual int terminalsCount() const = 0;
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *) = 0;
	virtual QString typeId() const = 0;
	virtual QString name() const = 0;
};
#endif
