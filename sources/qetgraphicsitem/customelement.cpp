/*
	Copyright 2006-2016 The QElectroTech Team
	This file is part of QElectroTech.
	
	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	
	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "customelement.h"
#include "elementtextitem.h"
#include "diagram.h"
#include "qetapp.h"
#include "partline.h"
#include "elementdefinition.h"
#include <iostream>
#include "terminal.h"
#include "diagramposition.h"

/**
	Constructeur de la classe CustomElement. Permet d'instancier un element
	utilisable comme un element fixe a la difference que l'element perso est
	construit a partir d'une description au format XML. Celle-ci est recuperee
	a l'emplacement indique.
	@param location Emplacement de la definition d'element a utiliser
	@param qgi Le QGraphicsItem parent de cet element
	@param s Le Schema affichant cet element
	@param state Un pointeur facultatif vers un entier. La valeur de cet entier
	sera changee de maniere a refleter le deroulement de l'instanciation :
		- 0 : L'instanciation a reussi
		- 1 : l'emplacement n'a pas permis d'acceder a une definition d'element
		- 2 : la definition n'etait pas lisible
		- 3 : la definition n'etait pas valide / exploitable / utilisable
		- 4 : Le document XML n'est pas un element "definition"
		- 5 : Les attributs de la definition ne sont pas presents et / ou valides
		- 6 : La definition est vide
		- 7 : L'analyse d'un element XML decrivant une partie du dessin de l'element a echoue
		- 8 : Aucune partie du dessin n'a pu etre chargee
*/
CustomElement::CustomElement(const ElementsLocation &location, QGraphicsItem *qgi, int *state) :
	FixedElement(qgi),
	location_(location),
	forbid_antialiasing(false)
{

	if(Q_UNLIKELY( !(location.isElement() && location.exist()) ))
	{
		if (state) *state = 1;
		return;
	}

		//Start from empty lists.
	list_lines_.clear();
	list_rectangles_.clear();
	list_circles_.clear();
	list_polygons_.clear();
	list_arcs_.clear();

	int elmt_state;
	buildFromXml(location.xml(), &elmt_state);
	if (state) *state = elmt_state;
	if (elmt_state) return;
	
	if (state) *state = 0;
}

