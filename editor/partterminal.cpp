#include "partterminal.h"
#include "terminal.h"
#include "terminaleditor.h"

PartTerminal::PartTerminal(QETElementEditor *editor, QGraphicsItem *parent, QGraphicsScene *scene) :
	CustomElementPart(editor),
	QGraphicsItem(parent, scene),
	_orientation(QET::North)
{
	informations = new TerminalEditor(elementEditor(), this);
	informations -> setElementTypeName(QObject::tr("borne"));
	updateSecondPoint();
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setZValue(100000);
}

PartTerminal::~PartTerminal() {
	delete informations;
};

void PartTerminal::fromXml(const QDomElement &xml_elmt) {
	// lit la position de la borne
	qreal term_x = 0.0, term_y = 0.0;
	QET::attributeIsAReal(xml_elmt, "x", &term_x);
	QET::attributeIsAReal(xml_elmt, "y", &term_y);
	setPos(QPointF(term_x, term_y));
	
	// lit l'orientation de la borne
	_orientation = QET::orientationFromString(xml_elmt.attribute("orientation"));
	updateSecondPoint();
}

const QDomElement PartTerminal::toXml(QDomDocument &xml_document) const {
	QDomElement xml_element = xml_document.createElement("terminal");
	
	// ecrit la position de la borne
	xml_element.setAttribute("x", QString("%1").arg(scenePos().x()));
	xml_element.setAttribute("y", QString("%1").arg(scenePos().y()));
	
	// ecrit l'orientation de la borne
	xml_element.setAttribute("orientation", orientationToString(_orientation));
	
	return(xml_element);
}

QWidget *PartTerminal::elementInformations() {
	return(informations);
}

void PartTerminal::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) {
	p -> save();
	
	// annulation des renderhints
	p -> setRenderHint(QPainter::Antialiasing,          false);
	p -> setRenderHint(QPainter::TextAntialiasing,      false);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	
	QPen t;
	t.setWidthF(1.0);
	
	// dessin de la borne en rouge
	t.setColor(Qt::red);
	p -> setPen(t);
	p -> drawLine(QPointF(0.0, 0.0), second_point);
	
	// dessin du point d'amarrage au conducteur en bleu
	t.setColor(Terminal::couleur_neutre);
	p -> setPen(t);
	p -> setBrush(Terminal::couleur_neutre);
	p -> drawPoint(QPointF(0.0, 0.0));
	p -> restore();
}

QRectF PartTerminal::boundingRect() const {
	QPointF p1, p2;
	if (second_point.x() <= 0.0 && second_point.y() <= 0.0) {
		p1 = second_point;
		p2 = QPointF(0.0, 0.0);
	} else {
		p1 = QPointF(0.0, 0.0);
		p2 = second_point;
	}
	QRectF br;
	br.setTopLeft    (p1 - QPointF(2.0, 2.0));
	br.setBottomRight(p2 + QPointF(2.0, 2.0));
	return(br);
}

QET::Orientation PartTerminal::orientation() const {
	return(_orientation);
}

void PartTerminal::setOrientation(QET::Orientation ori) {
	prepareGeometryChange();
	_orientation = ori;
	updateSecondPoint();
	informations -> updateForm();
}

void PartTerminal::setProperty(const QString &property, const QVariant &value) {
	if (property == "x") {
		if (!value.canConvert(QVariant::Double)) return;
		setPos(value.toDouble(), pos().y());
	} else if (property == "y") {
		if (!value.canConvert(QVariant::Double)) return;
		setPos(pos().x(), value.toDouble());
	} else if (property == "orientation") {
		if (!value.canConvert(QVariant::Int)) return;
		setOrientation(static_cast<QET::Orientation>(value.toInt()));
	}
}

QVariant PartTerminal::property(const QString &property) {
	if (property == "x") {
		return(scenePos().x());
	} else if (property == "y") {
		return(scenePos().y());
	} else if (property == "orientation") {
		return(_orientation);
	}
	return(QVariant());
}

QVariant PartTerminal::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (scene()) {
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemSelectedChange) {
			informations -> updateForm();
		}
	}
	return(QGraphicsItem::itemChange(change, value));
}

void PartTerminal::updateSecondPoint() {
	qreal ts = 4.0; // terminal size
	switch(_orientation) {
		case QET::North: second_point = QPointF(0.0,  ts); break;
		case QET::East : second_point = QPointF(-ts, 0.0); break;
		case QET::South: second_point = QPointF(0.0, -ts); break;
		case QET::West : second_point = QPointF(ts,  0.0); break;
	}
}
