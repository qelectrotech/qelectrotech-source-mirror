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
#include "partpolygon.h"
#include "qet.h"
#include "polygoneditor.h"
PartPolygon::PartPolygon(QETElementEditor *editor, QGraphicsItem *parent, QGraphicsScene *scene) : 
	QGraphicsPolygonItem(parent, scene),
	CustomElementGraphicPart(editor),
	closed(false)
{
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptedMouseButtons(Qt::LeftButton);
	informations = new PolygonEditor(elementEditor(), this);
	informations -> setElementTypeName(name());
	style_editor -> appendWidget(informations);
	style_editor -> setElementTypeName(name());
}

void PartPolygon::fromXml(const QDomElement &qde) {
	stylesFromXml(qde);
	int i = 1;
	while(true) {
		if (
			QET::attributeIsAReal(qde, QString("x%1").arg(i)) &&\
			QET::attributeIsAReal(qde, QString("y%1").arg(i))
		) ++ i;
		else break;
	}
	
	QPolygonF temp_polygon;
	for (int j = 1 ; j < i ; ++ j) {
		temp_polygon << QPointF(
			qde.attribute(QString("x%1").arg(j)).toDouble(),
			qde.attribute(QString("y%1").arg(j)).toDouble()
		);
	}
	setPolygon(temp_polygon);
	
	closed = qde.attribute("closed") != "false";
}

const QDomElement PartPolygon::toXml(QDomDocument &xml_document) const {
	QDomElement xml_element = xml_document.createElement("polygon");
	int i = 1;
	foreach(QPointF point, polygon()) {
		point = mapToScene(point);
		xml_element.setAttribute(QString("x%1").arg(i), point.x());
		xml_element.setAttribute(QString("y%1").arg(i), point.y());
		++ i;
	}
	if (!closed) xml_element.setAttribute("closed", "false");
	stylesToXml(xml_element);
	return(xml_element);
}

void PartPolygon::paint(QPainter *painter, const QStyleOptionGraphicsItem */*q*/, QWidget */*w*/) {
	applyStylesToQPainter(*painter);
	QPen t = painter -> pen();
	if (isSelected()) {
		t.setColor(Qt::red);
		painter -> setPen(t);
	}
	if (closed) painter -> drawPolygon(polygon());
	else painter -> drawPolyline(polygon());
}

void PartPolygon::setProperty(const QString &property, const QVariant &value) {
	CustomElementGraphicPart::setProperty(property, value);
	if (property == "closed") closed = value.toBool();
}

QVariant PartPolygon::property(const QString &property) {
	// appelle la methode property de CustomElementGraphicpart pour les styles
	QVariant style_property = CustomElementGraphicPart::property(property);
	if (style_property != QVariant()) return(style_property);
	
	if (property == "closed") return(closed);
	return(QVariant());
}

QVariant PartPolygon::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (scene()) {
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemSelectedChange) {
			informations -> updateForm();
		}
	}
	return(QGraphicsPolygonItem::itemChange(change, value));
}
