/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "qetshapeitem.h"
#include "diagramcommands.h"
#include "createdxf.h"
#include "diagram.h"
#include "qet.h"


/**
 * @brief QetShapeItem::QetShapeItem
 * Constructor of shape item. point 1 and 2 must be in scene coordinate
 * @param p1 first point
 * @param p2 second point
 * @param type type of item (line, rectangle, ellipse)
 * @param parent parent item
 */
QetShapeItem::QetShapeItem(QPointF p1, QPointF p2, ShapeType type, QGraphicsItem *parent) :
	QetGraphicsItem(parent),
	m_shapeType(type),
	m_shapeStyle(Qt::DashLine),
	m_P1 (Diagram::snapToGrid(p1)),
	m_P2 (Diagram::snapToGrid(p2))

{
	if (type == Polyline) m_polygon << m_P1 << m_P2;
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
}

QetShapeItem::~QetShapeItem()
{
}

/**
 * @brief QetShapeItem::setStyle
 * Set the new style of pen for thi item
 * @param newStyle
 */
void QetShapeItem::setStyle(Qt::PenStyle newStyle)
{
	m_shapeStyle = newStyle;
	update();
}

/**
 * @brief QetShapeItem::scale
 * Scale this item by (factor / 100 ).
 * @param factor
 */
void QetShapeItem::previewScale(int factor) {
	if (factor >= 1 && factor <= 200) {
		qreal new_scale = factor;
		new_scale /= 100;
		setScale(new_scale);
	}
}

/**
 * @brief QetShapeItem::setP2
 * Set the second point of this item.
 * If this item is a polyline,
 * the last point of the polyline is replaced by P2.
 * @param P2
 */
void QetShapeItem::setP2(QPointF P2) {
	P2 = Diagram::snapToGrid(P2);

	if (m_shapeType == Polyline) {
		prepareGeometryChange();
		m_polygon.replace(m_polygon.size()-1, P2);
	}
	else {
		if (P2 == m_P2) return;
		prepareGeometryChange();
		m_P2 = P2;
	}
	setTransformOriginPoint(boundingRect().center());
}

/**
 * @brief QetShapeItem::setNextPoint
 * Add a new point to the curent polygon
 * @param P the new point.
 */
void QetShapeItem::setNextPoint(QPointF P) {
	prepareGeometryChange();
	m_polygon.append(Diagram::snapToGrid(P));
	setTransformOriginPoint(boundingRect().center());
}

/**
 * @brief QetShapeItem::boundingRect
 * @return the bounding rect of this item
 */
QRectF QetShapeItem::boundingRect() const {
	if (m_shapeType == Polyline)
		return ( shape().boundingRect());

	QRectF b(m_P1, m_P2);
	return b.normalized();
}

/**
 * @brief QetShapeItem::shape
 * @return the shape of this item
 */
QPainterPath QetShapeItem::shape() const {
	QPainterPath path;
	QPainterPathStroker pps;

	switch (m_shapeType) {
		case Line:
			path.moveTo(m_P1);
			path.lineTo(m_P2);
			pps.setWidth(10);
			path = pps.createStroke(path);
			break;
		case Rectangle:
			path.addRect(boundingRect());
			break;
		case Ellipse:
			path.addEllipse(boundingRect());
			break;
		case Polyline:
			path.addPolygon(m_polygon);
			break;
		default:
			Q_ASSERT(false);
			break;
	}

	return path;
}

/**
 * @brief QetShapeItem::changeGraphicsItem
 * Change the curent type of this item to newtype
 * @param newtype
 */
void QetShapeItem::changeGraphicsItem(const ShapeType &newtype) {
	if (newtype == m_shapeType) return;
	prepareGeometryChange();
	m_shapeType = newtype;
	setTransformOriginPoint(boundingRect().center());
}

/**
 * @brief QetShapeItem::paint
 * Paint this item
 * @param painter
 * @param option
 * @param widget
 */
void QetShapeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option); Q_UNUSED(widget);

	QPen pen;
	pen.setStyle(m_shapeStyle);
	if (isSelected()) pen.setColor(Qt::red);
	painter -> setRenderHint(QPainter::Antialiasing, false);
	pen.setWidthF(1);
	painter->setPen(pen);



	switch (m_shapeType) {
		case Line:
			painter->drawLine(QLineF(m_P1, m_P2));
			break;
		case Rectangle:
			painter->drawRect(boundingRect());
			break;
		case Ellipse:
			painter->drawEllipse(boundingRect());
			break;
		case Polyline:
			painter->drawPolyline(m_polygon);
			break;
	}
}

/**
 * @brief QetShapeItem::fromXml
 * Build this item from the xml description
 * @param e element where is stored this item
 * @return true if load success
 */
bool QetShapeItem::fromXml(const QDomElement &e) {
	if (e.tagName() != "shape") return (false);

	is_movable_ = e.attribute("is_movable", "true").toInt ();
	m_shapeStyle = Qt::PenStyle(e.attribute("style","0").toInt());

	if (e.attribute("type", "0").toInt() != Polyline) {
		m_P1.setX(e.attribute("x1", 0).toDouble());
		m_P1.setY(e.attribute("y1", 0).toDouble());
		m_P2.setX(e.attribute("x2", 0).toDouble());
		m_P2.setY(e.attribute("y2", 0).toDouble());
	}

	else {
		foreach(QDomElement de, QET::findInDomElement(e, "points", "point")) {
			m_polygon << QPointF(de.attribute("x", 0).toDouble(), de.attribute("y", 0).toDouble());
		}
	}

	changeGraphicsItem(QetShapeItem::ShapeType(e.attribute("type","0").toInt()));
	return (true);
}

/**
 * @brief QetShapeItem::toXml
 * Save this item to xml element
 * @param document parent document xml
 * @return element xml where is write this item
 */
QDomElement QetShapeItem::toXml(QDomDocument &document) const {
	QDomElement result = document.createElement("shape");

	//write some attribute
	result.setAttribute("type", QString::number(m_shapeType));
	result.setAttribute("style", QString::number(m_shapeStyle));
	result.setAttribute("is_movable", bool(is_movable_));

	if (m_shapeType != Polyline) {
		result.setAttribute("x1", mapToScene(m_P1).x());
		result.setAttribute("y1", mapToScene(m_P1).y());
		result.setAttribute("x2", mapToScene(m_P2).x());
		result.setAttribute("y2", mapToScene(m_P2).y());
	}

	else {
		QDomElement points = document.createElement("points");
		foreach(QPointF p, m_polygon) {
			QDomElement point = document.createElement("point");
			QPointF pf = mapToScene(p);
			point.setAttribute("x", pf.x());
			point.setAttribute("y", pf.y());
			points.appendChild(point);
		}
		result.appendChild(points);
	}

	return(result);
}

/**
 * @brief QetShapeItem::toDXF
 * Draw this element to the dxf document
 * @param filepath file path of the the dxf document
 * @return true if draw success
 */
bool QetShapeItem::toDXF(const QString &filepath) {
	switch (m_shapeType) {
		case Line:
			Createdxf::drawLine(filepath, QLineF(mapToScene(m_P1), mapToScene(m_P2)), 0);
			return true;
			break;
		case Rectangle:
			Createdxf::drawRectangle(filepath, QRectF(mapToScene(m_P1), mapToScene(m_P2)).normalized(), 0);
			return true;
			break;
		case Ellipse:
			Createdxf::drawEllipse(filepath, QRectF(mapToScene(m_P1), mapToScene(m_P2)).normalized(), 0);
			return true;
			break;
		default:
			return false;
			break;
	}
}

/**
 * @brief QetShapeItem::editProperty
 * Edit the property of this item
 */
