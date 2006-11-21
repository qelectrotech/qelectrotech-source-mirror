#ifndef ELEMENTPERSO_H
	#define ELEMENTPERSO_H
	#include "elementfixe.h"
	#include <QtGui>
	class ElementPerso : public ElementFixe {
		public:
		ElementPerso(QString &, QGraphicsItem * = 0, Schema * = 0, int * = NULL);
		virtual int nbBornes() const;
		virtual void paint(QPainter *, const QStyleOptionGraphicsItem *);
		inline QString typeId() { return(nomfichier); }
		inline QString fichier() { return(nomfichier); }
		inline bool isNull() { return(elmt_etat != 0); }
		inline int etat() { return(elmt_etat); }
		inline QString nom() { return(priv_nom); }
		
		private:
		int elmt_etat; // contient le code d'erreur si l'instanciation a echoue ou 0 si l'instanciation s'est bien passe
		QString priv_nom;
		QString nomfichier;
		QPicture dessin;
		bool parseElement(QDomElement &, QPainter &, Schema *);
		bool parseLigne(QDomElement &, QPainter &);
		bool parseEllipse(QDomElement &, QPainter &);
		bool parseCercle(QDomElement &, QPainter &);
		bool parsePolygone(QDomElement &, QPainter &);
		bool parseBorne(QDomElement &, Schema *);
		void setQPainterAntiAliasing(QPainter &, bool);
		bool attributeIsAnInteger(QDomElement &, QString, int * = NULL);
		bool attributeIsAReal(QDomElement &, QString, double * = NULL);
		bool validOrientationAttribute(QDomElement &);
		void setPainterStyle(QDomElement &, QPainter &);
		int nb_bornes;
	};
#endif
