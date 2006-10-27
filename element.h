#ifndef ELEMENT_H
	#define ELEMENT_H
	#include <QtGui>
	#include "borne.h"
	class Schema;
	class Element : public QGraphicsItem {
		public:
		enum { Type = UserType + 1000 };
    	virtual int type() const { return Type; }
		Element(QGraphicsItem * = 0, Schema * = 0);
		
		virtual  int nbBornes()    const = 0;
		virtual  int nbBornesMin() const = 0;
		virtual  int nbBornesMax() const = 0;
		virtual  void paint(QPainter *, const QStyleOptionGraphicsItem *) = 0;
		virtual  QString typeId() = 0;
		
		virtual QString  nom() = 0;
		void     paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
		QRectF   boundingRect() const;
		QSize    setSize(int, int);
		QPoint   setHotspot(QPoint);
		QPoint   hotspot() const;
		void     select();
		void     deselect();
		QPixmap  pixmap();
		QVariant itemChange(GraphicsItemChange, const QVariant &);
		bool     orientation() const;
		bool     invertOrientation();
		void     setPos(const QPointF &);
		void     setPos(qreal, qreal);
		bool     connexionsInternesAcceptees() { return(peut_relier_ses_propres_bornes); }
		static bool     valideXml(QDomElement &);
		virtual bool fromXml(QDomElement &, QHash<int, Borne *>&) = 0;
		
		protected:
		void drawAxes(QPainter *, const QStyleOptionGraphicsItem *);
		void mouseMoveEvent(QGraphicsSceneMouseEvent *);
		bool peut_relier_ses_propres_bornes;
		
		private:
		void drawSelection(QPainter *, const QStyleOptionGraphicsItem *);
		void updatePixmap();
		bool    sens;
		QSize   dimensions;
		QPoint  hotspot_coord;
		QPixmap apercu;
		QMenu   menu;
	};
#endif
