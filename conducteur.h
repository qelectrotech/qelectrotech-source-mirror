#ifndef CONDUCTEUR_H
	#define CONDUCTEUR_H
	#include <QtGui>
	#include "borne.h"
	class Element;
	/**
		Cette classe represente un conducteur. Un conducteur relie deux bornes d'element.
	*/
	class Conducteur : public QGraphicsPathItem {
		public:
		enum { Type = UserType + 1001 };
    	virtual int type() const { return Type; }
		Conducteur(Borne *, Borne *, Element * = 0, QGraphicsScene * = 0);
		//virtual ~Conducteur();
		
		void destroy();
		bool isDestroyed() const { return(destroyed); }
		void updateWithNewPos(const QRectF &, const Borne *, const QPointF &);
		void update(const QRectF & = QRectF());
		void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
		QRectF boundingRect() const;
		virtual QPainterPath shape() const;
		static bool valideXml(QDomElement &);
		
		///Premiere borne a laquelle le fil est rattache
		Borne *borne1;
		///Deuxieme borne a laquelle le fil est rattache
		Borne *borne2;
		
		protected:
		void mousePressEvent(QGraphicsSceneMouseEvent *);
		void mouseMoveEvent(QGraphicsSceneMouseEvent *);
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
		
		private:
		/// booleen indiquant si le fil est encore valide
		bool destroyed;
		QList<QPointF> points;
		QList<qreal> moves_x;
		QList<qreal> moves_y;
		qreal orig_dist_2_terms_x;
		qreal orig_dist_2_terms_y;
		bool type_trajet_x;
		QPointF press_point;
		bool moving_point;
		int moved_point;
		bool modified_path;
		static QPen conducer_pen;
		static QBrush conducer_brush;
		static bool pen_and_brush_initialized;
		
		void pointsToPath();
		void updatePoints();
		void priv_calculeConducteur(const QPointF &, Borne::Orientation, const QPointF &, Borne::Orientation);
		void priv_modifieConducteur(const QPointF &, Borne::Orientation, const QPointF &, Borne::Orientation);
		static QPointF extendTerminal(const QPointF &, Borne::Orientation, qreal = 12.0);
		static bool surLeMemeAxe(Borne::Orientation, Borne::Orientation);
		static bool estHorizontale(Borne::Orientation a);
		static bool estVerticale(Borne::Orientation a);
		static qreal conducer_bound(qreal tobound, qreal bound1, qreal bound2);
		static qreal conducer_bound(qreal tobound, qreal bound, bool positive);
	};
#endif
