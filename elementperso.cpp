#include "elementperso.h"

ElementPerso::ElementPerso(QString &nom_fichier, QGraphicsItem *qgi, Schema *s, int *etat) : ElementFixe(qgi, s) {
	nomfichier = nom_fichier;
	nb_bornes = 0;
	// pessimisme inside : par defaut, ca foire
	elmt_etat = -1;
	
	// le fichier doit exister
	QFileInfo infos_file(nomfichier);
	if (!infos_file.exists() || !infos_file.isFile()) {
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
		!attributeIsAnInteger(racine, QString("width"), &w) ||\
		!attributeIsAnInteger(racine, QString("height"), &h) ||\
		!attributeIsAnInteger(racine, QString("hotspot_x"), &hot_x) ||\
		!attributeIsAnInteger(racine, QString("hotspot_y"), &hot_y) ||\
		!validOrientationAttribute(racine)
	) {
		if (etat != NULL) *etat = 5;
		elmt_etat = 5;
		return;
	}
	
	// on peut d'ores et deja specifier la taille et le hotspot
	setSize(w, h);
	setHotspot(QPoint(hot_x, hot_y));
	setConnexionsInternesAcceptees(racine.attribute("ci") == "true");
	// la definition est supposee avoir des enfants
	if (racine.firstChild().isNull()) {
		if (etat != NULL) *etat = 6;
		elmt_etat = 6;
		return;
	}
	
	// initialisation du QPainter (pour dessiner l'element)
	QPainter qp;
	qp.begin(&dessin);
	QPen t;
	t.setColor(Qt::black);
	t.setWidthF(1.0);
	t.setJoinStyle(Qt::MiterJoin);
	qp.setPen(t);
	
	// recupere les deux premiers caracteres de la locale en cours du systeme
	QString system_language = QLocale::system().name().left(2);
	
	// au depart, le nom de l'element est celui du fichier le decrivant
	bool name_found = false;
	QString name_elmt = infos_file.baseName();
	
	// parcours des enfants de la definition : noms et parties du dessin
	int nb_elements_parses = 0;
	for (QDomNode node = racine.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
		QDomElement elmts = node.toElement();
		if (elmts.isNull()) continue;
		if (elmts.tagName() == "names") {
			// gestion des noms de l'element
			if (name_found) continue;
			for (QDomNode n = node.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
				QDomElement qde = n.toElement();
				if (qde.isNull() || qde.tagName() != "name" || !qde.hasAttribute("lang")) continue;
				if (qde.attribute("lang") == system_language) {
					name_elmt = qde.text();
					name_found = true;
					break;
				} else if (qde.attribute("lang") == "en") {
					name_elmt = qde.text();
				}
			}
		} else if (elmts.tagName() == "description") {
			// gestion de la description graphique de l'element
			//  = parcours des differentes parties du dessin
			for (QDomNode n = node.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
				QDomElement qde = n.toElement();
				if (qde.isNull()) continue;
				if (parseElement(qde, qp, s)) ++ nb_elements_parses;
				else {
					if (etat != NULL) *etat = 7;
					elmt_etat = 7;
					return;
				}
			}
		}
	}
	
	// on garde le nom trouve
	priv_nom = name_elmt;
	
	// fin du dessin
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
	else if (e.tagName() == "ellipse") return(parseEllipse(e, qp));
	else if (e.tagName() == "cercle") return(parseCercle(e, qp));
	else if (e.tagName() == "polygone") return(parsePolygone(e, qp));
	else return(true);	// on n'est pas chiant, on ignore l'element inconnu
}

bool ElementPerso::parseLigne(QDomElement &e, QPainter &qp) {
	// verifie la presence et la validite des attributs obligatoires
	double x1, y1, x2, y2;
	if (!attributeIsAReal(e, QString("x1"), &x1)) return(false);
	if (!attributeIsAReal(e, QString("y1"), &y1)) return(false);
	if (!attributeIsAReal(e, QString("x2"), &x2)) return(false);
	if (!attributeIsAReal(e, QString("y2"), &y2)) return(false);
	qp.save();
	setPainterStyle(e, qp);
	qp.drawLine(QLineF(x1, y1, x2, y2));
	qp.restore();
	return(true);
}

bool ElementPerso::parseCercle(QDomElement &e, QPainter &qp) {
	// verifie la presence des attributs obligatoires
	double cercle_x, cercle_y, cercle_r;
	if (!attributeIsAReal(e, QString("x"),     &cercle_x)) return(false);
	if (!attributeIsAReal(e, QString("y"),     &cercle_y)) return(false);
	if (!attributeIsAReal(e, QString("rayon"), &cercle_r)) return(false);
	qp.save();
	setPainterStyle(e, qp);
	qp.drawEllipse(QRectF(cercle_x, cercle_y, cercle_r, cercle_r));
	qp.restore();
	return(true);
}

bool ElementPerso::parseEllipse(QDomElement &e, QPainter &qp) {
	// verifie la presence des attributs obligatoires
	double ellipse_x, ellipse_y, ellipse_l, ellipse_h;
	if (!attributeIsAReal(e, QString("x"),       &ellipse_x))  return(false);
	if (!attributeIsAReal(e, QString("y"),       &ellipse_y))  return(false);
	if (!attributeIsAReal(e, QString("largeur"), &ellipse_l))  return(false);
	if (!attributeIsAReal(e, QString("hauteur"), &ellipse_h))  return(false);
	qp.save();
	setPainterStyle(e, qp);
	qp.drawEllipse(QRectF(ellipse_x, ellipse_y, ellipse_l, ellipse_h));
	qp.restore();
	return(true);
}

