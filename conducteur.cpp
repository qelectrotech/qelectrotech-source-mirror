#include <QtDebug>
#include "conducteur.h"
#include "element.h"

/**
	Constructeur
	@param p1     Premiere Borne auquel le conducteur est lie
	@param p2     Seconde Borne auquel le conducteur est lie
	@param parent Element parent du conducteur (0 par defaut)
	@param scene  QGraphicsScene auquelle appartient le conducteur
*/
Conducteur::Conducteur(Borne *p1, Borne* p2, Element *parent, QGraphicsScene *scene) : QGraphicsPathItem(parent, scene) {
	// bornes que le conducteur relie
	borne1 = p1;
	borne2 = p2;
	// ajout du conducteur a la liste de conducteurs de chacune des deux bornes
	bool ajout_p1 = borne1 -> addConducteur(this);
	bool ajout_p2 = borne2 -> addConducteur(this);
	// en cas d'echec de l'ajout (conducteur deja existant notamment)
	if (!ajout_p1 || !ajout_p2) return;
	destroyed = false;
	// le conducteur est represente par un trait fin
	QPen t;
	t.setWidthF(1.0);
	setPen(t);
	// calcul du rendu du conducteur
	calculeConducteur();
}

/**
	Met a jour la representation graphique du conducteur.
	@param rect Rectangle a mettre a jour
*/
void Conducteur::update(const QRectF &rect = QRectF()) {
	calculeConducteur();
	QGraphicsPathItem::update(rect);
}

/**
	Met a jour la representation graphique du conducteur.
	@param x      abscisse  du rectangle a mettre a jour
	@param y      ordonnee du rectangle a mettre a jour
	@param width  longueur du rectangle a mettre a jour
	@param height hauteur du rectangle a mettre a jour
*/
void Conducteur::update(qreal x, qreal y, qreal width, qreal height) {
	calculeConducteur();
	QGraphicsPathItem::update(x, y, width, height);
}

/**
	Destructeur du Conducteur. Avant d'etre detruit, le conducteur se decroche des bornes
	auxquelles il est lie.
*/
/*Conducteur::~Conducteur() {

}*/

/**
	Met a jour le QPainterPath constituant le conducteur pour obtenir
	un conducteur uniquement compose de droites reliant les deux bornes.
*/
void Conducteur::calculeConducteur() {
	QPainterPath t;
	
	QPointF p1 = borne1 -> amarrageConducteur();
	QPointF p2 = borne2 -> amarrageConducteur();
	
	QPointF depart, arrivee;
	Borne::Orientation ori_depart, ori_arrivee;
	// distingue le depart de l'arrivee : le trajet se fait toujours de gauche a droite
	if (p1.x() <= p2.x()) {
		depart      = mapFromScene(p1);
		arrivee     = mapFromScene(p2);
		ori_depart  = borne1 -> orientation();
		ori_arrivee = borne2 -> orientation();
	} else {
		depart      = mapFromScene(p2);
		arrivee     = mapFromScene(p1);
		ori_depart  = borne2 -> orientation();
		ori_arrivee = borne1 -> orientation();
	}
	
	// debut du trajet
	t.moveTo(depart);
	if (depart.y() < arrivee.y()) {
		// trajet descendant
		if ((ori_depart == Borne::Nord && (ori_arrivee == Borne::Sud || ori_arrivee == Borne::Ouest)) || (ori_depart == Borne::Est && ori_arrivee == Borne::Ouest)) {
			// cas « 3 »
			qreal ligne_inter_x = (depart.x() + arrivee.x()) / 2.0;
			t.lineTo(ligne_inter_x, depart.y());
			t.lineTo(ligne_inter_x, arrivee.y());
		} else if ((ori_depart == Borne::Sud && (ori_arrivee == Borne::Nord || ori_arrivee == Borne::Est)) || (ori_depart == Borne::Ouest && ori_arrivee == Borne::Est)) {
			// cas « 4 »
			qreal ligne_inter_y = (depart.y() + arrivee.y()) / 2.0;
			t.lineTo(depart.x(), ligne_inter_y);
			t.lineTo(arrivee.x(), ligne_inter_y);
		} else if ((ori_depart == Borne::Nord || ori_depart == Borne::Est) && (ori_arrivee == Borne::Nord || ori_arrivee == Borne::Est)) {
			t.lineTo(arrivee.x(), depart.y()); // cas « 2 »
		} else t.lineTo(depart.x(), arrivee.y()); // cas « 1 »
	} else {
		// trajet montant
		if ((ori_depart == Borne::Ouest && (ori_arrivee == Borne::Est || ori_arrivee == Borne::Sud)) || (ori_depart == Borne::Nord && ori_arrivee == Borne::Sud)) {
			// cas « 3 »
			qreal ligne_inter_y = (depart.y() + arrivee.y()) / 2.0;
			t.lineTo(depart.x(), ligne_inter_y);
			t.lineTo(arrivee.x(), ligne_inter_y);
		} else if ((ori_depart == Borne::Est && (ori_arrivee == Borne::Ouest || ori_arrivee == Borne::Nord)) || (ori_depart == Borne::Sud && ori_arrivee == Borne::Nord)) {
			// cas « 4 »
			qreal ligne_inter_x = (depart.x() + arrivee.x()) / 2.0;
			t.lineTo(ligne_inter_x, depart.y());
			t.lineTo(ligne_inter_x, arrivee.y());
		} else if ((ori_depart == Borne::Ouest || ori_depart == Borne::Nord) && (ori_arrivee == Borne::Ouest || ori_arrivee == Borne::Nord)) {
			t.lineTo(depart.x(), arrivee.y()); // cas « 2 »
		} else t.lineTo(arrivee.x(), depart.y()); // cas « 1 »
	}
	// fin du trajet
	t.lineTo(arrivee);
	setPath(t);
}

