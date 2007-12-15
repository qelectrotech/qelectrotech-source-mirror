/*
	Copyright 2006-2007 Xavier Guerrin
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
#include "lineeditor.h"
#include <cmath>

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param parent Le QGraphicsItem parent de cette ligne
	@param scene La scene sur laquelle figure cette ligne
*/
PartLine::PartLine(QETElementEditor *editor, QGraphicsItem *parent, QGraphicsScene *scene) : QGraphicsLineItem(parent, scene), CustomElementGraphicPart(editor) {
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptedMouseButtons(Qt::LeftButton);
	informations = new LineEditor(elementEditor(), this);
	informations -> setElementTypeName(name());
	style_editor -> appendWidget(informations);
	style_editor -> setElementTypeName(name());
}

/// Destructeur
PartLine::~PartLine() {
}

/**
	Dessine la ligne
	@param painter QPainter a utiliser pour rendre le dessin
	@param options Options pour affiner le rendu
	@param widget Widget sur lequel le rendu est effectue
*/
void PartLine::paint(QPainter *painter, const QStyleOptionGraphicsItem */*q*/, QWidget */*w*/) {
	applyStylesToQPainter(*painter);
	QPen t = painter -> pen();
	if (isSelected()) {
		t.setColor(Qt::red);
		painter -> setPen(t);
	}
	painter -> setBrush(Qt::NoBrush);
	painter -> drawLine(line());
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
	xml_element.setAttribute("x1", p1.x());
	xml_element.setAttribute("y1", p1.y());
	xml_element.setAttribute("x2", p2.x());
	xml_element.setAttribute("y2", p2.y());
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
}

/**
	Specifie la valeur d'une propriete donnee de la ligne
	@param property propriete a modifier. Valeurs acceptees :
		* x1 : abscisse du premier point
		* y1 : ordonnee du second point
		* x2 : abscisse du premier point
		* y2 : ordonnee du second point
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
	} else setline = false;
	setLine(QLineF(mapFromScene(new_p1), mapFromScene(new_p2)));
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
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemSelectedChange) {
			informations -> updateForm();
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
	@return le rectangle delimitant cette partie.
*/
QRectF PartLine::boundingRect() const {
	qreal adjust = 1.5;
	QRectF r(QGraphicsLineItem::boundingRect());
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

