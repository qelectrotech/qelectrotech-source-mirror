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
#include "qetgraphicshandlerutility.h"
#include <QPainter>

/**
 * @brief QetGraphicsHandlerUtility::QetGraphicsHandlerUtility
 * Constructor
 * @param size : the size of the handler
 */
QetGraphicsHandlerUtility::QetGraphicsHandlerUtility(qreal size) :
	m_size (size),
	m_zoom_factor(1)
{}

/**
 * @brief QetGraphicsHandlerUtility::DrawHandler
 * Draw the handler at pos @point, using the QPainter @painter.
 * @param painter : painter to use for drawing the handler
 * @param point : point to draw the handler
 */
void QetGraphicsHandlerUtility::DrawHandler(QPainter *painter, const QPointF &point, bool color2)
{
		//Color of handler
	QColor inner(0xFF, 0xFF, 0xFF);
	QColor outer(0x00, 0x61, 0xFF);
	if(color2) outer = QColor(0x1A, 0x5C, 0x14);
		//Setup the zoom factor to draw the handler in the same size at screen,
		//no matter the zoom of the QPainter
	m_zoom_factor = 1.0/painter->transform().m11();

	painter->save();
	painter->setBrush(QBrush(inner));
	QPen square_pen(QBrush(outer), 2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
	square_pen.setCosmetic(true);
	painter->setPen(square_pen);
	painter->drawRect(getRect(point));
	painter->restore();
}

/**
 * @brief QetGraphicsHandlerUtility::pointIsInHandler
 * @param point : point to compare
 * @param key_point : point at the center of handler (the point to modify, for exemple the corner of a rectangle)
 * @return true if point is in a handler. else false
 */
bool QetGraphicsHandlerUtility::pointIsInHandler(const QPointF &point, const QPointF &key_point) const {
	return (getRect(key_point).contains(point));
}

/**
 * @brief QetGraphicsHandlerUtility::pointIsHoverHandler
 * @param point : point to compare
 * @param vector : vector of key_point (the point to modify, for exemple the corners of a rectangle)
 * @return if point is hover an handler, return the index of the hovered key_point in the vector, else return -1
 */
int QetGraphicsHandlerUtility::pointIsHoverHandler(const QPointF &point, const QVector<QPointF> &vector) const
{
	foreach (QPointF key_point, vector)
		if (pointIsInHandler(point, key_point))
			return vector.indexOf(key_point);

	return -1;
}

/**
 * @brief QetGraphicsHandlerUtility::handlerRect
 * Return the rect of the handler for all key_point in vector (the point to modify, for exemple the corners of a rectangle)
 * The order of rect in the returned vector is the same as the given vector.
 * @param vector
 * @return
 */
QVector<QRectF> QetGraphicsHandlerUtility::handlerRect(const QVector<QPointF> &vector) const
{
	QVector <QRectF> rect_vector;

	foreach(QPointF point, vector)
		rect_vector << getRect(point);

	return rect_vector;
}

/**
 * @brief QetGraphicsHandlerUtility::getRect
 * @param point
 * @return
 */
QRectF QetGraphicsHandlerUtility::getRect(const QPointF &point) const
{
	qreal rect_size = m_size * m_zoom_factor;
	QRectF rect(point.x() - rect_size/2, point.y() - rect_size/2, rect_size, rect_size);
	return rect;
}

/**
 * @brief QetGraphicsHandlerUtility::pointsForRect
 * Return the point of the rect in vector.
 * The point are stored like this :
 * top left, top right, bottom left, bottom right;
 * @param rect
 * @return
 */
QVector<QPointF> QetGraphicsHandlerUtility::pointsForRect(const QRectF &rect)
{
	QVector<QPointF> vector;
	vector << rect.topLeft() << rect.topRight() << rect.bottomLeft() << rect.bottomRight();
	return vector;
}
