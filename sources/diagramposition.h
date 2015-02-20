/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef DIAGRAM_POSITION_H
#define DIAGRAM_POSITION_H
#include <QPointF>
#include <QString>
#include <QRegExp>
/**
	This class stores the position of an electrical element on its parent diagram.
	While exact coordinates can be stored for convenience, the concept of diagram
	position as implemented by this class simply designates the intersection
	between a column and a row, e.g. B2 or C4.
*/
class DiagramPosition {
	// constructors, destructor
	public:
	DiagramPosition(const QString & = "", unsigned int = 0);
	virtual ~DiagramPosition();
	
	// methods
	public:
	QPointF position() const;
	void setPosition(const QPointF &);
	QString toString();
	bool isOutOfBounds() const;
	unsigned int number()const {return number_;}
	QString letter()const {return letter_;}
	
	// attributes
	private:
	QString letter_;
	unsigned int number_;
	QPointF position_;
};
#endif
