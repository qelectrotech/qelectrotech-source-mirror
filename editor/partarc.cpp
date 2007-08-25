#include "partarc.h"
#include "arceditor.h"

PartArc::PartArc(QETElementEditor *editor, QGraphicsItem *parent, QGraphicsScene *scene) :
	QGraphicsEllipseItem(parent, scene),
	CustomElementGraphicPart(editor),
	_angle(-90),
	start_angle(0)
{
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptedMouseButtons(Qt::LeftButton);
	informations = new ArcEditor(elementEditor(), this);
	informations -> setElementTypeName(QObject::tr("arc"));
	style_editor -> appendWidget(informations);
	style_editor -> setElementTypeName(QObject::tr("arc"));
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

void PartArc::setProperty(const QString &property, const QVariant &value) {
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
	} else if (property == "diameter_h") {
		qreal new_width = qAbs(value.toDouble());
		QRectF current_rect = rect();
		current_rect.translate((new_width - current_rect.width()) / -2.0, 0.0);
		current_rect.setWidth(new_width);
		setRect(current_rect);
	} else if (property == "diameter_v") {
		qreal new_height = qAbs(value.toDouble());
		QRectF current_rect = rect();
		current_rect.translate(0.0, (new_height - current_rect.height()) / -2.0);
		current_rect.setHeight(new_height);
		setRect(current_rect);
	} else if (property == "start_angle") {
		setStartAngle(value.toInt() );
	} else if (property == "angle") {
		setAngle(value.toInt());
	}
}

QVariant PartArc::property(const QString &property) {
	// appelle la methode property de CustomElementGraphicpart pour les styles
	QVariant style_property = CustomElementGraphicPart::property(property);
	if (style_property != QVariant()) return(style_property);
	
	if (property == "x") {
		return(mapToScene(rect().center()).x());
	} else if (property == "y") {
		return(mapToScene(rect().center()).y());
	} else if (property == "diameter_h") {
		return(rect().width());
	} else if (property == "diameter_v") {
		return(rect().height());
	} else if (property == "start_angle") {
		return(start_angle);
	} else if (property == "angle") {
		return(_angle);
	}
	return(QVariant());
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
