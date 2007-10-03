#ifndef CONDUCTOR_SEGMENT_H
#define CONDUCTOR_SEGMENT_H
#include <QPointF>
#include "qet.h"
/**
	Cette classe represente un segment de conducteur.
*/
class ConductorSegment {
	
	// constructeurs, destructeur
	public:
	ConductorSegment(const QPointF &, const QPointF &, ConductorSegment * = NULL, ConductorSegment * = NULL);
	virtual ~ConductorSegment();
	
	private:
	ConductorSegment(const ConductorSegment &);
	
	// attributs
	private:
	ConductorSegment *previous_segment;
	ConductorSegment *next_segment;
	QPointF point1;
	QPointF point2;
	
	// methodes
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
	bool canMove1stPointX(const qreal &, qreal &) const;
	bool canMove2ndPointX(const qreal &, qreal &) const;
	bool canMove1stPointY(const qreal &, qreal &) const;
	bool canMove2ndPointY(const qreal &, qreal &) const;
};
#endif