void QetShapeItem::editProperty()
{
	if (diagram() -> isReadOnly()) return;

	//the dialog
	QDialog property_dialog(diagram()->views().at(0));
	property_dialog.setWindowTitle(tr("\311diter les propri\351t\351s d'une shape, Zone ", "window title"));
	//the main layout
	QVBoxLayout dialog_layout(&property_dialog);

	//GroupBox for resizer image
	QGroupBox restyle_groupe(QObject::tr("Type de trait", "shape style"));
	dialog_layout.addWidget(&restyle_groupe);
	QHBoxLayout restyle_layout(&restyle_groupe);

	QComboBox style_combo(&property_dialog);
	style_combo.addItem(QObject::tr("Normal"));
	style_combo.addItem(QObject::tr("Tiret"));
	style_combo.addItem(QObject::tr("Pointill\351"));
	style_combo.addItem(QObject::tr("Traits et points"));
	style_combo.addItem(QObject::tr("Traits points points"));

	// The items have been added in order accordance with Qt::PenStyle.
	style_combo.setCurrentIndex(int(m_shapeStyle) - 1);

	restyle_layout.addWidget(&style_combo);

	//check box for disable move
	QCheckBox cb(tr("Verrouiller la position"), &property_dialog);
	cb.setChecked(!is_movable_);
	dialog_layout.addWidget(&cb);

	//GroupBox for Scaling
	QGroupBox scale_groupe(QObject::tr("\311chelle", "shape scale"));
	dialog_layout.addWidget(&scale_groupe);
	QHBoxLayout scale_layout(&scale_groupe);

	int min_range = 1;
	int max_range = 200;
	int factor_range = 100;

		//slider
	QSlider slider(Qt::Horizontal, &property_dialog);
	slider.setRange(min_range, max_range);
	qreal scale_= scale();
	slider.setValue(scale_*factor_range);
		//spinbox
	QSpinBox spin_box(&property_dialog);
	spin_box.setRange(min_range, max_range);
	spin_box.setValue(scale_*factor_range);
	spin_box.setSuffix(" %");
		//synchro slider with spinbox
	connect(&slider, SIGNAL(valueChanged(int)), &spin_box, SLOT(setValue(int)));
	connect(&slider, SIGNAL(valueChanged(int)), this, SLOT(previewScale(int)));
	connect(&spin_box, SIGNAL(valueChanged(int)), &slider, SLOT(setValue(int)));
		//add slider and spinbox to layout
	scale_layout.addWidget(&slider);
	scale_layout.addWidget(&spin_box);

	//dialog button, box
	QDialogButtonBox dbb(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dialog_layout.addWidget(&dbb);
	connect(&dbb, SIGNAL(accepted()), &property_dialog, SLOT(accept()));
	connect(&dbb, SIGNAL(rejected()), &property_dialog, SLOT(reject()));

	//dialog is accepted...
	if (property_dialog.exec() == QDialog::Accepted) {
		cb.isChecked() ? is_movable_=false : is_movable_=true;

		Qt::PenStyle new_style = Qt::PenStyle(style_combo.currentIndex() + 1);
		if (new_style != m_shapeStyle) diagram()->undoStack().push(new ChangeShapeStyleCommand(this, m_shapeStyle, new_style));

		qreal scale_factor = slider.value();
		scale_factor /= factor_range;
		if (scale_ != scale_factor) diagram()->undoStack().push(new ItemResizerCommand(this, scale_, scale_factor, tr("une shape")));
		return;
	}
	//...or not
	setScale(scale_);
}

/**
 * @brief QetShapeItem::name
 * @return the name of the curent shape.
 */
QString QetShapeItem::name() const {
	switch (m_shapeType) {
		case Line:
			return tr("une ligne");
			break;
		case Rectangle:
			return tr("un rectangle");
			break;
		case Ellipse:
			return tr("une \351llipse");
			break;
		case Polyline:
			return tr("une polyligne");
			break;
		default:
			return tr("une shape");
			break;
	}
}
