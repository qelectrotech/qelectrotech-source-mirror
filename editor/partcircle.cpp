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
#include "partcircle.h"
#include "circleeditor.h"

PartCircle::PartCircle(QETElementEditor *editor, QGraphicsItem *parent, QGraphicsScene *scene) : QGraphicsEllipseItem(parent, scene), CustomElementGraphicPart(editor) {
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptedMouseButtons(Qt::LeftButton);
	informations = new CircleEditor(elementEditor(), this);
	informations -> setElementTypeName(name());
	style_editor -> appendWidget(informations);
	style_editor -> setElementTypeName(name());
}

void PartCircle::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
	applyStylesToQPainter(*painter);
	QPen t = painter -> pen();
	if (isSelected()) {
		t.setColor(Qt::red);
		painter -> setPen(t);
	}
	painter -> drawEllipse(rect());
	if (isSelected()) {
		painter -> setRenderHint(QPainter::Antialiasing, false);
		painter -> setPen((painter -> brush().color() == QColor(Qt::black) && painter -> brush().isOpaque()) ? Qt::yellow : Qt::blue);
		QPointF center = rect().center();
		painter -> drawLine(QLineF(center.x() - 2.0, center.y(), center.x() + 2.0, center.y()));
		painter -> drawLine(QLineF(center.x(), center.y() - 2.0, center.x(), center.y() + 2.0));
	}
}

const QDomElement PartCircle::toXml(QDomDocument &xml_document) const {
	QDomElement xml_element = xml_document.createElement("circle");
	QPointF top_left(sceneTopLeft());
	xml_element.setAttribute("x", top_left.x());
	xml_element.setAttribute("y", top_left.y());
	xml_element.setAttribute("diameter",  rect().width());
	stylesToXml(xml_element);
	return(xml_element);
}

void PartCircle::fromXml(const QDomElement &qde) {
	stylesFromXml(qde);
	qreal diameter = qde.attribute("diameter",  "0").toDouble();
	setRect(
		QRectF(
			mapFromScene(
				qde.attribute("x", "0").toDouble(),
				qde.attribute("y", "0").toDouble()
			),
			QSizeF(
				diameter,
				diameter
			)
		)
	);
}

void PartCircle::setProperty(const QString &property, const QVariant &value) {
	CustomElementGraphicPart::setProperty(property, value);
	if (!value.canConvert(QVariant::Double)) return;
	if (property == "x") {
		QRectF current_rect = rect();
		QPointF current_pos = mapToScene(current_rect.center());
		setRect(current_rect.translated(value.toDouble() - current_pos.x(), 0.0));
	} else if (property == "y") {
		QRectF current_rect = rect();
		QPointF current_pos = mapToScene(current_rect.center());
		setRect(current_rect.translated(0.0, value.toDouble() - current_pos.y()));
	} else if (property == "diameter") {
		QRectF current_rect = rect();
		qreal new_diameter = qAbs(value.toDouble());
		current_rect.translate(
			(new_diameter - current_rect.width()) / -2.0,
			(new_diameter - current_rect.height()) / -2.0
		);
		current_rect.setSize(QSizeF(new_diameter, new_diameter));
		setRect(current_rect);
	}
}

QVariant PartCircle::property(const QString &property) {
	// appelle la methode property de CustomElementGraphicpart pour les styles
	QVariant style_property = CustomElementGraphicPart::property(property);
	if (style_property != QVariant()) return(style_property);
	
	if (property == "x") {
		return(mapToScene(rect().center()).x());
	} else if (property == "y") {
		return(mapToScene(rect().center()).y());
	} else if (property == "diameter") {
		return(rect().width());
	}
	return(QVariant());
}

QVariant PartCircle::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (scene()) {
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemSelectedChange) {
			informations -> updateForm();
		}
	}
	return(QGraphicsEllipseItem::itemChange(change, value));
}

QPointF PartCircle::sceneTopLeft() const {
	return(mapToScene(rect().topLeft()));
}

QPointF PartCircle::sceneCenter() const {
	return(mapToScene(rect().center()));
}

QRectF PartCircle::boundingRect() const {
	qreal adjust = 1.5;
	QRectF r(QGraphicsEllipseItem::boundingRect());
	r.adjust(-adjust, -adjust, adjust, adjust);
	return(r);
}
