#include "qetshapeitem.h"
#include "diagramcommands.h"


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
	update();
}

void QetShapeItem::setFullyBuilt(bool isBuilt)
{
	_isFullyBuilt = isBuilt;
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
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
		pen.setWidthF(0.5);

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

void QetShapeItem::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
	_origMousePress = mapToScene(e -> pos());
	first_move_ = true;
	if (e -> modifiers() & Qt::ControlModifier) {
		setSelected(!isSelected());
	}
	QGraphicsItem::mousePressEvent(e);
}

void QetShapeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
	if (diagram()) diagram() -> endMoveElements();
	QPointF newCoord = mapToScene(e -> pos());
	if (newCoord != _origMousePress) {
		//translate bounding rectangle
		QRectF rec = boundingRect();
		rec.translate(newCoord - _origMousePress);
		setBoundingRect(rec);
		setPos(pos() - newCoord + _origMousePress);
	}

	if (!(e -> modifiers() & Qt::ControlModifier)) QGraphicsItem::mouseReleaseEvent(e);
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

void QetShapeItem::editProperty()
{
	if (diagram() -> isReadOnly()) return;

	//the dialog
	QDialog property_dialog(diagram()->views().at(0));
	property_dialog.setWindowTitle(tr("\311diter les propri\351t\351s d'une liaison, Zone ", "window title"));
	//the main layout
	QVBoxLayout dialog_layout(&property_dialog);

	//GroupBox for resizer image
	QGroupBox restyle_groupe(tr("Shape Line Style", "shape style"));
	dialog_layout.addWidget(&restyle_groupe);
	QHBoxLayout restyle_layout(&restyle_groupe);

	QComboBox style_combo(&property_dialog);
	style_combo.addItem(tr("Normal"));
	style_combo.addItem(tr("Tiret"));
	style_combo.addItem(tr("Pointill\351"));
	style_combo.addItem(tr("Traits et points"));
	style_combo.addItem(tr("Traits points points"));

	// The items have been added in order accordance with Qt::PenStyle.
	style_combo.setCurrentIndex(int(_shapeStyle) - 1);

	restyle_layout.addWidget(&style_combo);

	//check box for disable move
	QCheckBox cb(tr("Verrouiller la position"), &property_dialog);
	cb.setChecked(!is_movable_);
	dialog_layout.addWidget(&cb);

	//dialog button, box
	QDialogButtonBox dbb(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dialog_layout.addWidget(&dbb);
	connect(&dbb, SIGNAL(accepted()), &property_dialog, SLOT(accept()));
	connect(&dbb, SIGNAL(rejected()), &property_dialog, SLOT(reject()));

	//dialog is accepted...
	if (property_dialog.exec() == QDialog::Accepted) {
		cb.isChecked() ? is_movable_=false : is_movable_=true;

		Qt::PenStyle new_style = Qt::PenStyle(style_combo.currentIndex() + 1);
		if (new_style != _shapeStyle)
			diagram()->undoStack().push(new ChangeShapeStyleCommand(this, _shapeStyle, new_style));
	}
	return;
}
