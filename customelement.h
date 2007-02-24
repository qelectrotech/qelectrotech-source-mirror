#ifndef ELEMENTPERSO_H
	#define ELEMENTPERSO_H
	#include "fixedelement.h"
	#include <QtGui>
	class CustomElement : public FixedElement {
		public:
		CustomElement(QString &, QGraphicsItem * = 0, Diagram * = 0, int * = NULL);
		virtual int nbTerminals() const;
		virtual void paint(QPainter *, const QStyleOptionGraphicsItem *);
		inline QString typeId() const { return(nomfichier); }
		inline QString fichier() const { return(nomfichier); }
		inline bool isNull() const { return(elmt_etat != 0); }
		inline int etat() const { return(elmt_etat); }
		inline QString nom() const { return(priv_nom); }
		
		private:
		int elmt_etat; // contient le code d'erreur si l'instanciation a echoue ou 0 si l'instanciation s'est bien passe
		QString priv_nom;
		QString nomfichier;
		QPicture dessin;
		bool parseElement(QDomElement &, QPainter &, Diagram *);
		bool parseLine(QDomElement &, QPainter &);
		bool parseEllipse(QDomElement &, QPainter &);
		bool parseCircle(QDomElement &, QPainter &);
		bool parseArc(QDomElement &, QPainter &);
		bool parsePolygon(QDomElement &, QPainter &);
		bool parseText(QDomElement &, QPainter &);
		bool parseInput(QDomElement &, Diagram *);
		bool parseTerminal(QDomElement &, Diagram *);
		void setQPainterAntiAliasing(QPainter &, bool);
		bool attributeIsAnInteger(QDomElement &, QString, int * = NULL);
		bool attributeIsAReal(QDomElement &, QString, double * = NULL);
		bool validOrientationAttribute(QDomElement &);
		void setPainterStyle(QDomElement &, QPainter &);
		int nb_terminals;
	};
#endif
