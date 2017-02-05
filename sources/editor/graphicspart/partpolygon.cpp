/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "partpolygon.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "elementscene.h"


/**
 * @brief PartPolygon::PartPolygon
 * Constructor
 * @param editor : editor of this item
 * @param parent : parent item
 */
PartPolygon::PartPolygon(QETElementEditor *editor, QGraphicsItem *parent) :
	CustomElementGraphicPart(editor, parent),
	m_closed(false),
	m_handler(10),
	m_handler_index(-1),
	m_undo_command(nullptr)
{}

/**
 * @brief PartPolygon::~PartPolygon
 */
PartPolygon::~PartPolygon() {
	if(m_undo_command) delete m_undo_command;
}

/**
 * @brief PartPolygon::paint
 * Draw this polygon
 * @param painter
 * @param options
 * @param widget
 */
void PartPolygon::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *widget)
{
	Q_UNUSED(widget);

	applyStylesToQPainter(*painter);

	QPen t = painter -> pen();
	t.setCosmetic(options && options -> levelOfDetail < 1.0);
	if (isSelected()) t.setColor(Qt::red);
	painter -> setPen(t);

	m_closed ? painter -> drawPolygon (m_polygon) :
			   painter -> drawPolyline(m_polygon);

	if (m_hovered)
		drawShadowShape(painter);

	if (isSelected() && scene()->selectedItems().size() == 1)
		m_handler.drawHandler(painter, m_polygon);
}

/**
 * @brief PartPolygon::fromXml
 * Import the properties of this polygon from a xml element
 * @param qde : Xml document to use
 */
void PartPolygon::fromXml(const QDomElement &qde)
{
	stylesFromXml(qde);

	int i = 1;
	while(true)
	{
		if (QET::attributeIsAReal(qde, QString("x%1").arg(i)) &&\
			QET::attributeIsAReal(qde, QString("y%1").arg(i)))
			++ i;

		else break;
	}
	
	QPolygonF temp_polygon;
	for (int j = 1 ; j < i ; ++ j)
	{
		temp_polygon << QPointF(qde.attribute(QString("x%1").arg(j)).toDouble(),
								qde.attribute(QString("y%1").arg(j)).toDouble());
	}
	m_polygon = temp_polygon;
	
	m_closed = qde.attribute("closed") != "false";
}

/**
 * @brief PartPolygon::toXml
 * Export this polygin in xml
 * @param xml_document : Xml document to use for create the xml element
 * @return an xml element that describe this polygon
 */
const QDomElement PartPolygon::toXml(QDomDocument &xml_document) const
{
	QDomElement xml_element = xml_document.createElement("polygon");
	int i = 1;
	foreach(QPointF point, m_polygon) {
		point = mapToScene(point);
		xml_element.setAttribute(QString("x%1").arg(i), QString("%1").arg(point.x()));
		xml_element.setAttribute(QString("y%1").arg(i), QString("%1").arg(point.y()));
		++ i;
	}
	if (!m_closed) xml_element.setAttribute("closed", "false");
	stylesToXml(xml_element);
	return(xml_element);
}

/**
 * @brief PartPolygon::isUseless
 * @return true if this part is irrelevant and does not deserve to be Retained / registered.
 * A polygon is relevant when he have 2 differents points
 */
bool PartPolygon::isUseless() const
{
	if (m_polygon.count() < 2) return(true);

	for (int i = 1 ; i < m_polygon.count() ; ++ i)
		if (m_polygon[i] != m_polygon[i-1]) return(false);

	return(true);
}

/**
 * @brief PartPolygon::sceneGeometricRect
 * @return the minimum, margin-less rectangle this part can fit into, in scene
 * coordinates. It is different from boundingRect() because it is not supposed
 * to imply any margin, and it is different from shape because it is a regular
 * rectangle, not a complex shape.
 */
QRectF PartPolygon::sceneGeometricRect() const {
	return(mapToScene(m_polygon.boundingRect()).boundingRect());
}

/**
 * @brief PartPolygon::startUserTransformation
 * Start the user-induced transformation, provided this primitive is contained
 * within the initial_selection_rect bounding rectangle.
 * @param initial_selection_rect
 */
void PartPolygon::startUserTransformation(const QRectF &initial_selection_rect)
{
	Q_UNUSED(initial_selection_rect)
	saved_points_ = mapToScene(m_polygon).toList();
}

/**
 * @brief PartPolygon::handleUserTransformation
 * Handle the user-induced transformation from initial_selection_rect to new_selection_rect
 * @param initial_selection_rect
 * @param new_selection_rect
 */
void PartPolygon::handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect)
{
	QList<QPointF> mapped_points = mapPoints(initial_selection_rect, new_selection_rect, saved_points_);
	m_polygon = (mapFromScene(QPolygonF(mapped_points.toVector())));
}

/**
 * @brief PartPolygon::preferredScalingMethod
 * This method is called by the decorator when it needs to determine the best
 * way to interactively scale a primitive. It is typically called when only a
 * single primitive is being scaled.
 * @return : This reimplementation systematically returns QET::RoundScaleRatios.
 */
