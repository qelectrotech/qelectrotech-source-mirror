#ifndef ELEMENT_H
	#define ELEMENT_H
	#include <QtGui>
	#include "borne.h"
	class Diagram;
	class Element : public QGraphicsItem {
		public:
		enum { Type = UserType + 1000 };
    	virtual int type() const { return Type; }
		Element(QGraphicsItem * = 0, Diagram * = 0);
		
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
		inline bool     connexionsInternesAcceptees() { return(peut_relier_ses_propres_bornes); }
		inline void     setConnexionsInternesAcceptees(bool cia) { peut_relier_ses_propres_bornes = cia; }
		static bool     valideXml(QDomElement &);
		virtual bool fromXml(QDomElement &, QHash<int, Borne *>&) = 0;
		// methodes d'acces aux possibilites d'orientation
		inline Borne::Orientation orientation() { return(ori); }
		inline bool acceptOrientation(Borne::Orientation o) {
			switch(o) {
				case Borne::Nord:  return(ori_n);
				case Borne::Est:   return(ori_e);
				case Borne::Sud:   return(ori_s);
				case Borne::Ouest: return(ori_w);
				default: return(false);
			}
		}
		inline Borne::Orientation defaultOrientation() { return(ori_d); }
		inline Borne::Orientation nextAcceptableOrientation() {
			Borne::Orientation retour = nextOrientation(ori);
			for (int i = 0 ; i < 4 ; ++ i) {
				if (acceptOrientation(retour)) return(retour);
				retour = nextOrientation(retour);
			}
			// on ne devrait pas arriver la : renvoi d'une valeur par defaut = nord
			return(Borne::Nord);
		}
		inline Borne::Orientation previousAcceptableOrientation() {
			Borne::Orientation retour = previousOrientation(ori);
			for (int i = 0 ; i < 4 ; ++ i) {
				if (acceptOrientation(retour)) return(retour);
				retour = previousOrientation(retour);
			}
			// on ne devrait pas arriver la : renvoi d'une valeur par defaut = nord
			return(Borne::Nord);
		}
		bool setOrientation(Borne::Orientation o);
		
		protected:
		void drawAxes(QPainter *, const QStyleOptionGraphicsItem *);
		void mouseMoveEvent(QGraphicsSceneMouseEvent *);
		bool    ori_n;
		bool    ori_s;
		bool    ori_e;
		bool    ori_w;
		Borne::Orientation ori_d;
		Borne::Orientation ori;
		
		private:
		bool peut_relier_ses_propres_bornes;
		void drawSelection(QPainter *, const QStyleOptionGraphicsItem *);
		void updatePixmap();
		inline Borne::Orientation nextOrientation(Borne::Orientation o) {
			if (o < 0 || o > 2) return(Borne::Nord);
			return((Borne::Orientation)(o + 1));
		}
		inline Borne::Orientation previousOrientation(Borne::Orientation o) {
			if (o < 0 || o > 3) return(Borne::Nord);
			if (o == Borne::Nord) return(Borne::Ouest);
			return((Borne::Orientation)(o - 1));
		}
		
		QSize   dimensions;
		QPoint  hotspot_coord;
		QPixmap apercu;
		QMenu   menu;
	};
#endif
