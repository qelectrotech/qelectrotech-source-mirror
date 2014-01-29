/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "partline.h"
#include <cmath>

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param parent Le QGraphicsItem parent de cette ligne
	@param scene La scene sur laquelle figure cette ligne
*/
PartLine::PartLine(QETElementEditor *editor, QGraphicsItem *parent, QGraphicsScene *scene) :
	QGraphicsLineItem(parent, scene),
	CustomElementGraphicPart(editor),
	first_end(QET::None),
	first_length(1.5),
	second_end(QET::None),
	second_length(1.5)
{
	setFlags(QGraphicsItem::ItemIsSelectable);
#if QT_VERSION >= 0x040600
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif
	setAcceptedMouseButtons(Qt::LeftButton);
}

/// Destructeur
PartLine::~PartLine() {
}

/**
	@param end_type Type d'extremite
	@return Le nombre de "longueurs" requises pour dessiner une extremite de type end_type
*/
uint PartLine::requiredLengthForEndType(const QET::EndType &end_type) {
	uint length_count_required = 0;
	if (end_type == QET::Circle || end_type == QET::Diamond) {
		length_count_required = 2;
	} else if (end_type == QET::Simple || end_type == QET::Triangle) {
		length_count_required = 1;
	}
	return(length_count_required);
}

/**
	Dessine la ligne
	@param painter QPainter a utiliser pour rendre le dessin
	@param options Options pour affiner le rendu
	@param widget Widget sur lequel le rendu est effectue
*/
void PartLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *widget) {
	Q_UNUSED(widget);
	// inutile de dessiner une ligne nulle
	if (line().p1() == line().p2()) return;
	applyStylesToQPainter(*painter);
	QPen t = painter -> pen();
	t.setJoinStyle(Qt::MiterJoin);
	t.setCosmetic(options && options -> levelOfDetail < 1.0);
	if (isSelected()) {
		t.setColor(Qt::red);
	}
	painter -> setPen(t);
	
	QPointF point1(line().p1());
	QPointF point2(line().p2());
	
	qreal line_length(line().length());
	qreal pen_width = painter -> pen().widthF();
	
	qreal length1 = first_length;
	qreal length2 = second_length;
	
	//debugPaint(painter);
	
	// determine s'il faut dessiner les extremites
	bool draw_1st_end, draw_2nd_end;
	qreal reduced_line_length = line_length - (length1 * requiredLengthForEndType(first_end));
	draw_1st_end = first_end && reduced_line_length >= 0;
	if (draw_1st_end) {
		reduced_line_length -= (length2 * requiredLengthForEndType(second_end));
	} else {
		reduced_line_length = line_length - (length2 * requiredLengthForEndType(second_end));
	}
	draw_2nd_end = second_end && reduced_line_length >= 0;
	
	// dessine la premiere extremite
	QPointF start_point, stop_point;
	if (draw_1st_end) {
		QList<QPointF> four_points1(fourEndPoints(point1, point2, length1));
		if (first_end == QET::Circle) {
			painter -> drawEllipse(QRectF(four_points1[0] - QPointF(length1, length1), QSizeF(length1 * 2.0, length1 * 2.0)));
			start_point = four_points1[1];
		} else if (first_end == QET::Diamond) {
			painter -> drawPolygon(QPolygonF() << four_points1[1] << four_points1[2] << point1 << four_points1[3]);
			start_point = four_points1[1];
		} else if (first_end == QET::Simple) {
			painter -> drawPolyline(QPolygonF() << four_points1[3] << point1 << four_points1[2]);
			start_point = point1;
			
		} else if (first_end == QET::Triangle) {
			painter -> drawPolygon(QPolygonF() << four_points1[0] << four_points1[2] << point1 << four_points1[3]);
			start_point = four_points1[0];
		}
		
		// ajuste le depart selon l'epaisseur du trait
		if (pen_width && (first_end == QET::Simple || first_end == QET::Circle)) {
			start_point = QLineF(start_point, point2).pointAt(pen_width / 2.0 / line_length);
		}
	} else {
		start_point = point1;
	}
	
	// dessine la seconde extremite
	if (draw_2nd_end) {
		QList<QPointF> four_points2(fourEndPoints(point2, point1, length2));
		if (second_end == QET::Circle) {
			painter -> drawEllipse(QRectF(four_points2[0] - QPointF(length2, length2), QSizeF(length2 * 2.0, length2 * 2.0)));
			stop_point = four_points2[1];
		} else if (second_end == QET::Diamond) {
			painter -> drawPolygon(QPolygonF() << four_points2[2] << point2 << four_points2[3] << four_points2[1]);
			stop_point = four_points2[1];
		} else if (second_end == QET::Simple) {
			painter -> drawPolyline(QPolygonF() << four_points2[3] << point2 << four_points2[2]);
			stop_point = point2;
		} else if (second_end == QET::Triangle) {
			painter -> drawPolygon(QPolygonF() << four_points2[0] << four_points2[2] << point2 << four_points2[3] << four_points2[0]);
			stop_point = four_points2[0];
		}
		
		// ajuste l'arrivee selon l'epaisseur du trait
		if (pen_width && (second_end == QET::Simple || second_end == QET::Circle)) {
			stop_point = QLineF(point1, stop_point).pointAt((line_length - (pen_width / 2.0)) / line_length);
		}
	} else {
		stop_point = point2;
	}
	
	painter -> drawLine(start_point, stop_point);
}