bool ElementPerso::parsePolygone(QDomElement &e, QPainter &qp) {
	int i = 1;
	while(true) {
		if (attributeIsAReal(e, QString("x%1").arg(i)) && attributeIsAReal(e, QString("y%1").arg(i))) ++ i;
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
	qp.save();
	setPainterStyle(e, qp);
	qp.drawPolygon(points, i-1);
	qp.restore();
	return(true);
}

bool ElementPerso::parseBorne(QDomElement &e, Schema *s) {
	// verifie la presence et la validite des attributs obligatoires
	double bornex, borney;
	Borne::Orientation borneo;
	if (!attributeIsAReal(e, QString("x"), &bornex)) return(false);
	if (!attributeIsAReal(e, QString("y"), &borney)) return(false);
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

void ElementPerso::setQPainterAntiAliasing(QPainter &qp, bool aa) {
	qp.setRenderHint(QPainter::Antialiasing,          aa);
	qp.setRenderHint(QPainter::TextAntialiasing,      aa);
	qp.setRenderHint(QPainter::SmoothPixmapTransform, aa);
}

bool ElementPerso::attributeIsAnInteger(QDomElement &e, QString nom_attribut, int *entier) {
	// verifie la presence de l'attribut
	if (!e.hasAttribute(nom_attribut)) return(false);
	// verifie la validite de l'attribut
	bool ok;
	int tmp = e.attribute(nom_attribut).toInt(&ok);
	if (!ok) return(false);
	if (entier != NULL) *entier = tmp;
	return(true);
}

bool ElementPerso::attributeIsAReal(QDomElement &e, QString nom_attribut, double *reel) {
	// verifie la presence de l'attribut
	if (!e.hasAttribute(nom_attribut)) return(false);
	// verifie la validite de l'attribut
	bool ok;
	qreal tmp = e.attribute(nom_attribut).toDouble(&ok);
	if (!ok) return(false);
	if (reel != NULL) *reel = tmp;
	return(true);
}

bool ElementPerso::validOrientationAttribute(QDomElement &e) {
	// verifie la presence de l'attribut orientation
	if (!e.hasAttribute("orientation")) return(false);
	QString t = e.attribute("orientation");
	// verification syntaxique : 4 lettres, un d, que des y ou des n pour le reste
	if (t.length() != 4) return(false);
	int d_pos = -1;
	for (int i = 0 ; i < 4 ; ++ i) {
		QChar c = t.at(i);
		if (c != 'd' && c != 'y' && c != 'n') return(false);
		if (c == 'd') {
			if (d_pos == -1) d_pos = i;
			else return(false);
		}
	}
	if (d_pos == -1) return(false);
	
	// orientation : 4 lettres = nord/est/sud/ouest avec d = default, y = yes et n = no
	ori_n = (t.at(0) == 'd' || t.at(0) == 'y');
	ori_e = (t.at(1) == 'd' || t.at(1) == 'y');
	ori_s = (t.at(2) == 'd' || t.at(2) == 'y');
	ori_w = (t.at(3) == 'd' || t.at(3) == 'y');
	ori_d = (Borne::Orientation)d_pos;
	ori = ori_d;
	return(true);
}

/**
	Applique les parametres de style definis dans l'attribut "style" de
	l'element XML e au QPainter qp
	Les styles possibles sont :
		- line-style : style du trait
			- dashed : trait en pointilles
			- normal : trait plein [par defaut]
		- line-weight : epaiseur du trait
			- thin : trait fin
			- normal : trait d'epaisseur 1 [par defaut]
		- filling : remplissage de la forme
			- white : remplissage blanc
			- black : remplissage noir
			- none : pas de remplissage [par defaut]
	Les autres valeurs ne sont pas prises en compte.
	@param e L'element XML a parser
	@param qp Le QPainter a modifier en fonction des styles
*/
void ElementPerso::setPainterStyle(QDomElement &e, QPainter &qp) {
	// recupere le QPen et la QBrush du QPainter
	QPen pen = qp.pen();
	QBrush brush = qp.brush();
	
	// attributs par defaut
	pen.setJoinStyle(Qt::MiterJoin);
	pen.setCapStyle(Qt::SquareCap);
	pen.setColor(Qt::black);
	pen.setStyle(Qt::SolidLine);
	pen.setWidthF(1.0);
	brush.setStyle(Qt::NoBrush);
	
	// recupere la liste des couples style / valeur
	QStringList styles = e.attribute("style").split(";", QString::SkipEmptyParts);
	
	// agit sur le QPen et la QBrush en fonction des valeurs rencontrees
	QRegExp rx("^\\s*([a-z-]+)\\s*:\\s*([a-z-]+)\\s*$");
	foreach (QString style, styles) {
		if (rx.exactMatch(style)) {
			QString style_name = rx.cap(1);
			QString style_value = rx.cap(2);
			if (style_name == "line-style") {
				if (style_value == "dashed") pen.setStyle(Qt::DashLine);
				else if (style_value == "normal") pen.setStyle(Qt::SolidLine);
			} else if (style_name == "line-weight") {
				if (style_value == "thin") pen.setWidth(0);
				else if (style_value == "normal") pen.setWidthF(1.0);
			} else if (style_name == "filling") {
				if (style_value == "white") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::white);
				} else if (style_value == "black") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::black);
				} else if (style_value == "none") {
					brush.setStyle(Qt::NoBrush);
				}
			}
		}
	}
	/*line-style:dashed;
	if (e.attribute("style") == "dashed") {
		
		pen.setStyle(Qt::DashLine);
		
	}*/
	
	// affectation du QPen et de la QBrush modifies au QPainter 
	qp.setPen(pen);
	qp.setBrush(brush);
	
	// mise en place (ou non) de l'antialiasing
	setQPainterAntiAliasing(qp, e.attribute("antialias") == "true");
}
