#include "del.h"
#include <QPen>
#include <QGraphicsTextItem>

/**
	Constructeur
	@param parent Le QObject parent de l'element.
	@param scene  La scene sur laquelle l'element est affiche
 */
DEL::DEL(QGraphicsItem *parent, Schema *scene) : ElementFixe(parent, scene) {
	// taille et hotspot
	setSize(30, 70);
	setHotspot(QPoint(15, 5));
	
	// ajout de deux bornes a l'element
	new Borne(0,  0, Borne::Nord, this, scene);
	new Borne(0, 60, Borne::Sud,  this, scene);
	
	peut_relier_ses_propres_bornes = true;
}

/**
	@return Le nombre actuel de bornes de l'element
*/
int DEL::nbBornes() const {
	return(2);
}

/**
	Fonction qui effectue le rendu graphique de la DEL
	@param p Le QPainter a utiliser pour dessiner l'element
	@param o Les options de dessin
*/
void DEL::paint(QPainter *p, const QStyleOptionGraphicsItem *) {
	// traits de couleur noire
	QPen t;
	t.setColor(Qt::black);
	t.setWidthF(1.0);
	p -> setPen(t);
	
	// un cercle a fond blanc
	p -> setBrush(QBrush(Qt::white, Qt::SolidPattern));
	p -> drawEllipse(-10, 20, 20, 20);
	p -> setBrush(Qt::NoBrush);
	// deux lignes eventuellement antialiasees
	p -> drawLine(-7, 23,  7, 37);
	p -> drawLine( 7, 23, -7, 37);
	// deux lignes JAMAIS antialiasees
	p -> save();
	p -> setRenderHint(QPainter::Antialiasing,          false);
	p -> setRenderHint(QPainter::TextAntialiasing,      false);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	p -> drawLine(0,  0, 0, 20);
	p -> drawLine(0, 40, 0, 60);
	p -> restore();
}

/**
	@return l'ID du type "DEL"
*/
QString DEL::typeId() {
	return(QString("1"));
}
