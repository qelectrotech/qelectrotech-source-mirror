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
#include <QPixmapCache>
#include <QPainter>

#define QetGraphicsHandlerSquareSize 10

/**
 * @brief QetGraphicsHandlerUtility::pixmapHandler
 * @return The pixmap of an handler
 */
QPixmap QetGraphicsHandlerUtility::pixmapHandler()
{
	QPixmap handler(QetGraphicsHandlerSquareSize, QetGraphicsHandlerSquareSize);

	if (!QPixmapCache::find("QetGraphicsHandler", handler))
	{		//Pixmap isn't store in the QPixmapCache, we create it
		QColor inner(0xFF, 0xFF, 0xFF);
		QColor outer(0x00, 0x61, 0xFF);

		QPainter painter_(&handler);
		painter_.setBrush(QBrush(inner));
		QPen square_pen(QBrush(outer), 2.0, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
		square_pen.setCosmetic(true);
		painter_.setPen(square_pen);
		painter_.drawRect(0,0,10,10);

			//Store the pixmap in the QPixmapCache
		QPixmapCache::insert("QetGraphicsHandler", handler);
	}
	return handler;
}

/**
 * @brief QetGraphicsHandlerUtility::posForHandler
 * Returns a QPointF at the good coordinates
 * for draw the handler pixmap centered on the point to modify
 * @param point : the point to modify
 * @return : point at the good coordinates to draw handler centered in @point
 */
QPointF QetGraphicsHandlerUtility::posForHandler(const QPointF &point)
{
	QPointF snap_point = point;
	snap_point.rx() -= QetGraphicsHandlerSquareSize/2;
	snap_point.ry() -= QetGraphicsHandlerSquareSize/2;
	return snap_point;
}

/**
 * @brief QetGraphicsHandlerUtility::pointIsInHandler
 * @param point : point to compare
 * @param key_point : point at the center of handler (the point to modify, for exemple the corner of a rectangle)
 * @return true if point is in a handler. else false
 */
bool QetGraphicsHandlerUtility::pointIsInHandler(const QPointF &point, const QPointF &key_point)
{
	QRectF handler (posForHandler(key_point), QSize(QetGraphicsHandlerSquareSize, QetGraphicsHandlerSquareSize));
	return handler.contains(point);
}

/**
 * @brief QetGraphicsHandlerUtility::pointIsHoverHandler
 * @param point : point to compare
 * @param vector : vector of key_point (the point to modify, for exemple the corners of a rectangle)
 * @return if point is hover an handler, return the index of the hovered key_point in the vector, else return -1
 */
int QetGraphicsHandlerUtility::pointIsHoverHandler(const QPointF &point, const QVector<QPointF> &vector)
{
	foreach (QPointF key_point, vector)
		if (pointIsInHandler(point, key_point))
			return vector.indexOf(key_point);

	return -1;
}

/**
 * @brief QetGraphicsHandlerUtility::handlerRect
 * Return the rect of pixmap handler for all key_point in vector (the point to modify, for exemple the corners of a rectangle)
 * The order of rect in the returned vector is the same as the given vector.
 * @param vector
 * @return
 */
QVector<QRectF> QetGraphicsHandlerUtility::handlerRect(const QVector<QPointF> &vector)
{
	QVector <QRectF> rect_vector;
	QSize size(QetGraphicsHandlerSquareSize, QetGraphicsHandlerSquareSize);

	foreach(QPointF point, vector)
		rect_vector << QRectF(posForHandler(point), size);

	return rect_vector;
}
