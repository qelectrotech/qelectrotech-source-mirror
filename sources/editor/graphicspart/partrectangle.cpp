/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "partrectangle.h"

#include "../../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../../QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "../../QetGraphicsItemModeler/qetgraphicshandlerutility.h"
#include "../elementscene.h"

/**
	@brief PartRectangle::PartRectangle
	Constructor
	@param editor the QETElementEditor of this item
	@param parent parent item
*/
PartRectangle::PartRectangle(QETElementEditor *editor, QGraphicsItem *parent) :
	CustomElementGraphicPart(editor, parent)
{
	m_rot=0;
}

/**
	@brief PartRectangle::~PartRectangle
*/
PartRectangle::~PartRectangle()
{
	removeHandler();
}

/**
	@brief PartRectangle::paint
	Draw this Rectangle
	@param painter
	@param options
	@param widget
*/
void PartRectangle::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *widget)
{
	Q_UNUSED(widget);
	applyStylesToQPainter(*painter);
	QPen t = painter -> pen();
	t.setCosmetic(options && options -> levelOfDetailFromTransform(painter->worldTransform()) < 1.0);
	if (isSelected())
		t.setColor(Qt::red);

	t.setJoinStyle(Qt::MiterJoin);

		//Force the pen to width 0 if one of dimension is null
	if (!rect().width() || !rect().height())
		t.setWidth(0);

	painter->setPen(t);
	painter->drawRoundedRect(m_rect, m_xRadius, m_yRadius);

	if (m_hovered)
		drawShadowShape(painter);

	if (isSelected())
		drawCross(m_rect.center(), painter);
}

/**
	@brief PartRectangle::toXml
	Export this rectangle in xml
	@param xml_document : Xml document to use for create the xml element.
	@return an xml element that describe this ellipse
*/
const QDomElement PartRectangle::toXml(QDomDocument &xml_document) const
{
	QDomElement xml_element = xml_document.createElement("rect");
	QPointF top_left(sceneTopLeft());
	qreal x  = (qRound(top_left.x() * 100.0) / 100.0);
	qreal y  = (qRound(top_left.y() * 100.0) / 100.0);
	qreal w  = (qRound(m_rect.width() * 100.0) / 100.0);
	qreal h  = (qRound(m_rect.height() * 100.0) / 100.0);
	qreal rx = (qRound(m_xRadius * 100.0) / 100.0);
	qreal ry = (qRound(m_yRadius * 100.0) / 100.0);

	xml_element.setAttribute("x", QString::number(x));
	xml_element.setAttribute("y", QString::number(y));
	xml_element.setAttribute("width",  QString::number(w));
	xml_element.setAttribute("height", QString::number(h));
	xml_element.setAttribute("rx", QString::number(rx));
	xml_element.setAttribute("ry", QString::number(ry));

	stylesToXml(xml_element);
	return(xml_element);
}

/**
	@brief PartRectangle::fromXml
	Import the properties of this rectangle from a xml element.
	@param qde : Xml document to use.
*/
void PartRectangle::fromXml(const QDomElement &qde)
{
	stylesFromXml(qde);
	setPos(mapFromScene(qde.attribute("x", "0").toDouble(),
						qde.attribute("y", "0").toDouble()));

	QRectF rect(QPointF(0,0), QSizeF(qde.attribute("width",  "0").toDouble(),
									 qde.attribute("height", "0").toDouble()));

	setRect(rect.normalized());
	setXRadius(qde.attribute("rx", "0").toDouble());
	setYRadius(qde.attribute("ry", "0").toDouble());
}

/**
	@brief PartRectangle::rect
	@return : Returns the item's rectangle.
*/
QRectF PartRectangle::rect() const
{
	return m_rect;
}

/**
	@brief PartRectangle::setRect
	Sets the item's rectangle to be the given rectangle.
	@param rect
*/
void PartRectangle::setRect(const QRectF &rect)
{
	if (rect == m_rect) return;
	prepareGeometryChange();
	m_rect = rect;
	adjustHandlerPos();
	emit rectChanged();
}

void PartRectangle::setXRadius(qreal X)
{
	m_xRadius = X;
	update();
	adjustHandlerPos();
	emit XRadiusChanged();
}

void PartRectangle::setYRadius(qreal Y)
{
	m_yRadius = Y;
	update();
	adjustHandlerPos();
	emit YRadiusChanged();
}