/**
	Exporte la ligne en XML
	@param xml_document Document XML a utiliser pour creer l'element XML
	@return un element XML decrivant la ligne
*/
const QDomElement PartLine::toXml(QDomDocument &xml_document) const {
	
	QPointF p1(sceneP1());
	QPointF p2(sceneP2());
	
	QDomElement xml_element = xml_document.createElement("line");
	xml_element.setAttribute("x1", QString("%1").arg(p1.x()));
	xml_element.setAttribute("y1", QString("%1").arg(p1.y()));
	xml_element.setAttribute("x2", QString("%1").arg(p2.x()));
	xml_element.setAttribute("y2", QString("%1").arg(p2.y()));
	xml_element.setAttribute("end1", QET::endTypeToString(first_end));
	xml_element.setAttribute("length1", QString("%1").arg(first_length));
	xml_element.setAttribute("end2", QET::endTypeToString(second_end));
	xml_element.setAttribute("length2", QString("%1").arg(second_length));
	
	stylesToXml(xml_element);
	return(xml_element);
}

/**
	Importe les proprietes d'une ligne depuis un element XML
	@param qde Element XML a lire
*/
void PartLine::fromXml(const QDomElement &qde) {
	stylesFromXml(qde);
	setLine(
		QLineF(
			mapFromScene(
				qde.attribute("x1", "0").toDouble(),
				qde.attribute("y1", "0").toDouble()
			),
			mapFromScene(
				qde.attribute("x2", "0").toDouble(),
				qde.attribute("y2", "0").toDouble()
			)
		)
	);
	first_end    = QET::endTypeFromString(qde.attribute("end1"));
	first_length = qde.attribute("length1", "1.5").toDouble();
	second_end   = QET::endTypeFromString(qde.attribute("end2"));
	second_length = qde.attribute("length2", "1.5").toDouble();
}

