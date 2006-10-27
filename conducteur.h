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
		void update(const QRectF & rect);
		void update(qreal x, qreal y, qreal width, qreal height);
		void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
		static bool valideXml(QDomElement &);
		
		///Premiere borne a laquelle le fil est rattache
		Borne *borne1;
		///Deuxieme borne a laquelle le fil est rattache
		Borne *borne2;
		private:
		/// booleen indiquant si le fil est encore valide
		bool destroyed;
		
		void calculeConducteur();
		bool surLeMemeAxe(Borne::Orientation, Borne::Orientation);
		bool estHorizontale(Borne::Orientation a);
		bool estVerticale(Borne::Orientation a);
	};
#endif
