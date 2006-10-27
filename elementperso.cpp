#include "elementperso.h"

ElementPerso::ElementPerso(QString &nom_fichier, QGraphicsItem *qgi, Schema *s, int *etat) : ElementFixe(qgi, s) {
	nomfichier = nom_fichier;
	nb_bornes = 0;
	// pessimisme inside : par defaut, ca foire
	elmt_etat = -1;
	
	// le fichier doit exister
	QString chemin_elements = "elements/";
	nomfichier = chemin_elements + nom_fichier;
	if (!QFileInfo(nomfichier).exists()) {
		if (etat != NULL) *etat = 1;
		elmt_etat = 1;
		return;
	}
	
	// le fichier doit etre lisible
	QFile fichier(nomfichier);
	if (!fichier.open(QIODevice::ReadOnly)) {
		if (etat != NULL) *etat = 2;
		elmt_etat = 2;
		return;
	}
	
	// le fichier doit etre un document XML
	QDomDocument document_xml;
	if (!document_xml.setContent(&fichier)) {
		if (etat != NULL) *etat = 3;
		elmt_etat = 3;
		return;
	}
	
	// la racine est supposee etre une definition d'element 
	QDomElement racine = document_xml.documentElement();
	if (racine.tagName() != "definition" || racine.attribute("type") != "element") {
		if (etat != NULL) *etat = 4;
		elmt_etat = 4;
		return;
	}
	
	// ces attributs doivent etre presents et valides
	int w, h, hot_x, hot_y;
	if (
		racine.attribute("nom") == QString("") ||\
		!attributeIsAnInteger(racine, QString("width"), &w) ||\
		!attributeIsAnInteger(racine, QString("height"), &h) ||\
		!attributeIsAnInteger(racine, QString("hotspot_x"), &hot_x) ||\
		!attributeIsAnInteger(racine, QString("hotspot_y"), &hot_y)
	) {
		if (etat != NULL) *etat = 5;
		elmt_etat = 5;
		return;
	}
	
	// on peut d'ores et deja specifier le nom, la taille et le hotspot
	priv_nom = racine.attribute("nom");
	setSize(w, h);
	setHotspot(QPoint(hot_x, hot_y));
	
	// la definition est supposee avoir des enfants
	if (racine.firstChild().isNull()) {
		if (etat != NULL) *etat = 6;
		elmt_etat = 6;
		return;
	}
	
	// parcours des enfants de la definition
	int nb_elements_parses = 0;
	QPainter qp;
	qp.begin(&dessin);
	QPen t;
	t.setColor(Qt::black);
	t.setWidthF(1.0);
	t.setJoinStyle(Qt::MiterJoin);
	qp.setPen(t);
	for (QDomNode node = racine.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
		QDomElement elmts = node.toElement();
		if(elmts.isNull()) continue;
		if (parseElement(elmts, qp, s)) ++ nb_elements_parses;
		else {
			if (etat != NULL) *etat = 7;
			elmt_etat = 7;
			return;
		}
	}
	qp.end();
	
	// il doit y avoir au moins un element charge
	if (!nb_elements_parses) {
		if (etat != NULL) *etat = 8;
		elmt_etat = 8;
		return;
	}
	
	// fermeture du fichier
	fichier.close();
	
	if (etat != NULL) *etat = 0;
	elmt_etat = 0;
}

int ElementPerso::nbBornes() const {
	return(nb_bornes);
}

void ElementPerso::paint(QPainter *qp, const QStyleOptionGraphicsItem *) {
	dessin.play(qp);
}

bool ElementPerso::parseElement(QDomElement &e, QPainter &qp, Schema *s) {
	if (e.tagName() == "borne") return(parseBorne(e, s));
	else if (e.tagName() == "ligne") return(parseLigne(e, qp));
	else if (e.tagName() == "cercle") return(parseCercle(e, qp));
	else if (e.tagName() == "polygone") return(parsePolygone(e, qp));
	else return(true);	// on n'est pas chiant, on ignore l'element inconnu
}