QET::ScalingMethod PartPolygon::preferredScalingMethod() const {
	return(QET::RoundScaleRatios);
}

/**
 * @brief PartPolygon::polygon
 * @return the item's polygon, or an empty polygon if no polygon has been set.
 */
QPolygonF PartPolygon::polygon() const {
	return m_polygon;
}

/**
 * @brief PartPolygon::setPolygon
 * Sets the item's polygon to be the given polygon.
 * @param polygon
 */
void PartPolygon::setPolygon(const QPolygonF &polygon)
{
	if (m_polygon == polygon) return;
	prepareGeometryChange();
	m_polygon = polygon;
	emit polygonChanged();
}

/**
 * @brief PartPolygon::addPoint
 * Add new point to polygon
 * @param point
 */
void PartPolygon::addPoint(const QPointF &point)
{
	prepareGeometryChange();
	m_polygon << point;
}

/**
 * @brief PartPolygon::setLastPoint
 * Set the last point of polygon to @point
 * @param point
 */
void PartPolygon::setLastPoint(const QPointF &point)
{
	if (m_polygon.size())
		m_polygon.pop_back();

	prepareGeometryChange();
	m_polygon << point;
}

/**
 * @brief PartPolygon::removeLastPoint
 * Remove the last point of polygon
 */
void PartPolygon::removeLastPoint()
{
	if (m_polygon.size())
	{
		prepareGeometryChange();
		m_polygon.pop_back();
	}
}

void PartPolygon::setClosed(bool close)
{
	if (m_closed == close) return;
	prepareGeometryChange();
	m_closed = close;
	emit closedChange();
}

void PartPolygon::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
	if (!isSelected())
	{
		CustomElementGraphicPart::hoverMoveEvent(event);
		return;
	}

	if (m_handler.pointIsHoverHandler(event->pos(), m_polygon) >= 0)
		setCursor(Qt::SizeAllCursor);
	else
		CustomElementGraphicPart::hoverMoveEvent(event);
}

/**
 * @brief PartPolygon::mousePressEvent
 * Handle mouse press event
 * @param event
 */
void PartPolygon::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		setCursor(Qt::ClosedHandCursor);
		if(isSelected())
		{
			m_handler_index = m_handler.pointIsHoverHandler(event->pos(), m_polygon);

			if(m_handler_index >= 0) //User click on an handler
			{
				m_undo_command = new QPropertyUndoCommand(this, "polygon", QVariant(m_polygon));
				m_undo_command->setText(tr("Modifier un polygone"));
				return;
			}
		}
	}

	CustomElementGraphicPart::mousePressEvent(event);
}

/**
 * @brief PartPolygon::mouseMoveEvent
 * Handle mouse move event
 * @param event
 */
void PartPolygon::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if(m_handler_index >= 0)
	{
		QPointF pos_ = event->modifiers() == Qt::ControlModifier ? event->pos() : mapFromScene(elementScene()->snapToGrid(event->scenePos()));
		prepareGeometryChange();
		m_polygon.replace(m_handler_index, pos_);
		emit polygonChanged();
	}
	else
		CustomElementGraphicPart::mouseMoveEvent(event);
}

/**
 * @brief PartPolygon::mouseReleaseEvent
 * Handle mouse release event
 * @param event
 */
void PartPolygon::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		setCursor(Qt::OpenHandCursor);

	if (m_handler_index >= 0)
	{
		m_undo_command->setNewValue(QVariant(m_polygon));
		elementScene()->undoStack().push(m_undo_command);
		m_undo_command = nullptr;
		m_handler_index = -1;
	}
	else
		CustomElementGraphicPart::mouseReleaseEvent(event);
}

/**
 * @brief PartPolygon::shape
 * @return the shape of this item
 */
QPainterPath PartPolygon::shape() const
{
	QPainterPath shape;
	shape.addPolygon(m_polygon);

	if (m_closed)
		shape.lineTo(m_polygon.first());

	QPainterPathStroker pps;
	pps.setWidth(m_hovered? penWeight()+SHADOWS_HEIGHT : penWeight());
	shape = pps.createStroke(shape);

	if (isSelected())
		foreach(QRectF rect, m_handler.handlerRect(m_polygon))
			shape.addRect(rect);

	return shape;
}

QPainterPath PartPolygon::shadowShape() const
{
	QPainterPath shape;
	shape.addPolygon(m_polygon);

	if (m_closed)
		shape.lineTo(m_polygon.first());

	QPainterPathStroker pps;
	pps.setWidth(penWeight());

	return (pps.createStroke(shape));
}

/**
 * @brief PartPolygon::boundingRect
 * @return the bounding rect of this item
 */
QRectF PartPolygon::boundingRect() const
{
	QRectF r = m_polygon.boundingRect();

	qreal adjust = (SHADOWS_HEIGHT + penWeight()) / 2;
		//We add 0.5 because CustomElementGraphicPart::drawShadowShape
		//draw a shape bigger of 0.5 when pen weight is to 0.
	if (penWeight() == 0) adjust += 0.5;

	r.adjust(-adjust, -adjust, adjust, adjust);

	foreach(QRectF rect, m_handler.handlerRect(m_polygon))
		r |=rect;

	return(r);
}
