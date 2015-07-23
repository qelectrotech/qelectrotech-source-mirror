/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "abstractpartellipse.h"

/**
 * @brief AbstractPartEllipse::AbstractPartEllipse
 * Constructor
 * @param editor : QETElementEditor of this part
 * @param parent : parent item
 */
AbstractPartEllipse::AbstractPartEllipse(QETElementEditor *editor, QGraphicsItem *parent) :
	CustomElementGraphicPart(editor, parent),
	m_rect                  (QRectF(0, 0, 0, 0)),
	m_start_angle           (0),
	m_span_angle            (5760)
{}

/**
 * @brief AbstractPartEllipse::~AbstractPartEllipse
 * Destructor
 */
AbstractPartEllipse::~AbstractPartEllipse() {}

/**
 * @brief AbstractPartEllipse::startUserTransformation
 * Start the user-induced transformation, provided this primitive is contained
 * within the initial_selection_rect bounding rectangle.
 * @param initial_selection_rect
 */
void AbstractPartEllipse::startUserTransformation(const QRectF &initial_selection_rect)
{
	Q_UNUSED(initial_selection_rect)
		// we keep track of our own rectangle at the moment in scene coordinates too
	saved_points_.clear();
	saved_points_ << mapToScene(rect().topLeft()) << mapToScene(rect().bottomRight());
}

/**
 * @brief AbstractPartEllipse::handleUserTransformation
 * Handle the user-induced transformation from \a initial_selection_rect to \a new_selection_rect
 * @param initial_selection_rect
 * @param new_selection_rect
 */
void AbstractPartEllipse::handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect)
{
	QList<QPointF> mapped_points = mapPoints(initial_selection_rect, new_selection_rect, saved_points_);
	setRect(QRectF(mapFromScene(mapped_points.at(0)), mapFromScene(mapped_points.at(1))));
}

/**
 * @brief AbstractPartEllipse::boundingRect
 * Bounding rectangle this part can fit into
 * @return
 */
QRectF AbstractPartEllipse::boundingRect() const
{
	qreal adjust = (SHADOWS_HEIGHT + penWeight()) / 2;
		//We add 0.5 because CustomElementGraphicPart::drawShadowShape
		//draw a shape bigger of 0.5 when pen weight is to 0.
	if (penWeight() == 0) adjust += 0.5;
	QRectF r(m_rect.normalized());
	r.adjust(-adjust, -adjust, adjust, adjust);
	return(r);
}

/**
 * @brief AbstractPartEllipse::sceneGeometricRect
 * @return the minimum, margin-less rectangle this part can fit into in scene coordinates.
 * It is different from boundingRect() because it is not supposed
 * to imply any margin, and it is different from shape because it is a regular
 * rectangle, not a complex shape.
 */
QRectF AbstractPartEllipse::sceneGeometricRect() const {
	return(mapToScene(rect()).boundingRect());
}

/**
 * @brief AbstractPartEllipse::sceneTopLeft
 * @return return the top left of rectangle, in scene coordinate
 */
QPointF AbstractPartEllipse::sceneTopLeft() const {
	return(mapToScene(rect().topLeft()));
}

/**
 * @brief AbstractPartEllipse::rect
 * Returns the item's ellipse geometry as a QRectF.
 */
QRectF AbstractPartEllipse::rect() const {
	return m_rect;
}

/**
 * @brief AbstractPartEllipse::setRect
 * Sets the item's ellipse geometry to rect.
 * The rectangle's left edge defines the left edge of the ellipse,
 * and the rectangle's top edge describes the top of the ellipse
 * The height and width of the rectangle describe the height and width of the ellipse.
 * @param rect
 */
void AbstractPartEllipse::setRect(const QRectF &rect)
{
	if (rect == m_rect) return;
	prepareGeometryChange();
	m_rect = rect;
	emit rectChanged();
}

/**
 * @brief AbstractPartEllipse::isUseless
 * @return true if this part is irrelevant and does not deserve to be Retained / registered.
 * An ellipse is relevant when is rect is not null.
 */
bool AbstractPartEllipse::isUseless() const {
	return(rect().isNull());
}

/**
 * @brief AbstractPartEllipse::setStartAngle
 * Sets the start angle for an ellipse segment to angle, which is in 16ths of a degree.
 * This angle is used together with spanAngle() for representing an ellipse segment (a pie).
 * By default, the start angle is 0.
 * @param start_angle
 */
void AbstractPartEllipse::setStartAngle(const int &start_angle)
{
	if (m_start_angle == start_angle) return;
	prepareGeometryChange();
	m_start_angle = start_angle;
	emit startAngleChanged();
}

/**
 * @brief AbstractPartEllipse::setSpanAngle
 * Returns the span angle of an ellipse segment in 16ths of a degree.
 * This angle is used together with startAngle() for representing an ellipse segment (a pie).
 * By default, this function returns 5760 (360 * 16, a full ellipse).
 * @param span_angle
 */
void AbstractPartEllipse::setSpanAngle(const int &span_angle)
{
	if (m_span_angle == span_angle) return;
	prepareGeometryChange();
	m_span_angle = span_angle;
	emit spanAngleChanged();
}

/**
 * @brief AbstractPartEllipse::setCenterX
 * Like setCenter but Y keep unchanged
 * See setCenter(const QPointF &center)
 * @param x
 */
void AbstractPartEllipse::setCenterX(const qreal x)
{
	QPointF pos = mapToParent(m_rect.center());
	pos.setX(x);
	setCenter(pos);
}

/**
 * @brief AbstractPartEllipse::setCenterY
 * Like setCenter but X keep unchanged
 * See setCenter(const QPointF &center)
 * @param y
 */
void AbstractPartEllipse::setCenterY(const qreal y)
{
	QPointF pos = mapToParent(m_rect.center());
	pos.setY(y);
	setCenter(pos);
}

/**
 * @brief AbstractPartEllipse::setCenter
 * This is a convenience method to setPos().
 * Adjust the position of this item,
 * so that the center of the rectangle is at the given position(position in parent coordinates).
 * @param center
 */
void AbstractPartEllipse::setCenter(const QPointF &center)
{
	QPointF pos = center - m_rect.center();
	setPos(pos);
}

/**
 * @brief AbstractPartEllipse::setWidth
 * Set new width for rectangle.
 * The center of rectangle is unchanged,
 * The coordinates of the left side and right side of the rectangle change
 * @param w
 */
void AbstractPartEllipse::setWidth(const qreal w)
{
	qreal new_width = qAbs(w);
	QRectF current_rect = rect();
	current_rect.translate((new_width - current_rect.width()) / -2.0, 0.0);
	current_rect.setWidth(new_width);
	setRect(current_rect);
}

/**
 * @brief AbstractPartEllipse::setHeight
 * Set new height for rectangle
 * The center of rectangle is unchanged
 * The coordinates of the top side and bottom side of the rectangle change
 * @param h
 */
void AbstractPartEllipse::setHeight(const qreal h)
{
	qreal new_height = qAbs(h);
	QRectF current_rect = rect();
	current_rect.translate(0.0, (new_height - current_rect.height()) / -2.0);
	current_rect.setHeight(new_height);
	setRect(current_rect);
}