void PartRectangle::setRotation(qreal angle) {
	qreal diffAngle = qRound((angle - rotation()) * 100.0) / 100.0;
	m_rot = QET::correctAngle(angle, true);
	auto p1 = mapToScene(m_rect.x(),m_rect.y());
	qreal width  = m_rect.height();
	qreal height = m_rect.width();
	qreal x; qreal y;
	if (diffAngle > 0) {
		x = (p1.y() + m_rect.height()) * (-1);
		y = p1.x();
	} else {
		x = p1.y();
		y = (p1.x() + m_rect.width()) * (-1);
	}
	p1 = mapFromScene(x, y);
	m_rect = QRectF(p1.x(), p1.y(), width, height);
	std::swap (m_xRadius, m_yRadius);

	prepareGeometryChange();
	adjustHandlerPos();
	emit rectChanged();
}

qreal PartRectangle::rotation() const {
	return qRound(m_rot * 100.0) / 100.0;
}

void PartRectangle::flip() {
	auto height = m_rect.height();
	auto p1 = mapToScene(m_rect.x(),m_rect.y());
	qreal x = p1.x();
	qreal y = ((-1.0) * p1.y()) - height;
	p1 = mapFromScene(x, y);
	m_rect.setX(p1.x());
	m_rect.setY(p1.y());
	m_rect.setHeight(height);
	prepareGeometryChange();
	adjustHandlerPos();
	emit rectChanged();
}

void PartRectangle::mirror() {
	auto width = m_rect.width();
	auto p1 = mapToScene(m_rect.x(),m_rect.y());
	qreal x = ((-1.0) * p1.x()) - width;
	qreal y = p1.y();
	p1 = mapFromScene(x, y);
	m_rect.setX(p1.x());
	m_rect.setY(p1.y());
	m_rect.setWidth(width);
	prepareGeometryChange();
	adjustHandlerPos();
	emit rectChanged();
}


/**
	@brief PartRectangle::sceneGeometricRect
	@return the minimum, margin-less rectangle this part can fit into, in scene
	coordinates. It is different from boundingRect() because it is not supposed
	to imply any margin, and it is different from shape because it is a regular
	rectangle, not a complex shape.
*/
QRectF PartRectangle::sceneGeometricRect() const
{
	return(mapToScene(rect()).boundingRect());
}

/**
	@brief PartRectangle::sceneTopLeft
	@return the top left of rectangle, in scene coordinate
*/
QPointF PartRectangle::sceneTopLeft() const
{
	return(mapToScene(rect().topLeft()));
}

/**
	@brief PartRectangle::shape
	@return the shape of this item
*/
QPainterPath PartRectangle::shape() const
{
	QPainterPath shape;
	shape.addRoundedRect(m_rect, m_xRadius, m_yRadius);

	QPainterPathStroker pps;
	pps.setWidth(m_hovered? penWeight()+SHADOWS_HEIGHT : penWeight());
	shape = pps.createStroke(shape);

	return shape;
}

QPainterPath PartRectangle::shadowShape() const
{
	QPainterPath shape;
	shape.addRoundedRect(m_rect, m_xRadius, m_yRadius);

	QPainterPathStroker pps;
	pps.setWidth(penWeight());

	return (pps.createStroke(shape));
}

/**
	@brief PartRectangle::boundingRect
	@return Bounding rectangle this part can fit into
*/
QRectF PartRectangle::boundingRect() const
{
	qreal adjust = (SHADOWS_HEIGHT + penWeight()) / 2;
		//We add 0.5 because CustomElementGraphicPart::drawShadowShape
		//draw a shape bigger of 0.5 when pen weight is to 0.
	if (penWeight() == 0) adjust += 0.5;

	QRectF r = m_rect.normalized();
	r.adjust(-adjust, -adjust, adjust, adjust);

	return(r);
}

/**
	@brief PartRectangle::isUseless
	@return true if this part is irrelevant and does not deserve to be Retained / registered.
	An rectangle is relevant when he's not null.
*/
bool PartRectangle::isUseless() const
{
	return(rect().isNull());
}