/**
	Construit l'element personnalise a partir d'un element XML representant sa
	definition.
	@param xml_def_elmt
	@param state Un pointeur facultatif vers un entier. La valeur de cet entier
	sera changee de maniere a refleter le deroulement de l'instanciation :
		- 0 : La construction s'est bien passee
		- 4 : Le document XML n'est pas un element "definition"
		- 5 : Les attributs de la definition ne sont pas presents et / ou valides
		- 6 : La definition est vide
		- 7 : L'analyse d'un element XML decrivant une partie du dessin de l'element a echoue
		- 8 : Aucune partie du dessin n'a pu etre chargee
	@return true si le chargement a reussi, false sinon
*/
bool CustomElement::buildFromXml(const QDomElement &xml_def_elmt, int *state) {
	
	if (xml_def_elmt.tagName() != "definition" || xml_def_elmt.attribute("type") != "element")
	{
		if (state) *state = 4;
		return(false);
	}
	
	// verifie basiquement que la version actuelle est capable de lire ce fichier
	if (xml_def_elmt.hasAttribute("version")) {
		bool conv_ok;
		qreal element_version = xml_def_elmt.attribute("version").toDouble(&conv_ok);
		if (conv_ok && QET::version.toDouble() < element_version) {
			std::cerr << qPrintable(
				QObject::tr("Avertissement : l'élément "
				" a été enregistré avec une version"
				" ultérieure de QElectroTech.")
			) << std::endl;
		}
	}
	
	// ces attributs doivent etre presents et valides
	int w, h, hot_x, hot_y;
	if (
		!QET::attributeIsAnInteger(xml_def_elmt, QString("width"), &w) ||\
		!QET::attributeIsAnInteger(xml_def_elmt, QString("height"), &h) ||\
		!QET::attributeIsAnInteger(xml_def_elmt, QString("hotspot_x"), &hot_x) ||\
		!QET::attributeIsAnInteger(xml_def_elmt, QString("hotspot_y"), &hot_y) ||\
		!validOrientationAttribute(xml_def_elmt)
	) {
		if (state) *state = 5;
		return(false);
	}
	
	setSize(w, h);
	setHotspot(QPoint(hot_x, hot_y));
	
		//the definition must have childs
	if (xml_def_elmt.firstChild().isNull())
	{
		if (state) *state = 6;
		return(false);
	}
	
	// initialisation du QPainter (pour dessiner l'element)
	QPainter qp;
	qp.begin(&drawing);
	
	QPainter low_zoom_qp;
	low_zoom_qp.begin(&low_zoom_drawing);
	QPen tmp;
	tmp.setWidthF(1.0); // ligne vaudou pour prise en compte du setCosmetic - ne pas enlever
	tmp.setCosmetic(true);
	low_zoom_qp.setPen(tmp);
	
	// extrait les noms de la definition XML
	names.fromXml(xml_def_elmt);
	setToolTip(name());

	//load kind informations
	kind_informations_.fromXml(xml_def_elmt.firstChildElement("kindInformations"), "kindInformation");
	
	// parcours des enfants de la definition : parties du dessin
	int parsed_elements_count = 0;
	for (QDomNode node = xml_def_elmt.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
		QDomElement elmts = node.toElement();
		if (elmts.isNull()) continue;
		if (elmts.tagName() == "description") {
			// gestion de la description graphique de l'element
			//  = parcours des differentes parties du dessin
			for (QDomNode n = node.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
				QDomElement qde = n.toElement();
				if (qde.isNull()) continue;
				if (parseElement(qde, qp)) {
					++ parsed_elements_count;
					QString current_tag = qde.tagName();
					if (current_tag != "terminal" && current_tag != "input") {
						forbid_antialiasing = true;
						parseElement(qde, low_zoom_qp);
						forbid_antialiasing = false;
					}
				} else {
					if (state) *state = 7;
					return(false);
				}
			}
		}
	}
	
	// fin du dessin
	qp.end();
	low_zoom_qp.end();
	
	// il doit y avoir au moins un element charge
	if (!parsed_elements_count) {
		if (state) *state = 8;
		return(false);
	} else {
		if (state) *state = 0;
		return(true);
	}
}

/**
	Destructeur
*/
CustomElement::~CustomElement() {
	qDeleteAll (list_lines_);
	qDeleteAll (list_rectangles_);
	qDeleteAll (list_circles_);
	qDeleteAll (list_polygons_);
	qDeleteAll (list_arcs_);
	qDeleteAll (list_texts_);
	qDeleteAll (list_terminals);
}

/// @return la liste des bornes de cet element
QList<Terminal *> CustomElement::terminals() const {
	return(list_terminals);
}

/// @return la liste des conducteurs rattaches a cet element
QList<Conductor *> CustomElement::conductors() const {
	QList<Conductor *> conductors;
	foreach(Terminal *t, list_terminals) conductors << t -> conductors();
	return(conductors);
}

/// @return la liste des textes de cet element
QList<ElementTextItem *> CustomElement::texts() const {
	return(list_texts_);
}

/// @return the list of lines
QList<QLineF *> CustomElement::lines() const {
	return(list_lines_);
}

/// @return the list of rectangles
QList<QRectF *> CustomElement::rectangles() const {
	return(list_rectangles_);
}

/// @return the list of bounding rectangles for circles
QList<QRectF *> CustomElement::circles() const {
	return(list_circles_);
}

/// @return the list of bounding rectangles for circles
QList<QVector<QPointF> *> CustomElement::polygons() const {
	return(list_polygons_);
}

/// @return the list of arcs
QList<QVector<qreal> *> CustomElement::arcs() const {
	return(list_arcs_);
}

/**
	@return Le nombre de bornes que l'element possede
*/
int CustomElement::terminalsCount() const {
	return(list_terminals.size());
}

