#include "customelement.h"

/**
	Constructeur de la classe ElementPerso. Permet d'instancier un element
	utilisable comme un element fixe a la difference que l'element perso lit
	sa description (noms, dessin, comportement) dans un fichier XML a fournir
	en parametre.
	@param nom_fichier Le chemin du fichier XML decrivant l'element
	@param qgi Le QGraphicsItem parent de cet element
	@param s Le Schema affichant cet element
	@param etat Un pointeur facultatif vers un entier. La valeur de cet entier
	sera changee de maniere a refleter le deroulement de l'instanciation :
		- 0 : L'instanciation a reussi
		- 1 : Le fichier n'existe pas
		- 2 : Le fichier n'a pu etre ouvert
		- 3 : Le fichier n'est pas un document XML
		- 4 : Le document XML n'a pas une "definition" comme racine
		- 5 : Les attributs de la definition ne sont pas presents et / ou valides
		- 6 : La definition est vide
		- 7 : L'analyse d'un element XML decrivant une partie du dessin de l'element a echoue
		- 8 : Aucune partie du dessin n'a pu etre chargee
*/
CustomElement::CustomElement(QString &nom_fichier, QGraphicsItem *qgi, Diagram *s, int *etat) : FixedElement(qgi, s) {
	nomfichier = nom_fichier;
	nb_terminals = 0;
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
	
	//(new QGraphicsTextItem("plop", this, scene())) -> setTextInteractionFlags(Qt::TextEditorInteraction);
	if (etat != NULL) *etat = 0;
	elmt_etat = 0;
}

/**
	@return Le nombre de bornes que l'element possede
*/
int CustomElement::nbTerminals() const {
	return(nb_terminals);
}

/**
	Dessine le composant sur le Diagram
	@param qp Le QPainter a utiliser pour dessiner l'element
	@param qsogi Les options graphiques
*/
void CustomElement::paint(QPainter *qp, const QStyleOptionGraphicsItem *) {
	dessin.play(qp);
}

/**
	Analyse et prend en compte un element XML decrivant une partie du dessin
	de l'element perso. Si l'analyse reussit, la partie est ajoutee au dessin.
	Cette partie peut etre une borne, une ligne, une ellipse, un cercle, un arc
	de cercle ou un polygone. Cette methode renvoie false si l'analyse
	d'une de ces formes echoue. Si l'analyse reussit ou dans le cas d'une forme
	inconnue, cette methode renvoie true. A l'exception des bornes, toutes les
	formes peuvent avoir un attribut style. @see setPainterStyle
	@param e L'element XML a analyser
	@param qp Le QPainter a utiliser pour dessiner l'element perso
	@param s Le schema sur lequel sera affiche l'element perso
	@return true si l'analyse reussit, false sinon
*/
bool CustomElement::parseElement(QDomElement &e, QPainter &qp, Diagram *s) {
	if (e.tagName() == "terminal") return(parseTerminal(e, s));
	else if (e.tagName() == "line") return(parseLine(e, qp));
	else if (e.tagName() == "ellipse") return(parseEllipse(e, qp));
	else if (e.tagName() == "circle") return(parseCircle(e, qp));
	else if (e.tagName() == "arc") return(parseArc(e, qp));
	else if (e.tagName() == "polygon") return(parsePolygon(e, qp));
	else if (e.tagName() == "text") return(parseText(e, qp));
	else return(true);	// on n'est pas chiant, on ignore l'element inconnu
}