bool ElementPerso::parseLigne(QDomElement &e, QPainter &qp) {
	// verifie la presence et la validite des attributs obligatoires
	int x1, y1, x2, y2;
	if (!attributeIsAnInteger(e, QString("x1"), &x1)) return(false);
	if (!attributeIsAnInteger(e, QString("y1"), &y1)) return(false);
	if (!attributeIsAnInteger(e, QString("x2"), &x2)) return(false);
	if (!attributeIsAnInteger(e, QString("y2"), &y2)) return(false);
	/// @todo : gerer l'antialiasing (mieux que ca !) et le type de trait
	setQPainterAntiAliasing(&qp, e.attribute("antialias") == "true");
	qp.drawLine(x1, y1, x2, y2);
	return(true);
}

bool ElementPerso::parseCercle(QDomElement &e, QPainter &qp) {
	// verifie la presence des attributs obligatoires
	int cercle_x, cercle_y, cercle_r;
	if (!attributeIsAnInteger(e, QString("x"),     &cercle_x)) return(false);
	if (!attributeIsAnInteger(e, QString("y"),     &cercle_y)) return(false);
	if (!attributeIsAnInteger(e, QString("rayon"), &cercle_r)) return(false);
	/// @todo : gerer l'antialiasing (mieux que ca !) et le type de trait
	setQPainterAntiAliasing(&qp, e.attribute("antialias") == "true");
	qp.drawEllipse(cercle_x, cercle_y, cercle_r, cercle_r);
	return(true);
}

bool ElementPerso::parsePolygone(QDomElement &e, QPainter &qp) {
	int i = 1;
	while(true) {
		if (attributeIsAnInteger(e, QString("x%1").arg(i)) && attributeIsAnInteger(e, QString("y%1").arg(i))) ++ i;
		else break;
	}
	if (i < 3) return(false);
	QPointF points[i-1];
	for (int j = 1 ; j < i ; ++ j) {
		points[j-1] = QPointF(
			e.attribute(QString("x%1").arg(j)).toDouble(),
			e.attribute(QString("y%1").arg(j)).toDouble()
		);
	}
	setQPainterAntiAliasing(&qp, e.attribute("antialias") == "true");
	qp.drawPolygon(points, i-1);
	return(true);
}

bool ElementPerso::parseBorne(QDomElement &e, Schema *s) {
	// verifie la presence et la validite des attributs obligatoires
	int bornex, borney;
	Borne::Orientation borneo;
	if (!attributeIsAnInteger(e, QString("x"), &bornex)) return(false);
	if (!attributeIsAnInteger(e, QString("y"), &borney)) return(false);
	if (!e.hasAttribute("orientation")) return(false);
	if (e.attribute("orientation") == "n") borneo = Borne::Nord;
	else if (e.attribute("orientation") == "s") borneo = Borne::Sud;
	else if (e.attribute("orientation") == "e") borneo = Borne::Est;
	else if (e.attribute("orientation") == "o") borneo = Borne::Ouest;
	else return(false);
	new Borne(bornex, borney, borneo, this, s);
	++ nb_bornes;
	return(true);
}

void ElementPerso::setQPainterAntiAliasing(QPainter *qp, bool aa) {
	qp -> setRenderHint(QPainter::Antialiasing,          aa);
	qp -> setRenderHint(QPainter::TextAntialiasing,      aa);
	qp -> setRenderHint(QPainter::SmoothPixmapTransform, aa);
}

int ElementPerso::attributeIsAnInteger(QDomElement &e, QString nom_attribut, int *entier) {
	// verifie la presence de l'attribut
	if (!e.hasAttribute(nom_attribut)) return(false);
	// verifie la validite de l'attribut
	bool ok;
	int tmp = e.attribute(nom_attribut).toInt(&ok);
	if (!ok) return(false);
	if (entier != NULL) *entier = tmp;
	return(true);
}

