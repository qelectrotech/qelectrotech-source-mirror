#ifndef SCHEMA_H
	#define SCHEMA_H
	#define GRILLE_X 10
	#define GRILLE_Y 10
	#include <QtGui>
	#include <QtXml>
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
		QImage toImage();
		QDomDocument toXml(bool = true);
		bool fromXml(QDomDocument &, QPointF = QPointF());
		void reset();
		QGraphicsItem *getElementById(uint id);
		
		private:
		QGraphicsLineItem *poseur_de_conducteur;
		bool doit_dessiner_grille;
		// elements du cartouche
		QString auteur;
		QDate   date;
		QString titre;
		QString folio;       // vraiment necessaire ce truc ?
		QString nom_fichier; // meme remarque
		Element *elementFromXml(QDomElement &e, QHash<int, Borne *> &);
		
		private slots:
		void slot_checkSelectionChange();
		
		signals:
		void selectionChanged();
	};
#endif