/**
	Specifie la valeur d'une propriete donnee de la ligne
	@param property propriete a modifier. Valeurs acceptees :
		* x1 : abscisse du premier point
		* y1 : ordonnee du second point
		* x2 : abscisse du premier point
		* y2 : ordonnee du second point
		*end1 : type d'embout du premier point
		*end2 : type d'embout du second point
	@param value Valeur a attribuer a la propriete
*/
void PartLine::setProperty(const QString &property, const QVariant &value) {
	CustomElementGraphicPart::setProperty(property, value);
	if (!value.canConvert(QVariant::Double)) return;
	QPointF new_p1(sceneP1()), new_p2(sceneP2());
	bool setline = true;
	if (property == "x1") {
		new_p1.setX(value.toDouble());
	} else if (property == "y1") {
		new_p1.setY(value.toDouble());
	} else if (property == "x2") {
		new_p2.setX(value.toDouble());
	} else if (property == "y2") {
		new_p2.setY(value.toDouble());
	} else {
		setline = false;
		if (property == "end1") {
			setFirstEndType(static_cast<QET::EndType>(value.toUInt()));
		} else if (property == "end2") {
			setSecondEndType(static_cast<QET::EndType>(value.toUInt()));
		} else if (property == "length1") {
			setFirstEndLength(value.toDouble());
		} else if (property == "length2") {
			setSecondEndLength(value.toDouble());
		}
	}
	if (setline) setLine(QLineF(mapFromScene(new_p1), mapFromScene(new_p2)));
	update();
}

/**
	Permet d'acceder a la valeur d'une propriete donnee de la ligne
	@param property propriete lue. Valeurs acceptees :
		* x1 : abscisse du premier point
		* y1 : ordonnee du second point
		* x2 : abscisse du premier point
		* y2 : ordonnee du second point
	@return La valeur de la propriete property
*/
QVariant PartLine::property(const QString &property) {
	// appelle la methode property de CustomElementGraphicpart pour les styles
	QVariant style_property = CustomElementGraphicPart::property(property);
	if (style_property != QVariant()) return(style_property);
	
	if (property == "x1") {
		return(sceneP1().x());
	} else if (property == "y1") {
		return(sceneP1().y());
	} else if (property == "x2") {
		return(sceneP2().x());
	} else if (property == "y2") {
		return(sceneP2().y());
	} else if (property == "end1") {
		return(firstEndType());
	} else if (property == "end2") {
		return(secondEndType());
	} else if (property == "length1") {
		return(firstEndLength());
	} else if (property == "length2") {
		return(secondEndLength());
	}
	return(QVariant());
}

/**
	Gere les changements intervenant sur cette partie
	@param change Type de changement
	@param value Valeur numerique relative au changement
*/
QVariant PartLine::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (scene()) {
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemPositionHasChanged) {
			updateCurrentPartEditor();
		}
	}
	return(QGraphicsLineItem::itemChange(change, value));
}

/**
	@return le premier point, dans les coordonnees de la scene.
*/
QPointF PartLine::sceneP1() const {
	return(mapToScene(line().p1()));
}

/**
	@return le second point, dans les coordonnees de la scene.
*/
QPointF PartLine::sceneP2() const {
	return(mapToScene(line().p2()));
}

/**
	@return la forme selectionnable de la ligne
*/
QPainterPath PartLine::shape() const {
	QList<QPointF> points = fourShapePoints();
	QPainterPath t;
	t.setFillRule(Qt::WindingFill);
	t.moveTo(points.at(0));
	t.lineTo(points.at(1));
	t.lineTo(points.at(2));
	t.lineTo(points.at(3));
	t.lineTo(points.at(0));
	
	// n'en fait pas plus si la ligne se ramene a un point
	if (line().p1() == line().p2()) return(t);
	
	// ajoute un cercle pour l'extremite 1 si besoin
	if (first_end) {
		QPainterPath t2;
		t2.addEllipse(firstEndCircleRect());
		t.addPath(t2.subtracted(t));
	}
	
	// ajoute un cercle pour l'extremite 2 si besoin
	if (second_end) {
		QPainterPath t2;
		t2.addEllipse(secondEndCircleRect());
		t.addPath(t2.subtracted(t));
	}
	
	return(t);
}