/**
	Dessine le composant sur le Diagram
	@param qp Le QPainter a utiliser pour dessiner l'element
	@param options Les options graphiques
*/
void CustomElement::paint(QPainter *qp, const QStyleOptionGraphicsItem *options) {
	if (options && options -> levelOfDetail < 1.0) {
		low_zoom_drawing.play(qp);
	} else {
		drawing.play(qp);
	}
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
	@return true si l'analyse reussit, false sinon
*/
bool CustomElement::parseElement(QDomElement &e, QPainter &qp) {
	if (e.tagName() == "terminal") return(parseTerminal(e));
	else if (e.tagName() == "line") return(parseLine(e, qp));
	else if (e.tagName() == "rect") return(parseRect(e, qp));
	else if (e.tagName() == "ellipse") return(parseEllipse(e, qp));
	else if (e.tagName() == "circle") return(parseCircle(e, qp));
	else if (e.tagName() == "arc") return(parseArc(e, qp));
	else if (e.tagName() == "polygon") return(parsePolygon(e, qp));
	else if (e.tagName() == "text") return(parseText(e, qp));
	else if (e.tagName() == "input") return(parseInput(e));
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
	qreal x1, y1, x2, y2;
	if (!QET::attributeIsAReal(e, QString("x1"), &x1)) return(false);
	if (!QET::attributeIsAReal(e, QString("y1"), &y1)) return(false);
	if (!QET::attributeIsAReal(e, QString("x2"), &x2)) return(false);
	if (!QET::attributeIsAReal(e, QString("y2"), &y2)) return(false);
	
	Qet::EndType first_end = Qet::endTypeFromString(e.attribute("end1"));
	Qet::EndType second_end = Qet::endTypeFromString(e.attribute("end2"));
	qreal length1, length2;
	if (!QET::attributeIsAReal(e, QString("length1"), &length1)) length1 = 1.5;
	if (!QET::attributeIsAReal(e, QString("length2"), &length2)) length2 = 1.5;
	
	qp.save();
	setPainterStyle(e, qp);
	QPen t = qp.pen();
	t.setJoinStyle(Qt::MiterJoin);
	qp.setPen(t);
	
	QLineF line(x1, y1, x2, y2);

	//Add line to the list
	QLineF *newLine = new QLineF(line);
	list_lines_ << newLine;

	QPointF point1(line.p1());
	QPointF point2(line.p2());
	
	qreal line_length(line.length());
	qreal pen_width = qp.pen().widthF();
	
	// determine s'il faut dessiner les extremites
	bool draw_1st_end, draw_2nd_end;
	qreal reduced_line_length = line_length - (length1 * PartLine::requiredLengthForEndType(first_end));
	draw_1st_end = first_end && reduced_line_length >= 0;
	if (draw_1st_end) {
		reduced_line_length -= (length2 * PartLine::requiredLengthForEndType(second_end));
	} else {
		reduced_line_length = line_length - (length2 * PartLine::requiredLengthForEndType(second_end));
	}
	draw_2nd_end = second_end && reduced_line_length >= 0;
	
	// dessine la premiere extremite
	QPointF start_point, stop_point;
	if (draw_1st_end) {
		QList<QPointF> four_points1(PartLine::fourEndPoints(point1, point2, length1));
		if (first_end == Qet::Circle) {
			qp.drawEllipse(QRectF(four_points1[0] - QPointF(length1, length1), QSizeF(length1 * 2.0, length1 * 2.0)));
			start_point = four_points1[1];
		} else if (first_end == Qet::Diamond) {
			qp.drawPolygon(QPolygonF() << four_points1[1] << four_points1[2] << point1 << four_points1[3]);
			start_point = four_points1[1];
		} else if (first_end == Qet::Simple) {
			qp.drawPolyline(QPolygonF() << four_points1[3] << point1 << four_points1[2]);
			start_point = point1;
			
		} else if (first_end == Qet::Triangle) {
			qp.drawPolygon(QPolygonF() << four_points1[0] << four_points1[2] << point1 << four_points1[3]);
			start_point = four_points1[0];
		}
		
		// ajuste le depart selon l'epaisseur du trait
		if (pen_width && (first_end == Qet::Simple || first_end == Qet::Circle)) {
			start_point = QLineF(start_point, point2).pointAt(pen_width / 2.0 / line_length);
		}
	} else {
		start_point = point1;
	}
	
	// dessine la seconde extremite
	if (draw_2nd_end) {
		QList<QPointF> four_points2(PartLine::fourEndPoints(point2, point1, length2));
		if (second_end == Qet::Circle) {
			qp.drawEllipse(QRectF(four_points2[0] - QPointF(length2, length2), QSizeF(length2 * 2.0, length2 * 2.0)));
			stop_point = four_points2[1];
		} else if (second_end == Qet::Diamond) {
			qp.drawPolygon(QPolygonF() << four_points2[2] << point2 << four_points2[3] << four_points2[1]);
			stop_point = four_points2[1];
		} else if (second_end == Qet::Simple) {
			qp.drawPolyline(QPolygonF() << four_points2[3] << point2 << four_points2[2]);
			stop_point = point2;
		} else if (second_end == Qet::Triangle) {
			qp.drawPolygon(QPolygonF() << four_points2[0] << four_points2[2] << point2 << four_points2[3] << four_points2[0]);
			stop_point = four_points2[0];
		}
		
		// ajuste l'arrivee selon l'epaisseur du trait
		if (pen_width && (second_end == Qet::Simple || second_end == Qet::Circle)) {
			stop_point = QLineF(point1, stop_point).pointAt((line_length - (pen_width / 2.0)) / line_length);
		}
	} else {
		stop_point = point2;
	}
	
	qp.drawLine(start_point, stop_point);
	
	qp.restore();
	return(true);
}

/**
	Analyse un element XML suppose representer un rectangle. Si l'analyse
	reussit, le rectangle est ajoute au dessin.
	Le rectangle est defini par les attributs suivants :
		- x : abscisse du coin superieur gauche du rectangle
		- y : ordonnee du coin superieur gauche du rectangle
		- width : largeur du rectangle
		- height : hauteur du rectangle
		
	@param e L'element XML a analyser
	@param qp Le QPainter a utiliser pour dessiner l'element perso
	@return true si l'analyse reussit, false sinon
*/
bool CustomElement::parseRect(QDomElement &e, QPainter &qp) {
	// verifie la presence des attributs obligatoires
	qreal rect_x, rect_y, rect_w, rect_h;
	if (!QET::attributeIsAReal(e, QString("x"),       &rect_x))  return(false);
	if (!QET::attributeIsAReal(e, QString("y"),       &rect_y))  return(false);
	if (!QET::attributeIsAReal(e, QString("width"),   &rect_w))  return(false);
	if (!QET::attributeIsAReal(e, QString("height"),  &rect_h))  return(false);

	//Add rectangle to the list
	QRectF *rect = new QRectF(rect_x, rect_y, rect_w, rect_h);
	list_rectangles_ << rect;

	qp.save();
	setPainterStyle(e, qp);
	
	// force le type de jointures pour les rectangles
	QPen p = qp.pen();
	p.setJoinStyle(Qt::MiterJoin);
	qp.setPen(p);
	
	qp.drawRect(QRectF(rect_x, rect_y, rect_w, rect_h));
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
*/
bool CustomElement::parseCircle(QDomElement &e, QPainter &qp) {
	// verifie la presence des attributs obligatoires
	qreal cercle_x, cercle_y, cercle_r;
	if (!QET::attributeIsAReal(e, QString("x"),        &cercle_x)) return(false);
	if (!QET::attributeIsAReal(e, QString("y"),        &cercle_y)) return(false);
	if (!QET::attributeIsAReal(e, QString("diameter"), &cercle_r)) return(false);
	qp.save();
	setPainterStyle(e, qp);
	QRectF circle_bounding_rect(cercle_x, cercle_y, cercle_r, cercle_r);

	// Add circle to list
	QRectF *circle = new QRectF(circle_bounding_rect);
	list_circles_ << circle;

	qp.drawEllipse(circle_bounding_rect);
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
*/
bool CustomElement::parseEllipse(QDomElement &e, QPainter &qp) {
	// verifie la presence des attributs obligatoires
	qreal ellipse_x, ellipse_y, ellipse_l, ellipse_h;
	if (!QET::attributeIsAReal(e, QString("x"),       &ellipse_x))  return(false);
	if (!QET::attributeIsAReal(e, QString("y"),       &ellipse_y))  return(false);
	if (!QET::attributeIsAReal(e, QString("width"), &ellipse_l))  return(false);
	if (!QET::attributeIsAReal(e, QString("height"), &ellipse_h))  return(false);
	qp.save();
	setPainterStyle(e, qp);

	QVector<qreal> *arc = new QVector<qreal>;
	arc -> push_back(ellipse_x);
	arc -> push_back(ellipse_y);
	arc -> push_back(ellipse_l);
	arc -> push_back(ellipse_h);
	arc -> push_back(0);
	arc -> push_back(360);
	list_arcs_ << arc;

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
	qreal arc_x, arc_y, arc_l, arc_h, arc_s, arc_a;
	if (!QET::attributeIsAReal(e, QString("x"),       &arc_x))  return(false);
	if (!QET::attributeIsAReal(e, QString("y"),       &arc_y))  return(false);
	if (!QET::attributeIsAReal(e, QString("width"),   &arc_l))  return(false);
	if (!QET::attributeIsAReal(e, QString("height"),  &arc_h))  return(false);
	if (!QET::attributeIsAReal(e, QString("start"),   &arc_s))  return(false);
	if (!QET::attributeIsAReal(e, QString("angle"),   &arc_a))  return(false);
	
	qp.save();
	setPainterStyle(e, qp);

	QVector<qreal> *arc = new QVector<qreal>;
	arc -> push_back(arc_x);
	arc -> push_back(arc_y);
	arc -> push_back(arc_l);
	arc -> push_back(arc_h);
	arc -> push_back(arc_s);
	arc -> push_back(arc_a);
	list_arcs_ << arc;

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
	Il est possible d'obtenir un polygone non ferme en utilisant closed="false"
	@param e L'element XML a analyser
	@param qp Le QPainter a utiliser pour dessiner l'element perso
	@return true si l'analyse reussit, false sinon
*/
bool CustomElement::parsePolygon(QDomElement &e, QPainter &qp) {
	int i = 1;
	while(true) {
		if (QET::attributeIsAReal(e, QString("x%1").arg(i)) && QET::attributeIsAReal(e, QString("y%1").arg(i))) ++ i;
		else break;
	}
	if (i < 3) return(false);
	QVector<QPointF> points; // empty vector created instead of default initialized vector with i-1 elements.
	for (int j = 1 ; j < i ; ++ j) {
		points.insert(
			j - 1,
			QPointF(
				e.attribute(QString("x%1").arg(j)).toDouble(),
				e.attribute(QString("y%1").arg(j)).toDouble()
			)
		);
	}

	qp.save();
	setPainterStyle(e, qp);
	if (e.attribute("closed") == "false") qp.drawPolyline(points.data(), i-1);
	else {
		qp.drawPolygon(points.data(), i-1);

		// insert first point at the end again for DXF export.
		points.push_back(points[0]);
	}

	// Add to list of polygons.
	QVector<QPointF> *poly = new QVector<QPointF>(points);
	list_polygons_ << poly;

	qp.restore();
	return(true);
}

/**
	Analyse un element XML suppose representer un texte. Si l'analyse
	reussit, le texte est ajoute au dessin.
	Le texte est defini par une position, une chaine de caracteres et une
	taille.
	@param e L'element XML a analyser
	@param qp Le QPainter a utiliser pour dessiner l'element perso
	@return true si l'analyse reussit, false sinon
*/
bool CustomElement::parseText(QDomElement &e, QPainter &qp) {
	qreal pos_x, pos_y;
	int size;
	if (
		!QET::attributeIsAReal(e, "x", &pos_x) ||\
		!QET::attributeIsAReal(e, "y", &pos_y) ||\
		!QET::attributeIsAnInteger(e, "size", &size) ||\
		!e.hasAttribute("text")
	) return(false);
	
	qp.save();
	setPainterStyle(e, qp);
	
	// determine la police a utiliser et en recupere les metriques associees
	QFont used_font = QETApp::diagramTextsFont(size);
	QFontMetrics qfm(used_font);
	QColor text_color = (e.attribute("color") != "white"? Qt::black : Qt::white);
	
	// instancie un QTextDocument (comme la classe QGraphicsTextItem) pour
	// generer le rendu graphique du texte
	QTextDocument text_document;
	text_document.setDefaultFont(used_font);
	text_document.setPlainText(e.attribute("text"));

	//Add element to list of texts.
	ElementTextItem *eti = new ElementTextItem(e.attribute("text"));
	eti -> setFont(QETApp::diagramTextsFont(size));
	eti -> setOriginalPos(QPointF(pos_x, pos_y));
	eti -> setPos(pos_x, pos_y);
	qreal original_rotation_angle = 0.0;
	QET::attributeIsAReal(e, "rotation", &original_rotation_angle);
	eti -> setOriginalRotationAngle(original_rotation_angle);
	eti -> setRotationAngle(original_rotation_angle);
	eti -> setFollowParentRotations(e.attribute("rotate") == "true");
	list_texts_ << eti;
	
	// Se positionne aux coordonnees indiquees dans la description du texte	
	qp.setTransform(QTransform(), false);
	qp.translate(pos_x, pos_y);
	
	// Pivote le systeme de coordonnees du QPainter pour effectuer le rendu
	// dans le bon sens
	qreal default_rotation_angle = 0.0;
	if (QET::attributeIsAReal(e, "rotation", &default_rotation_angle)) {
		qp.rotate(default_rotation_angle);
	}
	
	/*
		Deplace le systeme de coordonnees du QPainter pour effectuer le rendu au
		bon endroit ; note : on soustrait l'ascent() de la police pour
		determiner le coin superieur gauche du texte alors que la position
		indiquee correspond a la baseline.
	*/
	QPointF qpainter_offset(0.0, -qfm.ascent());
	
		//adjusts the offset by the margin of the text document
	text_document.setDocumentMargin(0.0);
	
	qp.translate(qpainter_offset);
	
	// force the palette used to render the QTextDocument
	QAbstractTextDocumentLayout::PaintContext ctx;
	ctx.palette.setColor(QPalette::Text, text_color);
	text_document.documentLayout() -> draw(&qp, ctx);

	qp.restore();
	return(true);
}

/**
	Analyse un element XML suppose representer un champ de texte editable par
	l'utilisateur. Si l'analyse reussit, le champ est ajoute au dessin.
	Le texte est defini par :
		- une position
		- une chaine de caracteres facultative utilisee comme valeur par defaut
		- une taille
		- le fait de subir les rotations de l'element ou non
	@param e L'element XML a analyser
	@return Un pointeur vers l'objet ElementTextItem ainsi cree si l'analyse reussit, 0 sinon
*/
ElementTextItem *CustomElement::parseInput(QDomElement &e) {
	qreal pos_x, pos_y;
	int size;
	if (
		!QET::attributeIsAReal(e, "x", &pos_x) ||\
		!QET::attributeIsAReal(e, "y", &pos_y) ||\
		!QET::attributeIsAnInteger(e, "size", &size)
	) return(0);
	
	ElementTextItem *eti = new ElementTextItem(e.attribute("text"), this);
	eti -> setFont(QETApp::diagramTextsFont(size));
	eti -> setTagg(e.attribute("tagg", "other"));
	
	// position the text field
	eti -> setOriginalPos(QPointF(pos_x, pos_y));
	eti -> setPos(pos_x, pos_y);
	
	// rotation of the text field
	qreal original_rotation_angle = 0.0;
	QET::attributeIsAReal(e, "rotation", &original_rotation_angle);
	eti -> setOriginalRotationAngle(original_rotation_angle);
	eti -> setRotationAngle(original_rotation_angle);
	
	// behavior when the parent element is rotated
	eti -> setFollowParentRotations(e.attribute("rotate") == "true");
	
	list_texts_ << eti;
	
	return(eti);
}

/**
	Analyse un element XML suppose representer une borne. Si l'analyse
	reussit, la borne est ajoutee a l'element.
	Une borne est definie par les attributs suivants :
		- x, y : coordonnees de la borne
		- orientation  : orientation de la borne = Nord (n), Sud (s), Est (e) ou Ouest (w)
		
	@param e L'element XML a analyser
	@return Un pointeur vers l'objet Terminal ainsi cree, 0 sinon
*/
Terminal *CustomElement::parseTerminal(QDomElement &e) {
	// verifie la presence et la validite des attributs obligatoires
	qreal terminalx, terminaly;
	Qet::Orientation terminalo;
	if (!QET::attributeIsAReal(e, QString("x"), &terminalx)) return(0);
	if (!QET::attributeIsAReal(e, QString("y"), &terminaly)) return(0);
	if (!e.hasAttribute("orientation")) return(0);
	if (e.attribute("orientation") == "n") terminalo = Qet::North;
	else if (e.attribute("orientation") == "s") terminalo = Qet::South;
	else if (e.attribute("orientation") == "e") terminalo = Qet::East;
	else if (e.attribute("orientation") == "w") terminalo = Qet::West;
	else return(0);
	Terminal *new_terminal = new Terminal(terminalx, terminaly, terminalo, this);
	new_terminal -> setZValue(420); // valeur arbitraire pour maintenir les bornes au-dessus des champs de texte
	list_terminals << new_terminal;
	return(new_terminal);
}

/**
	Active / desactive l'antialiasing sur un QPainter
	@param qp Le QPainter a modifier
	@param aa Booleen a true pour activer l'antialiasing, a false pour le desactiver
*/
void CustomElement::setQPainterAntiAliasing(QPainter &qp, bool aa) {
	if (forbid_antialiasing) aa = false;
	qp.setRenderHint(QPainter::Antialiasing,          aa);
	qp.setRenderHint(QPainter::TextAntialiasing,      aa);
	qp.setRenderHint(QPainter::SmoothPixmapTransform, aa);
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
bool CustomElement::validOrientationAttribute(const QDomElement &e) {
	int ori = e.attribute("orientation").toInt();
	if(ori >= 0 && ori <=3) return true;
	return false;
}

/**
	Applique les parametres de style definis dans l'attribut "style" de
	l'element XML e au QPainter qp
	Les styles possibles sont :
		- line-style : style du trait
			- dashed : trait en pointilles (tirets)
			- dashdotted : Traits et points
			- dotted : trait en pointilles (points)
			- normal : trait plein [par defaut]
		- line-weight : epaiseur du trait
			- thin : trait fin
			- normal : trait d'epaisseur 1 [par defaut]
			- none : trait invisible
			- forte : trait d'epaisseur 2
			- eleve : trait d'epaisseur 5
		- filling : remplissage de la forme
			- white : remplissage blanc
			- black : remplissage noir
			 - red   : remplissage rouge
			- blue  : remplissage bleu
			- green : remplissage vert
			- gray : remplissage gris
			- brun : remplissage marron
			- yellow : remplissage jaune
			- cyan : remplissage cyan
			- lightgray : remplissage gris clair
			- orange : remplissage orange
			- purple : remplissage violet
			- none : pas de remplissage [par defaut]
		- color : couleur du trait et du texte
			- white : trait noir [par defaut]
			- black : trait blanc
			- red   : trait rouge
			- blue  : trait bleu
			- green : trait vert
			- gray : trait gris
			- brun : trait marron
			- yellow : trait jaune
			- cyan : trait cyan
			- lightgray : trait gris clair
			- orange : trait orange
			- purple : trait violet
			- lignes Horizontales
			- lignes Verticales
			- hachures gauche
			- hachures  droite
			- none : pas de contour
			
	Les autres valeurs ne sont pas prises en compte.
	@param e L'element XML a parser
	@param qp Le QPainter a modifier en fonction des styles
*/
void CustomElement::setPainterStyle(QDomElement &e, QPainter &qp) {
	// recupere le QPen et la QBrush du QPainter
	QPen pen = qp.pen();
	QBrush brush = qp.brush();
	
	// attributs par defaut
	pen.setJoinStyle(Qt::BevelJoin);
	pen.setCapStyle(Qt::SquareCap);
	
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
				else if (style_value == "dotted") pen.setStyle(Qt::DotLine);
				else if (style_value == "dashdotted") pen.setStyle(Qt::DashDotLine);
				else if (style_value == "normal") pen.setStyle(Qt::SolidLine);
			} else if (style_name == "line-weight") {
				if (style_value == "none") pen.setColor(QColor(0, 0, 0, 0));
				else if (style_value == "thin") pen.setWidth(0);
				else if (style_value == "normal") pen.setWidthF(1.0);
				else if (style_value == "hight") pen.setWidthF(2.0);
				else if (style_value == "eleve") pen.setWidthF(5.0);

			} else if (style_name == "filling") {
				if (style_value == "white") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::white);
				} else if (style_value == "black") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::black);
				} else if (style_value == "blue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::blue);
				} else if (style_value == "red") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::red);
				} else if (style_value == "green") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::green);
				} else if (style_value == "gray") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::gray);
				} else if (style_value == "brun") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(97, 44, 0));
				} else if (style_value == "yellow") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::yellow);
				} else if (style_value == "cyan") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::cyan);
				} else if (style_value == "magenta") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::magenta);
				} else if (style_value == "lightgray") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::lightGray);
				} else if (style_value == "orange") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 128, 0));
				} else if (style_value == "purple") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(136, 28, 168));
				}else if (style_value == "hor") {
					brush.setStyle(Qt::HorPattern);
					brush.setColor(Qt::black);
				} else if (style_value == "ver") {
					brush.setStyle(Qt::VerPattern);
					brush.setColor(Qt::black);
				} else if (style_value == "bdiag") {
					brush.setStyle(Qt::BDiagPattern);
					brush.setColor(Qt::black);
				} else if (style_value == "fdiag") {
					brush.setStyle(Qt::FDiagPattern);
					brush.setColor(Qt::black);
				} else if (style_value == "none") {
					brush.setStyle(Qt::NoBrush);
				}
			} else if (style_name == "color") {
				if (style_value == "black") {
					pen.setColor(QColor(0, 0, 0, pen.color().alpha()));
				} else if (style_value == "white") {
					pen.setColor(QColor(255, 255, 255, pen.color().alpha()));
				} else if (style_value == "red") {
					pen.setColor(Qt::red);
				}else if (style_value == "blue") {
					pen.setColor(Qt::blue);
				}else if (style_value == "green") {
					pen.setColor(Qt::green);
				}else if (style_value == "gray") {
					pen.setColor(Qt::gray);
				}else if (style_value == "brun") {
					pen.setColor(QColor(97, 44, 0));
				}else if (style_value == "yellow") {
					pen.setColor(Qt::yellow);
				}else if (style_value == "cyan") {
					pen.setColor(Qt::cyan);
				}else if (style_value == "magenta") {
					pen.setColor(Qt::magenta);
				}else if (style_value == "lightgray") {
					pen.setColor(Qt::lightGray);
				}else if (style_value == "orange") {
					pen.setColor(QColor(255, 128, 0));
				}else if (style_value == "purple") {
					pen.setColor(QColor(136, 28, 168));
				} else if (style_value == "none") {
					pen.setBrush(Qt::transparent);
				}
			}
		}
	}
	
	// affectation du QPen et de la QBrush modifies au QPainter 
	qp.setPen(pen);
	qp.setBrush(brush);
	
	// mise en place (ou non) de l'antialiasing
	setQPainterAntiAliasing(qp, e.attribute("antialias") == "true");
}