/**
	@brief PartRectangle::startUserTransformation
	Start the user-induced transformation, provided this primitive is contained
	within the initial_selection_rect bounding rectangle.
	@param initial_selection_rect
*/
void PartRectangle::startUserTransformation(const QRectF &initial_selection_rect)
{
	Q_UNUSED(initial_selection_rect)
		// we keep track of our own rectangle at the moment in scene coordinates too
	saved_points_.clear();
	saved_points_ << mapToScene(rect().topLeft()) << mapToScene(rect().bottomRight());
}

/**
	@brief PartRectangle::handleUserTransformation
	Handle the user-induced transformation from \a initial_selection_rect to \a new_selection_rect
	@param initial_selection_rect
	@param new_selection_rect
*/
void PartRectangle::handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect)
{
	QList<QPointF> mapped_points = mapPoints(initial_selection_rect, new_selection_rect, saved_points_);
	setRect(QRectF(mapFromScene(mapped_points.at(0)), mapFromScene(mapped_points.at(1))));
}

/**
	@brief PartRectangle::mouseReleaseEvent
	Handle mouse release event
	@param event
*/
void PartRectangle::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && event->buttonDownPos(Qt::LeftButton) == event->pos())
		switchResizeMode();

	CustomElementGraphicPart::mouseReleaseEvent(event);
}

/**
	@brief PartRectangle::itemChange
	@param change
	@param value
	@return
*/
QVariant PartRectangle::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemPositionHasChanged)
	{
		adjustHandlerPos();
	}
	else if (change == ItemSceneChange)
	{
		setSelected(false); //This item is removed from scene, then we deselect this, and so, the handlers is also removed.
	}

	return QGraphicsItem::itemChange(change, value);
}

/**
	@brief PartRectangle::sceneEventFilter
	@param watched
	@param event
	@return
*/
bool PartRectangle::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
		//Watched must be an handler
	if(watched->type() == QetGraphicsHandlerItem::Type)
	{
		QetGraphicsHandlerItem *qghi = qgraphicsitem_cast<QetGraphicsHandlerItem *>(watched);

		if(m_handler_vector.contains(qghi)) //Handler must be in m_vector_index, then we can start resize
		{
			m_vector_index = m_handler_vector.indexOf(qghi);
			if (m_vector_index != -1)
			{
				if(event->type() == QEvent::GraphicsSceneMousePress) //Click
				{
					handlerMousePressEvent(qghi, static_cast<QGraphicsSceneMouseEvent *>(event));
					return true;
				}
				else if(event->type() == QEvent::GraphicsSceneMouseMove) //Move
				{
					handlerMouseMoveEvent(qghi, static_cast<QGraphicsSceneMouseEvent *>(event));
					return true;
				}
				else if (event->type() == QEvent::GraphicsSceneMouseRelease) //Release
				{
					handlerMouseReleaseEvent(qghi, static_cast<QGraphicsSceneMouseEvent *>(event));
					return true;
				}
			}
		}
	}

	return false;
}

/**
	@brief PartRectangle::switchResizeMode
*/
void PartRectangle::switchResizeMode()
{
	if (m_resize_mode == 1)
	{
		m_resize_mode = 2;
		for (QetGraphicsHandlerItem *qghi : m_handler_vector)
			qghi->setColor(Qt::darkGreen);
	}
	else if (m_resize_mode == 2)
	{
		m_resize_mode = 3;
		qDeleteAll(m_handler_vector);
		m_handler_vector.clear();
		addHandler();
		for (QetGraphicsHandlerItem *qghi : m_handler_vector) {
			qghi->setColor(Qt::magenta);
		}
	}
	else if (m_resize_mode == 3)
	{
		m_resize_mode = 1;
		qDeleteAll(m_handler_vector);
		m_handler_vector.clear();
		addHandler();
		for (QetGraphicsHandlerItem *qghi : m_handler_vector) {
			qghi->setColor(Qt::blue);
		}
	}
}

/**
	@brief PartRectangle::adjustHandlerPos
*/
void PartRectangle::adjustHandlerPos()
{
	if (m_handler_vector.isEmpty()) {
		return;
	}

	QVector <QPointF> points_vector;

	if(m_resize_mode != 3) {
		points_vector = QetGraphicsHandlerUtility::pointsForRect(m_rect);
	}
	else {
		points_vector = QetGraphicsHandlerUtility::pointForRadiusRect(m_rect, m_xRadius, m_yRadius);
	}

	if (m_handler_vector.size() == points_vector.size())
	{
		points_vector = mapToScene(points_vector);
		for (int i = 0 ; i < points_vector.size() ; ++i)
			m_handler_vector.at(i)->setPos(points_vector.at(i));
	}
	else
	{
		qDeleteAll(m_handler_vector);
		m_handler_vector.clear();
		addHandler();
	}
}

