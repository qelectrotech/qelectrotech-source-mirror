#ifndef CONDUCER_SEGMENT_H
	#define CONDUCER_SEGMENT_H
	#include <QPointF>
	/**
		Cette classe represente un segment de conducteur.
	*/
	class ConducerSegment {
		// constructeurs et destructeur
		public:
		ConducerSegment(QPointF, QPointF, ConducerSegment * = NULL, ConducerSegment * = NULL);
		~ConducerSegment();
		
		// attributs
		private:
		ConducerSegment *previous_segment;
		ConducerSegment *next_segment;
		QPointF point1;
		QPointF point2;
		
		// methodes
		public:
		void moveX(qreal);
		void moveY(qreal);
		ConducerSegment *previousSegment();
		ConducerSegment *nextSegment();
		bool hasPreviousSegment();
		bool hasNextSegment();
		void setPreviousSegment(ConducerSegment *);
		void setNextSegment(ConducerSegment *);
		QPointF firstPoint();
		QPointF secondPoint();
		void setFirstPoint(QPointF);
		void setSecondPoint(QPointF);
		QPointF middle();
		bool isHorizontal();
		bool isVertical();
		qreal length();
		bool canMove1stPointX(qreal, qreal &);
		bool canMove2ndPointX(qreal, qreal &);
		bool canMove1stPointY(qreal, qreal &);
		bool canMove2ndPointY(qreal, qreal &);
	};
#endif
