#include "qetshapeitem.h"

QetShapeItem::QetShapeItem(QPointF p1, QPointF p2, ShapeType type, bool lineAngle,QGraphicsItem *parent) :
	QetGraphicsItem(parent),
	_shapeStyle(Qt::DashLine),
	_lineAngle(lineAngle)
{
	_shapeType = type;
	_boundingRect = QRectF(p1, p2);
}

QetShapeItem::~QetShapeItem()
{
}

void QetShapeItem::setStyle(Qt::PenStyle newStyle)
{
	_shapeStyle = newStyle;
}

QRectF QetShapeItem::boundingRect() const
{
	return _boundingRect;
}

void QetShapeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	QRectF rec = boundingRect();
	QPen pen(Qt::black);
	pen.setStyle(_shapeStyle);
	painter->setPen(pen);
	switch(_shapeType) {
		case Line:
			if (_lineAngle)
				painter -> drawLine(rec.topRight(), rec.bottomLeft());
			else
				painter -> drawLine(rec.topLeft(), rec.bottomRight());
			break;
		case Rectangle:
			painter -> drawRect(rec);
			break;
		default: //(case Ellipse:)
			painter ->drawEllipse(rec);
	}
}
