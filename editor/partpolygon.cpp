#include "partpolygon.h"
#include "qet.h"
#include "polygoneditor.h"
PartPolygon::PartPolygon(QGraphicsItem *parent, QGraphicsScene *scene) : 
	QGraphicsPolygonItem(parent, scene),
	CustomElementGraphicPart(),
	closed(false)
{
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptedMouseButtons(Qt::LeftButton);
	informations = new PolygonEditor(this);
	style_editor -> appendWidget(informations);
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

QVariant PartPolygon::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (scene()) {
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemSelectedChange) {
			informations -> updateForm();
		}
	}
	return(QGraphicsPolygonItem::itemChange(change, value));
}
