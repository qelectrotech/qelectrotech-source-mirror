/*
	Copyright 2006-2017 The QElectroTech Team
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
#ifndef CONDUCTOR_SEGMENT_H
#define CONDUCTOR_SEGMENT_H
#include <QPointF>
#include "qet.h"
/**
	This class represents a conductor segment.
*/
class ConductorSegment {
	
	// constructors, destructor
	public:
	ConductorSegment(const QPointF &, const QPointF &, ConductorSegment * = NULL, ConductorSegment * = NULL);
	virtual ~ConductorSegment();
	
	private:
	ConductorSegment(const ConductorSegment &);
	
	// attributes
	private:
	ConductorSegment *previous_segment;
	ConductorSegment *next_segment;
	QPointF point1;
	QPointF point2;
	
	// methods
	public:
	void moveX(const qreal &);
	void moveY(const qreal &);
	ConductorSegment *previousSegment() const;
	ConductorSegment *nextSegment()  const;
	bool hasPreviousSegment() const;
	bool hasNextSegment() const;
	void setPreviousSegment(ConductorSegment *);
	void setNextSegment(ConductorSegment *);
	bool isStatic() const;
	bool isFirstSegment() const;
	bool isLastSegment() const;
	QPointF firstPoint() const;
	QPointF secondPoint() const;
	void setFirstPoint(const QPointF &);
	void setSecondPoint(const QPointF &);
	QPointF middle() const;
	bool isHorizontal() const;
	bool isVertical() const;
	QET::ConductorSegmentType type() const;
	qreal length() const;
	bool isPoint() const;
	bool canMove1stPointX(const qreal &, qreal &) const;
	bool canMove2ndPointX(const qreal &, qreal &) const;
	bool canMove1stPointY(const qreal &, qreal &) const;
	bool canMove2ndPointY(const qreal &, qreal &) const;
};
#endif
