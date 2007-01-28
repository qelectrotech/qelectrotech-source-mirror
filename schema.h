#ifndef SCHEMA_H
	#define SCHEMA_H
	#define GRILLE_X 10
	#define GRILLE_Y 10
	#define MARGIN   5.0
	#include <QtGui>
	#include <QtXml>
	#include "qetapp.h"
	#include "borderinset.h"
	class Element;
	class Borne;
	class Schema : public QGraphicsScene {
		Q_OBJECT
		public:
		Schema(QObject * = 0);
		void drawBackground(QPainter *, const QRectF &);
		inline void poseConducteur(bool pf) {
			if (pf) {
				if (!poseur_de_conducteur -> scene()) addItem(poseur_de_conducteur);
			} else {
				if (poseur_de_conducteur -> scene()) removeItem(poseur_de_conducteur);
			}
		}
		inline void setDepart (QPointF d) { poseur_de_conducteur -> setLine(QLineF(d, poseur_de_conducteur -> line().p2())); }
		inline void setArrivee(QPointF a) { poseur_de_conducteur -> setLine(QLineF(poseur_de_conducteur -> line().p1(), a)); }
		QImage toImage(int = -1, int = -1, bool = true);
		QSize imageSize() const;
		QDomDocument toXml(bool = true);
		bool fromXml(QDomDocument &, QPointF = QPointF(), bool = true);
		QGraphicsItem *getElementById(uint id);
		inline void setAffichageGrille(bool ddg) { doit_dessiner_grille = ddg; }
		BorderInset border_and_inset;
		QRectF border() const;
		
		private:
		QGraphicsLineItem *poseur_de_conducteur;
		bool doit_dessiner_grille;
		Element *elementFromXml(QDomElement &, QHash<int, Borne *> &);
		
		private slots:
		void slot_checkSelectionChange();
		
		signals:
		void selectionChanged();
	};
#endif