/**
	Dessine le conducteur sans antialiasing.
	@param qp Le QPainter a utiliser pour dessiner le conducteur
	@param qsogi Les options de style pour le conducteur
	@param qw Le QWidget sur lequel on dessine 
*/
void Conducteur::paint(QPainter *qp, const QStyleOptionGraphicsItem *qsogi, QWidget *qw) {
	qp -> save();
	qp -> setRenderHint(QPainter::Antialiasing,          false);
	qp -> setRenderHint(QPainter::TextAntialiasing,      false);
	qp -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	QGraphicsPathItem::paint(qp, qsogi, qw);
	qp -> restore();
}

/**
	Indique si deux orientations de Borne sont sur le meme axe (Vertical / Horizontal).
	@param a La premiere orientation de Borne
	@param b La seconde orientation de Borne
	@return Un booleen a true si les deux orientations de bornes sont sur le meme axe
*/
bool Conducteur::surLeMemeAxe(Borne::Orientation a, Borne::Orientation b) {
	if ((a == Borne::Nord || a == Borne::Sud) && (b == Borne::Nord || b == Borne::Sud)) return(true);
	else if ((a == Borne::Est || a == Borne::Ouest) && (b == Borne::Est || b == Borne::Ouest)) return(true);
	else return(false);
}

/**
	Indique si une orientation de borne est horizontale (Est / Ouest).
	@param a L'orientation de borne
	@return True si l'orientation de borne est horizontale, false sinon
*/
bool Conducteur::estHorizontale(Borne::Orientation a) {
	return(a == Borne::Est || a == Borne::Ouest);
}

/**
	Indique si une orientation de borne est verticale (Nord / Sud).
	@param a L'orientation de borne
	@return True si l'orientation de borne est verticale, false sinon
*/
bool Conducteur::estVerticale(Borne::Orientation a) {
	return(a == Borne::Nord || a == Borne::Sud);
}

/**
	Methode de preparation a la destruction du conducteur ; le conducteur se detache de ses deux bornes
*/
void Conducteur::destroy() {
	destroyed = true;
	borne1 -> removeConducteur(this);
	borne2 -> removeConducteur(this);
}

/**
	Methode de validation d'element XML
	@param e Un element XML sense represente un Conducteur
	@return true si l'element XML represente bien un Conducteur ; false sinon
*/
bool Conducteur::valideXml(QDomElement &e){
	// verifie le nom du tag
	if (e.tagName() != "conducteur") return(false);
	
	// verifie la presence des attributs minimaux
	if (!e.hasAttribute("borne1")) return(false);
	if (!e.hasAttribute("borne2")) return(false);
	
	bool conv_ok;
	// parse l'abscisse
	e.attribute("borne1").toInt(&conv_ok);
	if (!conv_ok) return(false);
	
	// parse l'ordonnee
	e.attribute("borne2").toInt(&conv_ok);
	if (!conv_ok) return(false);
	return(true);
}
