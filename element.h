#ifndef ELEMENT_H
	#define ELEMENT_H
	#include <QtGui>
	#include "terminal.h"
	class Diagram;
	class Element : public QGraphicsItem {
		public:
		enum { Type = UserType + 1000 };
    	virtual int type() const { return Type; }
		Element(QGraphicsItem * = 0, Diagram * = 0);
		
		virtual  int nbTerminals()    const = 0;
		virtual  int nbTerminalsMin() const = 0;
		virtual  int nbTerminalsMax() const = 0;
		virtual  void paint(QPainter *, const QStyleOptionGraphicsItem *) = 0;
		virtual  QString typeId() const = 0;
		
		virtual QString  nom() const = 0;
		void     paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
		QRectF   boundingRect() const;
		QSize    setSize(int, int);
		QPoint   setHotspot(QPoint);
		QPoint   hotspot() const;
		void     select();
		void     deselect();
		QPixmap  pixmap();
		QVariant itemChange(GraphicsItemChange, const QVariant &);
		void     setPos(const QPointF &);
		void     setPos(qreal, qreal);
		inline bool     connexionsInternesAcceptees() { return(peut_relier_ses_propres_terminals); }
		inline void     setConnexionsInternesAcceptees(bool cia) { peut_relier_ses_propres_terminals = cia; }
		static bool     valideXml(QDomElement &);
		virtual bool fromXml(QDomElement  &, QHash<int, Terminal *>&);
		virtual QDomElement toXml (QDomDocument &, QHash<Terminal *, int>&) const;
		// methodes d'acces aux possibilites d'orientation
		inline Terminal::Orientation orientation() const { return(ori); }
		inline bool acceptOrientation(Terminal::Orientation o) {
			switch(o) {
				case Terminal::Nord:  return(ori_n);
				case Terminal::Est:   return(ori_e);
				case Terminal::Sud:   return(ori_s);
				case Terminal::Ouest: return(ori_w);
				default: return(false);
			}
		}
		inline Terminal::Orientation defaultOrientation() { return(ori_d); }
		inline Terminal::Orientation nextAcceptableOrientation() {
			Terminal::Orientation retour = nextOrientation(ori);
			for (int i = 0 ; i < 4 ; ++ i) {
				if (acceptOrientation(retour)) return(retour);
				retour = nextOrientation(retour);
			}
			// on ne devrait pas arriver la : renvoi d'une valeur par defaut = nord
			return(Terminal::Nord);
		}
		inline Terminal::Orientation previousAcceptableOrientation() {
			Terminal::Orientation retour = previousOrientation(ori);
			for (int i = 0 ; i < 4 ; ++ i) {
				if (acceptOrientation(retour)) return(retour);
				retour = previousOrientation(retour);
			}
			// on ne devrait pas arriver la : renvoi d'une valeur par defaut = nord
			return(Terminal::Nord);
		}
		bool setOrientation(Terminal::Orientation o);
		
		protected:
		void drawAxes(QPainter *, const QStyleOptionGraphicsItem *);
		void mouseMoveEvent(QGraphicsSceneMouseEvent *);
		bool    ori_n;
		bool    ori_s;
		bool    ori_e;
		bool    ori_w;
		Terminal::Orientation ori_d;
		Terminal::Orientation ori;
		
		private:
		bool peut_relier_ses_propres_terminals;
		void drawSelection(QPainter *, const QStyleOptionGraphicsItem *);
		void updatePixmap();
		inline Terminal::Orientation nextOrientation(Terminal::Orientation o) {
			if (o < 0 || o > 2) return(Terminal::Nord);
			return((Terminal::Orientation)(o + 1));
		}
		inline Terminal::Orientation previousOrientation(Terminal::Orientation o) {
			if (o < 0 || o > 3) return(Terminal::Nord);
			if (o == Terminal::Nord) return(Terminal::Ouest);
			return((Terminal::Orientation)(o - 1));
		}
		
		QSize   dimensions;
		QPoint  hotspot_coord;
		QPixmap apercu;
		QMenu   menu;
	};
#endif
