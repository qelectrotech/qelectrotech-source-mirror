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

PartLine::PartLine(QETElementEditor *editor, QGraphicsItem *parent, QGraphicsScene *scene) : QGraphicsLineItem(parent, scene), CustomElementGraphicPart(editor) {
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptedMouseButtons(Qt::LeftButton);
	informations = new LineEditor(elementEditor(), this);
	informations -> setElementTypeName(name());
	style_editor -> appendWidget(informations);
	style_editor -> setElementTypeName(name());
}

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

const QDomElement PartLine::toXml(QDomDocument &xml_document) const {
	
	QPointF p1(sceneP1());
	QPointF p2(sceneP2());
	
	// cas particulier : on n'enregistre pas les lignes equivalentes a un point
	if (p1 == p2) {
		QDomElement *null_qdom_elmt = new QDomElement();
		return(*null_qdom_elmt);
	}
	
	QDomElement xml_element = xml_document.createElement("line");
	xml_element.setAttribute("x1", p1.x());
	xml_element.setAttribute("y1", p1.y());
	xml_element.setAttribute("x2", p2.x());
	xml_element.setAttribute("y2", p2.y());
	stylesToXml(xml_element);
	return(xml_element);
}

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

QVariant PartLine::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (scene()) {
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemSelectedChange) {
			informations -> updateForm();
		}
	}
	return(QGraphicsLineItem::itemChange(change, value));
}

QPointF PartLine::sceneP1() const {
	return(mapToScene(line().p1()));
}

QPointF PartLine::sceneP2() const {
	return(mapToScene(line().p2()));
}

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
		
		// on définit le vecteur v(-b , a)  qui est perpendiculaire à AB
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

QRectF PartLine::boundingRect() const {
	qreal adjust = 1.5;
	QRectF r(QGraphicsLineItem::boundingRect());
	r.adjust(-adjust, -adjust, adjust, adjust);
	return(r);
}
