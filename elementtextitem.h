#ifndef ELEMENT_TEXT_ITEM_H
	#define ELEMENT_TEXT_ITEM_H
	#include <QGraphicsTextItem>
	class ElementTextItem : public QGraphicsTextItem {
		// constructeurs
		public:
		ElementTextItem(QGraphicsItem * = 0, QGraphicsScene * = 0);
		ElementTextItem(const QString &, QGraphicsItem * = 0, QGraphicsScene * = 0);
		
		// attributs
		private:
		bool follow_parent_rotations;
		
		// methodes
		public:
		enum { Type = UserType + 1003 };
    	virtual int type() const { return Type; }
		inline bool followParentRotations() const { return(follow_parent_rotations); }
		inline void setFollowParentRotations(bool frp) { follow_parent_rotations = frp; }
		void setPos(const QPointF &);
		void setPos(qreal, qreal);
		QPointF pos() const;
	};
#endif
