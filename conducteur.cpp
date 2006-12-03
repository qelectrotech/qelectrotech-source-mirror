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
	setFlags(QGraphicsItem::ItemIsSelectable);
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
	Met a jour la representation graphique du conducteur en considerant que la borne b
	a pour position pos
	@param rect Rectangle a mettre a jour
	@param b Borne
	@param pos position de la borne b
*/
void Conducteur::updateWithNewPos(const QRectF &rect, const Borne *b, const QPointF &newpos) {
	calculeConducteurWithNewPos(b, newpos);
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
	Met a jour le QPainterPath constituant le conducteur pour obtenir
	un conducteur uniquement compose de droites reliant les deux bornes.
*/
void Conducteur::calculeConducteur() {
	QPointF p1 = borne1 -> amarrageConducteur();
	QPointF p2 = borne2 -> amarrageConducteur();
	priv_calculeConducteur(p1, p2);
}

/**
	Met a jour le QPainterPath constituant le conducteur pour obtenir
	un conducteur uniquement compose de droites reliant les deux bornes.
*/
void Conducteur::calculeConducteurWithNewPos(const Borne *b, const QPointF &newpos) {
	QPointF p1, p2;
	if (b == borne1) {
		p1 = newpos;
		p2 = borne2 -> amarrageConducteur();
	} else if (b == borne2) {
		p1 = borne1 -> amarrageConducteur();
		p2 = newpos;
	} else {
		p1 = borne1 -> amarrageConducteur();
		p2 = borne2 -> amarrageConducteur();
	}
	priv_calculeConducteur(p1, p2);
}

/**
	@param p1 Coordonnees du point d'amarrage de la borne 1
	@param p2 Coordonnees du point d'amarrage de la borne 2
*/
void Conducteur::priv_calculeConducteur(const QPointF &p1, const QPointF &p2) {
	QPainterPath t;
	QPointF sp1, sp2, depart, newp1, newp2, arrivee, depart0, arrivee0;
	Borne::Orientation ori_borne1, ori_borne2, ori_depart, ori_arrivee;
	
	// recupere les orientations des bornes
	ori_borne1 = borne1 -> orientation();
	ori_borne2 = borne2 -> orientation();
	
	// mappe les points par rapport a la scene
	sp1 = mapFromScene(p1);
	sp2 = mapFromScene(p2);
	
	// tailles des prolongements
	qreal first_seg_size = 10;
	qreal last_seg_size  = 10;
	
	// prolonge la borne 1
	switch(ori_borne1) {
		case Borne::Nord:
			newp1 = QPointF(sp1.x(), sp1.y() - first_seg_size);
			break;
		case Borne::Est:
			newp1 = QPointF(sp1.x() + first_seg_size, sp1.y());
			break;
		case Borne::Sud:
			newp1 = QPointF(sp1.x(), sp1.y() + first_seg_size);
			break;
		case Borne::Ouest:
			newp1 = QPointF(sp1.x() - first_seg_size, sp1.y());
			break;
		default: newp1 = sp1;
	}
	
	// prolonge la borne 2
	switch(ori_borne2) {
		case Borne::Nord:
			newp2 = QPointF(sp2.x(), sp2.y() - last_seg_size);
			break;
		case Borne::Est:
			newp2 = QPointF(sp2.x() + last_seg_size, sp2.y());
			break;
		case Borne::Sud:
			newp2 = QPointF(sp2.x(), sp2.y() + last_seg_size);
			break;
		case Borne::Ouest:
			newp2 = QPointF(sp2.x() - last_seg_size, sp2.y());
			break;
		default: newp2 = sp2;
	}
	
	// distingue le depart de l'arrivee : le trajet se fait toujours de gauche a droite
	if (newp1.x() <= newp2.x()) {
		depart      = newp1;
		arrivee     = newp2;
		depart0     = sp1;
		arrivee0    = sp2;
		ori_depart  = ori_borne1;
		ori_arrivee = ori_borne2;
	} else {
		depart      = newp2;
		arrivee     = newp1;
		depart0     = sp2;
		arrivee0    = sp1;
		ori_depart  = ori_borne2;
		ori_arrivee = ori_borne1;
	}
	
	// debut du trajet
	t.moveTo(depart0);
	
	// prolongement de la borne de depart 
	t.lineTo(depart);
	
	// commence le vrai trajet
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
	
	// fin du vrai trajet
	t.lineTo(arrivee);
	
	// prolongement de la borne d'arrivee
	t.lineTo(arrivee0);
	
	setPath(t);
}

/**
	Dessine le conducteur sans antialiasing.
	@param qp Le QPainter a utiliser pour dessiner le conducteur
	@param qsogi Les options de style pour le conducteur
	@param qw Le QWidget sur lequel on dessine 
*/
void Conducteur::paint(QPainter *qp, const QStyleOptionGraphicsItem */*qsogi*/, QWidget */*qw*/) {
	qp -> save();
	qp -> setRenderHint(QPainter::Antialiasing,          false);
	qp -> setRenderHint(QPainter::TextAntialiasing,      false);
	qp -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	
	// recupere le QPen et la QBrush du QPainter
	QPen pen = qp -> pen();
	QBrush brush = qp -> brush();
	
	// attributs par defaut
	pen.setJoinStyle(Qt::MiterJoin);
	pen.setCapStyle(Qt::SquareCap);
	pen.setColor(isSelected() ? Qt::red : Qt::black);
	pen.setStyle(Qt::SolidLine);
	pen.setWidthF(1.0);
	brush.setStyle(Qt::NoBrush);
	
	// affectation du QPen et de la QBrush modifies au QPainter 
	qp -> setPen(pen);
	qp -> setBrush(brush);
	
	qp -> drawPath(path());
	//QGraphicsPathItem::paint(qp, qsogi, qw);
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
