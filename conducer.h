#ifndef CONDUCTEUR_H
	#define CONDUCTEUR_H
	#include <QtGui>
	#include "terminal.h"
	class ConducerSegment;
	class Element;
	/**
		Cette classe represente un conducteur. Un conducteur relie deux bornes d'element.
	*/
	class Conducer : public QGraphicsPathItem {
		public:
		enum { Type = UserType + 1001 };
    	virtual int type() const { return Type; }
		Conducer(Terminal *, Terminal *, Element * = 0, QGraphicsScene * = 0);
		//virtual ~Conducer();
		
		void destroy();
		bool isDestroyed() const { return(destroyed); }
		void updateWithNewPos(const QRectF &, const Terminal *, const QPointF &);
		void update(const QRectF & = QRectF());
		void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
		QRectF boundingRect() const;
		virtual QPainterPath shape() const;
		static bool valideXml(QDomElement &);
		bool fromXml(QDomElement &);
		void toXml(QDomDocument &, QDomElement &);
		
		///Premiere borne a laquelle le fil est rattache
		Terminal *terminal1;
		///Deuxieme borne a laquelle le fil est rattache
		Terminal *terminal2;
		
		protected:
		void mousePressEvent(QGraphicsSceneMouseEvent *);
		void mouseMoveEvent(QGraphicsSceneMouseEvent *);
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
		void hoverMoveEvent(QGraphicsSceneHoverEvent *);
		
		private:
		/// booleen indiquant si le fil est encore valide
		bool destroyed;
		ConducerSegment *segments;
		QList<qreal> moves_x;
		QList<qreal> moves_y;
		qreal orig_dist_2_terms_x;
		qreal orig_dist_2_terms_y;
		bool type_trajet_x;
		QPointF press_point;
		bool moving_point;
		bool moving_segment;
		int moved_point;
		qreal previous_z_value;
		ConducerSegment *moved_segment;
		bool modified_path;
		static QPen conducer_pen;
		static QBrush conducer_brush;
		static bool pen_and_brush_initialized;
		
		void segmentsToPath();
		void updatePoints();
		void priv_calculeConducer(const QPointF &, Terminal::Orientation, const QPointF &, Terminal::Orientation);
		void priv_modifieConducer(const QPointF &, Terminal::Orientation, const QPointF &, Terminal::Orientation);
		int nbSegments();
		
		QList<QPointF> segmentsToPoints() const;
		void pointsToSegments(QList<QPointF>);
		bool hasClickedOn(QPointF, QPointF);
		static QPointF extendTerminal(const QPointF &, Terminal::Orientation, qreal = 12.0);
		static bool surLeMemeAxe(Terminal::Orientation, Terminal::Orientation);
		static bool estHorizontale(Terminal::Orientation a);
		static bool estVerticale(Terminal::Orientation a);
		static qreal conducer_bound(qreal tobound, qreal bound1, qreal bound2);
		static qreal conducer_bound(qreal tobound, qreal bound, bool positive);
	};
#endif