/**
 * @brief CustomElement::setTaggedText
 * Set text @newstr to the text tagged with @tagg.
 * If tagg is found return the text item, else return NULL.
 * @param tagg required tagg
 * @param newstr new label
 * @param noeditable set editable or not (by default, set editable)
 */
ElementTextItem* CustomElement::setTaggedText(const QString &tagg, const QString &newstr, const bool noeditable) {
	ElementTextItem *eti = taggedText(tagg);
	if (eti) {
		eti -> setPlainText(newstr);
		eti -> setNoEditable(noeditable);
	}
	return eti;
}

/**
 * @brief CustomElement::taggedText
 * return the text field tagged with @tagg or NULL if text field isn't found
 * @param tagg
 */
ElementTextItem* CustomElement::taggedText(const QString &tagg) const {
	foreach (ElementTextItem *eti, list_texts_) {
		if (eti -> tagg() == tagg) return eti;
	}
	return NULL;
}

QString CustomElement::assignVariables(QString label, Element *elmt){
	label.replace("%f", QString::number(elmt->diagram()->folioIndex()+1));
	label.replace("%F", elmt->diagram() -> border_and_titleblock.folio());
	label.replace("%c", QString::number(elmt->diagram() -> convertPosition(elmt -> scenePos()).number()));
	label.replace("%l", elmt->diagram() -> convertPosition(elmt -> scenePos()).letter());
	return label;
}