/**
	@return une liste contenant les deux points de la droite + les 4 points entourant ces deux points
*/
QList<QPointF> PartLine::fourShapePoints() const {
	const qreal marge = 2.0;
	// on a donc A(xa , ya) et B(xb, yb)
	QPointF a = line().p1();
	QPointF b = line().p2();
	
	QList<QPointF> result;
	
	// cas particulier : la droite se ramene a un point
	if (a == b) {
		result << QPointF(a.x() - marge, a.y() - marge);
		result << QPointF(a.x() - marge, a.y() + marge);
		result << QPointF(a.x() + marge, a.y() + marge);
		result << QPointF(a.x() + marge, a.y() - marge);
	} else {
		
		// on calcule le vecteur AB : (xb-xa, yb-ya)
		QPointF v_ab = b - a;
		
		// et la distance AB : racine des coordonnees du vecteur au carre
		qreal ab = sqrt(pow(v_ab.x(), 2) + pow(v_ab.y(), 2));
		
		// ensuite on definit le vecteur u(a, b) qui est egal au vecteur AB divise
		// par sa longueur et multiplie par la longueur de la marge  que tu veux
		// laisser
		QPointF u = v_ab / ab * marge;
		
		// on definit le vecteur v(-b , a) qui est perpendiculaire a AB
		QPointF v(-u.y(), u.x());
		QPointF m = -u + v; // on a le vecteur M = -u + v
		QPointF n = -u - v; // et le vecteur N=-u-v
		QPointF h =  a + m; // H = A + M
		QPointF k =  a + n; // K = A + N
		QPointF i =  b - n; // I = B - N
		QPointF j =  b - m; // J = B - M
		
		result << h << i << j << k;
	}
	return(result);
}

/**
	@return le rectangle encadrant l'integralite de la premiere extremite
*/
QRectF PartLine::firstEndCircleRect() const {
	QList<QPointF> interesting_points = fourEndPoints(
		line().p1(),
		line().p2(),
		first_length
	);
	
	QRectF end_rect(
		interesting_points[0] - QPointF(first_length, first_length),
		QSizeF(2.0 * first_length, 2.0 * first_length)
	);
	
	return(end_rect);
}

/**
	@return le rectangle encadrant l'integralite de la seconde extremite
*/
QRectF PartLine::secondEndCircleRect() const {
	QList<QPointF> interesting_points = fourEndPoints(
		line().p2(),
		line().p1(),
		second_length
	);
	
	QRectF end_rect(
		interesting_points[0] - QPointF(second_length, second_length),
		QSizeF(2.0 * second_length, 2.0 * second_length)
	);
	
	return(end_rect);
}

/**
	Affiche differentes composantes du dessin :
	  - le boundingRect
	  - les point speciaux a chaque extremite
	  - la quadrature du cercle a chaque extremite, meme si celle-ci est d'un
	  autre type
*/
void PartLine::debugPaint(QPainter *painter) {
	painter -> save();
	painter -> setPen(Qt::gray);
	painter -> drawRect(boundingRect());
	
	painter -> setPen(Qt::green);
	painter -> drawRect(firstEndCircleRect());
	painter -> drawRect(secondEndCircleRect());
	
	painter -> setPen(Qt::red);
	foreach(QPointF pointy, fourEndPoints(line().p1(), line().p2(), first_length)) {
		painter -> drawEllipse(pointy, 0.1, 0.1);
	}
	foreach(QPointF pointy, fourEndPoints(line().p2(), line().p1(), second_length)) {
		painter -> drawEllipse(pointy, 0.1, 0.1);
	}
	
	painter -> restore();
}

/**
	@return le rectangle delimitant cette partie.
*/
QRectF PartLine::boundingRect() const {
	QRectF r(QGraphicsLineItem::boundingRect());
	
	// le rectangle ainsi obtenu ne doit pas avoir une dimension nulle
	r.adjust(0.0, 0.0, 0.1, 0.1);
	
	// cas special : les embouts sortent largement du bounding rect originel
	if (first_end != QET::None) {
		r = r.united(firstEndCircleRect());
	}
	
	if (second_end != QET::None) {
		r = r.united(secondEndCircleRect());
	}
	
	// la taille du bounding rect est ajustee avec une certaine marge
	qreal adjust = 1.2;
	r.adjust(-adjust, -adjust, adjust, adjust);
	return(r);
}

