#include "partcircle.h"
#include "circleeditor.h"

PartCircle::PartCircle(QGraphicsItem *parent, QGraphicsScene *scene) : QGraphicsEllipseItem(parent, scene), CustomElementGraphicPart() {
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptedMouseButtons(Qt::LeftButton);
	informations = new CircleEditor(this);
	style_editor -> appendWidget(informations);
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