/**
	@brief PartRectangle::handlerMousePressEvent
	@param qghi
	@param event
*/
void PartRectangle::handlerMousePressEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi)
	Q_UNUSED(event)

	m_old_rect = m_rect;
	m_old_xRadius = m_xRadius;
	m_old_yRadius = m_yRadius;
	if(m_xRadius == 0 && m_yRadius == 0) {
		m_modifie_radius_equaly = true;
	}
}

/**
	@brief PartRectangle::handlerMouseMoveEvent
	@param qghi
	@param event
*/
void PartRectangle::handlerMouseMoveEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi)

	QPointF new_pos = event->scenePos();
	if (event->modifiers() != Qt::ControlModifier)
		new_pos = elementScene()->snapToGrid(event->scenePos());
	new_pos = mapFromScene(new_pos);

	if (m_resize_mode == 1)
		setRect(QetGraphicsHandlerUtility::rectForPosAtIndex(m_rect, new_pos, m_vector_index));
	else if (m_resize_mode == 2)
		setRect(QetGraphicsHandlerUtility::mirrorRectForPosAtIndex(m_rect, new_pos, m_vector_index));
	else
	{
		qreal radius = QetGraphicsHandlerUtility::radiusForPosAtIndex(m_rect, new_pos, m_vector_index);
		if(m_modifie_radius_equaly) {
			setXRadius(radius);
			setYRadius(radius);
		}
		else if(m_vector_index == 0) {
			setXRadius(radius);
		}
		else {
			setYRadius(radius);
		}
	}

	adjustHandlerPos();
}

void PartRectangle::handlerMouseReleaseEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi)
	Q_UNUSED(event)

	m_modifie_radius_equaly = false;

	QUndoCommand *undo = new QUndoCommand("Modifier un rectangle");
	if (m_old_rect != m_rect) {
		QPropertyUndoCommand *u = new QPropertyUndoCommand(this, "rect", QVariant(m_old_rect.normalized()), QVariant(m_rect.normalized()), undo);
		u->setAnimated(true, false);
	}
	if (m_old_xRadius != m_xRadius) {
		QPropertyUndoCommand *u = new QPropertyUndoCommand(this, "xRadius", QVariant(m_old_xRadius), QVariant(m_xRadius), undo);
		u->setAnimated();
	}
	if (m_old_yRadius != m_yRadius) {
		QPropertyUndoCommand *u = new QPropertyUndoCommand(this, "yRadius", QVariant(m_old_yRadius), QVariant(m_yRadius), undo);
		u->setAnimated();
	}

	elementScene()->undoStack().push(undo);
	m_vector_index = -1;
}

/**
	@brief PartRectangle::addHandler
	Add handlers for this item
*/
void PartRectangle::addHandler()
{
	if (m_handler_vector.isEmpty() && scene())
	{
		if (m_resize_mode != 3) {
			m_handler_vector = QetGraphicsHandlerItem::handlerForPoint(mapToScene(QetGraphicsHandlerUtility::pointsForRect(m_rect)));
		}
		else {
			m_handler_vector = QetGraphicsHandlerItem::handlerForPoint(mapToScene(QetGraphicsHandlerUtility::pointForRadiusRect(m_rect, m_xRadius, m_yRadius)));
		}

		for (QetGraphicsHandlerItem *handler : m_handler_vector)
		{
			QColor color;
			if(m_resize_mode == 1)       {color = Qt::blue;}
			else if (m_resize_mode == 2) {color = Qt::darkGreen;}
			else                         {color = Qt::magenta;}

			handler->setColor(color);
			scene()->addItem(handler);
			handler->installSceneEventFilter(this);
			handler->setZValue(this->zValue()+1);
		}
	}
}

/**
	@brief PartRectangle::removeHandler
	Remove the handlers of this item
*/
void PartRectangle::removeHandler()
{
	if (!m_handler_vector.isEmpty())
	{
		qDeleteAll(m_handler_vector);
		m_handler_vector.clear();
	}
}
