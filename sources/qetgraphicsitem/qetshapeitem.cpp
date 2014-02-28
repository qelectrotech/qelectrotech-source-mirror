#include "qetshapeitem.h"


QetShapeItem::QetShapeItem(QPointF p1, QPointF p2, ShapeType type, bool lineAngle,QGraphicsItem *parent) :
	QetGraphicsItem(parent),
	_shapeStyle(Qt::DashLine),
	_lineAngle(lineAngle),
	_isFullyBuilt(false),
	_writingXml(false)
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

void QetShapeItem::setFullyBuilt(bool isBuilt)
{
	_isFullyBuilt = isBuilt;
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
}

QLineF *QetShapeItem::getLine()
{
	QRectF rect = boundingRect();
	QLineF *line = 0;
	if (_shapeType == Line) {
		if (_lineAngle)
			line = new QLineF(rect.topRight(), rect.bottomLeft());
		else
			line = new QLineF(rect.topLeft(), rect.bottomRight());
	}
	return line;
}

QRectF *QetShapeItem::getRectangle()
{
	QRectF rect = boundingRect();
	QRectF *rec = 0;
	if (_shapeType == Rectangle)
		rec = new QRectF(rect);
	return rec;
}

QRectF *QetShapeItem::getEllipse()
{
	QRectF rect = boundingRect();
	QRectF *rec = 0;
	if (_shapeType == Ellipse)
		rec = new QRectF(rect);
	return rec;
}

QRectF QetShapeItem::boundingRect() const
{
	return _boundingRect;
}

QPainterPath QetShapeItem::shape() const
{
	QPainterPath path;
	QPainterPathStroker pps;
	QRectF rect = boundingRect();

	switch (_shapeType) {
		case Line:
			if (_lineAngle) {
				path.moveTo(rect.topRight());
				path.lineTo(rect.bottomLeft());
			} else {
				path.moveTo(rect.topLeft());
				path.lineTo(rect.bottomRight());
			}
			//use @pps for grab line with bigger outerline
			//more usefull
			pps.setWidth(10);
			path= pps.createStroke(path);
			break;
		case Rectangle:
			path = QetGraphicsItem::shape();
			break;
		case Ellipse:
			path.addEllipse(rect);
			break;
		default:
			path = QetGraphicsItem::shape();
	}

	return path;
}

void QetShapeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (!_writingXml) {
		painter -> setRenderHint(QPainter::Antialiasing, false);
		QRectF rec = boundingRect();
		QPen pen(Qt::black);
		if (isSelected())
			pen.setColor(Qt::red);
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
}


bool QetShapeItem::fromXml(const QDomElement &e)
{
	if (!_writingXml) {
		if (e.tagName() != "shape") return (false);

		_shapeType = QetShapeItem::ShapeType(e.attribute("type","0").toInt());
		_shapeStyle = Qt::PenStyle(e.attribute("style","0").toInt());
		_lineAngle = e.attribute("lineAngle","0").toInt();
		qreal x = e.attribute("x","0").toDouble();
		qreal y = e.attribute("y","0").toDouble();
		qreal w = e.attribute("w","0").toDouble();
		qreal h = e.attribute("h","0").toDouble();
		setBoundingRect(QRectF(x, y, w, h));
		setFullyBuilt(true);
	}

	return (true);
}


QDomElement QetShapeItem::toXml(QDomDocument &document) const {
	QDomElement result = document.createElement("shape");
	QRectF rec = boundingRect();

	//write some attribute
	result.setAttribute("type", QString::number(_shapeType));
	result.setAttribute("x", QString::number(rec.topLeft().x()));
	result.setAttribute("y", QString::number(rec.topLeft().y()));
	result.setAttribute("w", QString::number(rec.width()));
	result.setAttribute("h", QString::number(rec.height()));
	result.setAttribute("lineAngle", QString::number(_lineAngle));
	result.setAttribute("style", QString::number(_shapeStyle));

	return(result);
}
