#include "partarc.h"
#include "arceditor.h"

PartArc::PartArc(QGraphicsItem *parent, QGraphicsScene *scene) :
	QGraphicsEllipseItem(parent, scene),
	CustomElementGraphicPart(),
	_angle(-90),
	start_angle(0)
{
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptedMouseButtons(Qt::LeftButton);
	informations = new ArcEditor(this);
	style_editor -> appendWidget(informations);
}

void PartArc::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
	applyStylesToQPainter(*painter);
	// enleve systematiquement la couleur de fond
	painter -> setBrush(Qt::NoBrush);
	QPen t = painter -> pen();
	if (isSelected()) {
		// dessine l'ellipse en noir
		painter -> drawEllipse(rect());
		
		// dessine l'arc en rouge
		t.setColor(Qt::red);
		painter -> setPen(t);
	}
	painter -> drawArc(rect(), start_angle * 16, _angle * 16);
	if (isSelected()) {
		// dessine la croix au centre de l'ellipse
		painter -> setRenderHint(QPainter::Antialiasing, false);
		painter -> setPen((painter -> brush().color() == QColor(Qt::black) && painter -> brush().isOpaque()) ? Qt::yellow : Qt::blue);
		QPointF center = rect().center();
		painter -> drawLine(QLineF(center.x() - 2.0, center.y(), center.x() + 2.0, center.y()));
		painter -> drawLine(QLineF(center.x(), center.y() - 2.0, center.x(), center.y() + 2.0));
	}
}

const QDomElement PartArc::toXml(QDomDocument &xml_document) const {
	QDomElement xml_element = xml_document.createElement("arc");
	QPointF top_left(sceneTopLeft());
	xml_element.setAttribute("x", top_left.x());
	xml_element.setAttribute("y", top_left.y());
	xml_element.setAttribute("width",  rect().width());
	xml_element.setAttribute("height", rect().height());
	xml_element.setAttribute("start", start_angle);
	xml_element.setAttribute("angle", _angle);
	stylesToXml(xml_element);
	return(xml_element);
}

void PartArc::fromXml(const QDomElement &qde) {
	stylesFromXml(qde);
	setRect(
		QRectF(
			mapFromScene(
				qde.attribute("x", "0").toDouble(),
				qde.attribute("y", "0").toDouble()
			),
			QSizeF(
				qde.attribute("width",  "0").toDouble(),
				qde.attribute("height", "0").toDouble()
			)
		)
	);
	setStartAngle(qde.attribute("start", "0").toInt());
	setAngle(qde.attribute("angle", "-90").toInt());
}

QPointF PartArc::sceneTopLeft() const {
	return(mapToScene(rect().topLeft()));
}

QVariant PartArc::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (scene()) {
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemSelectedChange) {
			informations -> updateForm();
		}
	}
	return(QGraphicsEllipseItem::itemChange(change, value));
}

void PartArc::setAngle(int a) {
	_angle = a;
}

void PartArc::setStartAngle(int a) {
	start_angle = a;
}

int PartArc::angle() const {
	return(_angle);
}

int PartArc::startAngle() const {
	return(start_angle);
}