/**
	@return true si cette partie n'est pas pertinente et ne merite pas d'etre
	conservee / enregistree.
	Une ligne est pertinente des lors que ses deux points sont differents
*/
bool PartLine::isUseless() const {
	return(sceneP1() == sceneP2());
}

/**
	@return the minimum, margin-less rectangle this part can fit into, in scene
	coordinates. It is different from boundingRect() because it is not supposed
	to imply any margin, and it is different from shape because it is a regular
	rectangle, not a complex shape.
*/
QRectF PartLine::sceneGeometricRect() const {
	return(QRectF(sceneP1(), sceneP2()));
}

/**
	Start the user-induced transformation, provided this primitive is contained
	within the \a initial_selection_rect bounding rectangle.
*/
void PartLine::startUserTransformation(const QRectF &initial_selection_rect) {
	Q_UNUSED(initial_selection_rect)
	saved_points_.clear();
	saved_points_ << sceneP1() << sceneP2();
}

/**
	Handle the user-induced transformation from \a initial_selection_rect to \a new_selection_rect
*/
void PartLine::handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect) {
	QList<QPointF> mapped_points = mapPoints(initial_selection_rect, new_selection_rect, saved_points_);
	setLine(QLineF(mapFromScene(mapped_points.at(0)), mapFromScene(mapped_points.at(1))));
}

/**
	@param end_type nouveau type d'embout pour l'extremite 1
*/
void PartLine::setFirstEndType(const QET::EndType &end_type) {
	first_end = end_type;
}

/**
	@return le type d'embout pour l'extremite 1
*/
QET::EndType PartLine::firstEndType() const {
	return(first_end);
}

/**
	@param end_type Nouveau type d'embout pour l'extremite 2
*/
void PartLine::setSecondEndType(const QET::EndType &end_type) {
	second_end = end_type;
}

/**
	@return le type d'embout pour l'extremite 2
*/
QET::EndType PartLine::secondEndType() const {
	return(second_end);
}

/**
	@return Les quatre points interessants a l'extremite d'une droite
	Ces points sont, dans l'ordre :
		* O : point sur la ligne, a une distance length de l'extremite
		* A : point sur la ligne a une distance 2 x length de l'extremite
		* B : point a une distance length de O - O est le projete de B sur la droite
		* C : point a une distance length de O - O est le projete de C sur la droite
		B et C sont situes de part et d'autre de la ligne
	@param end_point Extremite concernee
	@param other_point Autre point permettant de definir une ligne
	@param length Longueur a utiliser entre l'extremite et le point O
*/
QList<QPointF> PartLine::fourEndPoints(const QPointF &end_point, const QPointF &other_point, const qreal &length) {
	// vecteur et longueur de la ligne 
	QPointF line_vector = end_point - other_point;
	qreal line_length = sqrt(pow(line_vector.x(), 2) + pow(line_vector.y(), 2));
	
	// vecteur unitaire et vecteur perpendiculaire
	QPointF u(line_vector / line_length * length);
	QPointF v(-u.y(), u.x());
	
	// points O, A, B et C
	QPointF o(end_point - u);
	QPointF a(o - u);
	QPointF b(o + v);
	QPointF c(o - v);
	
	return(QList<QPointF>() << o << a << b << c);
}

/**
	@param length nouvelle longueur de la premiere extremite
	la longueur de l'extemite ne peut exceder celle de la ligne
*/
void PartLine::setFirstEndLength(const qreal &length) {
	first_length = qMin(qAbs(length), line().length());
}

/**
	@return longueur de la premiere extremite
*/
qreal PartLine::firstEndLength() const {
	return(first_length);
}

/**
	@param length nouvelle longueur de la seconde extremite
	la longueur de l'extemite ne peut exceder celle de la ligne
*/
void PartLine::setSecondEndLength(const qreal &length) {
	second_length = qMin(qAbs(length), line().length());
}

/**
	@return longueur de la seconde extremite
*/
qreal PartLine::secondEndLength() const {
	return(second_length);
}
