#include "contacteur.h"

/**
	Constructeur
	@param parent Le QObject parent de l'element.
	@param scene  La scene sur laquelle l'element est affiche
*/
Contacteur::Contacteur(QGraphicsItem *parent, Schema *scene) : ElementFixe(parent, scene) {
	// taille et hotspot
	setSize(15, 70);
	setHotspot(QPoint(10, 5));
	
	// ajout de deux bornes a l'element
	new Borne(0,  0, Borne::Nord, this, scene);
	new Borne(0, 60, Borne::Sud,  this, scene);
}

/**
	@return Le nombre actuel de bornes de l'element
*/
int Contacteur::nbBornes() const {
	return(2);
}

/**
	Fonction qui effectue le rendu graphique du contacteur
	@param p Le QPainter a utiliser pour dessiner l'element
	@param o Les options de dessin
*/
void Contacteur::paint(QPainter *p, const QStyleOptionGraphicsItem *) {
	// traits de couleur noire
	QPen t;
	t.setColor(Qt::black);
	t.setWidthF(1.0);
	t.setJoinStyle(Qt::MiterJoin);
	p -> setPen(t);
	
	// une ligne eventuellement antialiasee
	p -> drawLine(-5, 19, 0, 40);
	
	// deux lignes JAMAIS antialiasees (annulation des renderhints)
	p -> save();
	p -> setRenderHint(QPainter::Antialiasing,          false);
	p -> setRenderHint(QPainter::TextAntialiasing,      false);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	p -> drawLine(0, 0,  0, 20);
	p -> drawLine(0, 40, 0, 60);
	p -> restore();
}

/**
	@return l'ID du type "Contacteur"
*/
QString Contacteur::typeId() {
	return(QString("0"));
}
