#ifndef CONDUCER_SEGMENT_H
#define CONDUCER_SEGMENT_H
#include <QPointF>
#include "qet.h"
/**
	Cette classe represente un segment de conducteur.
*/
class ConducerSegment {
	
	// constructeurs, destructeur
	public:
	ConducerSegment(const QPointF &, const QPointF &, ConducerSegment * = NULL, ConducerSegment * = NULL);
	virtual ~ConducerSegment();
	
	private:
	ConducerSegment(const ConducerSegment &);
	
	// attributs
	private:
	ConducerSegment *previous_segment;
	ConducerSegment *next_segment;
	QPointF point1;
	QPointF point2;
	
	// methodes
	public:
	void moveX(const qreal &);
	void moveY(const qreal &);
	ConducerSegment *previousSegment() const;
	ConducerSegment *nextSegment()  const;
	bool hasPreviousSegment() const;
	bool hasNextSegment() const;
	void setPreviousSegment(ConducerSegment *);
	void setNextSegment(ConducerSegment *);
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
	QET::ConducerSegmentType type() const;
	qreal length() const;
	bool canMove1stPointX(const qreal &, qreal &) const;
	bool canMove2ndPointX(const qreal &, qreal &) const;
	bool canMove1stPointY(const qreal &, qreal &) const;
	bool canMove2ndPointY(const qreal &, qreal &) const;
};
#endif