/**
	Analyse un element XML suppose representer une ligne. Si l'analyse
	reussit, la ligne est ajoutee au dessin.
	La ligne est definie par les attributs suivants :
		- x1, y1 : reels, coordonnees d'une extremite de la ligne
		- x2, y2 : reels, coordonnees de l'autre extremite de la ligne
		
	@param e L'element XML a analyser
	@param qp Le QPainter a utiliser pour dessiner l'element perso
	@return true si l'analyse reussit, false sinon
*/
bool CustomElement::parseLine(QDomElement &e, QPainter &qp) {
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

/**
	Analyse un element XML suppose representer un cercle. Si l'analyse
	reussit, le cercle est ajoute au dessin.
	Le cercle est defini par les attributs suivants :
		- x : abscisse du coin superieur gauche de la quadrature du cercle
		- y : ordonnee du coin superieur gauche de la quadrature du cercle
		- diameter : diametre du cercle
		
	@param e L'element XML a analyser
	@param qp Le QPainter a utiliser pour dessiner l'element perso
	@return true si l'analyse reussit, false sinon
	@todo utiliser des attributs plus coherents : x et y = centre, rayon = vrai rayon 
*/
bool CustomElement::parseCircle(QDomElement &e, QPainter &qp) {
	// verifie la presence des attributs obligatoires
	double cercle_x, cercle_y, cercle_r;
	if (!attributeIsAReal(e, QString("x"),        &cercle_x)) return(false);
	if (!attributeIsAReal(e, QString("y"),        &cercle_y)) return(false);
	if (!attributeIsAReal(e, QString("diameter"), &cercle_r)) return(false);
	qp.save();
	setPainterStyle(e, qp);
	qp.drawEllipse(QRectF(cercle_x, cercle_y, cercle_r, cercle_r));
	qp.restore();
	return(true);
}

/**
	Analyse un element XML suppose representer une ellipse. Si l'analyse
	reussit, l'ellipse est ajoutee au dessin.
	L'ellipse est definie par les attributs suivants :
		- x : abscisse du coin superieur gauche du rectangle dans lequel s'inscrit l'ellipse
		- y : ordonnee du coin superieur gauche du rectangle dans lequel s'inscrit l'ellipse
		- width : dimension de la diagonale horizontale de l'ellipse
		- height : dimension de la diagonale verticale de l'ellipse
		
	@param e L'element XML a analyser
	@param qp Le QPainter a utiliser pour dessiner l'element perso
	@return true si l'analyse reussit, false sinon
	@todo utiliser des attributs plus coherents : x et y = centre
*/
bool CustomElement::parseEllipse(QDomElement &e, QPainter &qp) {
	// verifie la presence des attributs obligatoires
	double ellipse_x, ellipse_y, ellipse_l, ellipse_h;
	if (!attributeIsAReal(e, QString("x"),       &ellipse_x))  return(false);
	if (!attributeIsAReal(e, QString("y"),       &ellipse_y))  return(false);
	if (!attributeIsAReal(e, QString("width"), &ellipse_l))  return(false);
	if (!attributeIsAReal(e, QString("height"), &ellipse_h))  return(false);
	qp.save();
	setPainterStyle(e, qp);
	qp.drawEllipse(QRectF(ellipse_x, ellipse_y, ellipse_l, ellipse_h));
	qp.restore();
	return(true);
}

/**
	Analyse un element XML suppose representer un arc de cercle. Si l'analyse
	reussit, l'arc de cercle est ajoute au dessin.
	L'arc de cercle est defini par les quatres parametres d'une ellipse (en fait
	l'ellipse dans laquelle s'inscrit l'arc de cercle) auxquels s'ajoutent les
	attributs suivants :
		- start : angle de depart : l'angle "0 degre" est a trois heures
		- angle : etendue (en degres) de l'arc de cercle ; une valeur positive
		va dans le sens contraire des aiguilles d'une montre
		
	@param e L'element XML a analyser
	@param qp Le QPainter a utiliser pour dessiner l'element perso
	@return true si l'analyse reussit, false sinon
*/
bool CustomElement::parseArc(QDomElement &e, QPainter &qp) {
	// verifie la presence des attributs obligatoires
	double arc_x, arc_y, arc_l, arc_h, arc_s, arc_a;
	if (!attributeIsAReal(e, QString("x"),       &arc_x))  return(false);
	if (!attributeIsAReal(e, QString("y"),       &arc_y))  return(false);
	if (!attributeIsAReal(e, QString("width"),   &arc_l))  return(false);
	if (!attributeIsAReal(e, QString("height"),  &arc_h))  return(false);
	if (!attributeIsAReal(e, QString("start"),   &arc_s))  return(false);
	if (!attributeIsAReal(e, QString("angle"),   &arc_a))  return(false);
	
	qp.save();
	setPainterStyle(e, qp);
	qp.drawArc(QRectF(arc_x, arc_y, arc_l, arc_h), (int)(arc_s * 16), (int)(arc_a * 16));
	qp.restore();
	return(true);
}

/**
	Analyse un element XML suppose representer un polygone. Si l'analyse
	reussit, le polygone est ajoute au dessin.
	Le polygone est defini par une serie d'attributs x1, x2, ..., xn et autant
	d'attributs y1, y2, ..., yn representant les coordonnees des differents
	points du polygone.
	@param e L'element XML a analyser
	@param qp Le QPainter a utiliser pour dessiner l'element perso
	@return true si l'analyse reussit, false sinon
*/
bool CustomElement::parsePolygon(QDomElement &e, QPainter &qp) {
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

/**
	Analyse un element XML suppose representer un texte. Si l'analyse
	reussit, le texte est ajoute au dessin.
	Le texte est defini par une position, une chaine de caractères et une
	taille.
	@param e L'element XML a analyser
	@param qp Le QPainter a utiliser pour dessiner l'element perso
	@return true si l'analyse reussit, false sinon
*/
bool CustomElement::parseText(QDomElement &e, QPainter &qp) {
	qreal pos_x, pos_y;
	int size;
	if (
		!attributeIsAReal(e, "x", &pos_x) ||\
		!attributeIsAReal(e, "y", &pos_y) ||\
		!attributeIsAnInteger(e, "size", &size) ||\
		!e.hasAttribute("text")
	) return(false);
	
	qp.save();
	setPainterStyle(e, qp);
	qp.setFont(QFont(QString("Sans Serif"), size));
	qp.drawText(QPointF(pos_x, pos_y), e.attribute("text"));
	qp.restore();
	return(true);
}

/**
	Analyse un element XML suppose representer une borne. Si l'analyse
	reussit, la borne est ajoutee a l'element.
	Une borne est definie par les attributs suivants :
		- x, y : coordonnees de la borne
		- orientation  : orientation de la borne = Nord (n), Sud (s), Est (e) ou Ouest (w)
		
	@param e L'element XML a analyser
	@param s Le schema sur lequel l'element perso sera affiche
	@return true si l'analyse reussit, false sinon
*/
bool CustomElement::parseTerminal(QDomElement &e, Diagram *s) {
	// verifie la presence et la validite des attributs obligatoires
	double terminalx, terminaly;
	Terminal::Orientation terminalo;
	if (!attributeIsAReal(e, QString("x"), &terminalx)) return(false);
	if (!attributeIsAReal(e, QString("y"), &terminaly)) return(false);
	if (!e.hasAttribute("orientation")) return(false);
	if (e.attribute("orientation") == "n") terminalo = Terminal::Nord;
	else if (e.attribute("orientation") == "s") terminalo = Terminal::Sud;
	else if (e.attribute("orientation") == "e") terminalo = Terminal::Est;
	else if (e.attribute("orientation") == "w") terminalo = Terminal::Ouest;
	else return(false);
	new Terminal(terminalx, terminaly, terminalo, this, s);
	++ nb_terminals;
	return(true);
}

/**
	Active / desactive l'antialiasing sur un QPainter
	@param qp Le QPainter a modifier
	@param aa Booleen a true pour activer l'antialiasing, a false pour le desactiver
*/
void CustomElement::setQPainterAntiAliasing(QPainter &qp, bool aa) {
	qp.setRenderHint(QPainter::Antialiasing,          aa);
	qp.setRenderHint(QPainter::TextAntialiasing,      aa);
	qp.setRenderHint(QPainter::SmoothPixmapTransform, aa);
}

/**
	Permet de savoir si l'attribut nom_attribut d'un element XML e est bien un
	entier. Si oui, sa valeur est copiee dans entier.
	@param e Element XML
	@param nom_attribut Nom de l'attribut a analyser
	@param entier Pointeur facultatif vers un entier
	@return true si l'attribut est bien un entier, false sinon
*/
bool CustomElement::attributeIsAnInteger(QDomElement &e, QString nom_attribut, int *entier) {
	// verifie la presence de l'attribut
	if (!e.hasAttribute(nom_attribut)) return(false);
	// verifie la validite de l'attribut
	bool ok;
	int tmp = e.attribute(nom_attribut).toInt(&ok);
	if (!ok) return(false);
	if (entier != NULL) *entier = tmp;
	return(true);
}

/**
	Permet de savoir si l'attribut nom_attribut d'un element XML e est bien un
	reel. Si oui, sa valeur est copiee dans reel.
	@param e Element XML
	@param nom_attribut Nom de l'attribut a analyser
	@param reel Pointeur facultatif vers un double
	@return true si l'attribut est bien un reel, false sinon
*/
bool CustomElement::attributeIsAReal(QDomElement &e, QString nom_attribut, double *reel) {
	// verifie la presence de l'attribut
	if (!e.hasAttribute(nom_attribut)) return(false);
	// verifie la validite de l'attribut
	bool ok;
	qreal tmp = e.attribute(nom_attribut).toDouble(&ok);
	if (!ok) return(false);
	if (reel != NULL) *reel = tmp;
	return(true);
}

/**
	Verifie si l'attribut "orientation" de l'element XML e correspond bien a la
	syntaxe decrivant les orientations possibles pour un element.
	Cette syntaxe comprend exactement 4 lettres :
		- une pour le Nord
		- une pour l'Est
		- une pour le Sud
		- une pour l'Ouest
	 
	Pour chaque orientation, on indique si elle est :
		- l'orientation par defaut : d
		- une orientation autorisee : y
		- une orientation interdire : n
		
	Exemple : "dnny" represente un element par defaut oriente vers le nord et qui
	peut etre oriente vers l'ouest mais pas vers le sud ou vers l'est.
	@param e Element XML
	@return true si l'attribut "orientation" est valide, false sinon
*/
bool CustomElement::validOrientationAttribute(QDomElement &e) {
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
	ori_d = (Terminal::Orientation)d_pos;
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
		- color : couleur du trait et du texte
			- white : trait noir [par defaut]
			- black : trait blanc
			
	Les autres valeurs ne sont pas prises en compte.
	@param e L'element XML a parser
	@param qp Le QPainter a modifier en fonction des styles
*/
void CustomElement::setPainterStyle(QDomElement &e, QPainter &qp) {
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
				else if (style_value == "none") pen.setColor(QColor(0, 0, 0, 0));
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
			} else if (style_name == "color") {
				if (style_value == "black") {
					pen.setColor(QColor(0, 0, 0, pen.color().alpha()));
				} else if (style_value == "white") {
					pen.setColor(QColor(255, 255, 255, pen.color().alpha()));
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
