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
#include "qetgraphicshandlerutility.h"
//#include <QVector>
//#include <QPointF>
#include <QPainterPath>


/**
 * @brief QetGraphicsHandlerUtility::pointsForRect
 * Return the keys points of the rectangle, stored in a vector.
 * The points in the vector are stored like this :
 * **********
 *   0---1---2
 *   |       |
 *   3       4
 *   |       |
 *   5---6---7
 * ************
 * @param rect
 * @return
 */
QVector<QPointF> QetGraphicsHandlerUtility::pointsForRect(const QRectF &rect)
{
	QVector<QPointF> vector;
	QPointF point;
	vector << rect.topLeft();//*****Top left
	point = rect.center();
	point.setY(rect.top());
	vector << point;//**************Middle top
	vector << rect.topRight();//****Top right
	point = rect.center();
	point.setX(rect.left());
	vector << point;//**************Middle left
	point.setX(rect.right());
	vector << point;//**************Middle right
	vector << rect.bottomLeft();//**Bottom left
	point = rect.center();
	point.setY(rect.bottom());
	vector <<  point;//*************Middle bottom
	vector << rect.bottomRight();//*Bottom right
	return vector;
}

/**
 * @brief QetGraphicsHandlerUtility::pointsForLine
 * The point that define a line in a QVector.
 * there is two points.
 * @param line
 * @return
 */
QVector<QPointF> QetGraphicsHandlerUtility::pointsForLine(const QLineF &line) {
	return (QVector<QPointF> {line.p1(), line.p2()});
}

/**
 * @brief QetGraphicsHandlerUtility::pointsForArc
 * Return the points for the given arc.
 * The first value in the vector is the start point, the second the end point.
 * @param rect
 * @param start_angle : start angle in degree
 * @param span_angle : span angle in degree;
 * @return
 */
QVector<QPointF> QetGraphicsHandlerUtility::pointsForArc(const QRectF &rect, qreal start_angle, qreal span_angle)
{
	QVector<QPointF> vector;
	QPainterPath path;
	path.arcTo(rect, start_angle, 0);
	vector.append(path.currentPosition());
	path.arcTo(rect, start_angle, span_angle);
	vector.append(path.currentPosition());
	return vector;

}

/**
 * @brief QetGraphicsHandlerUtility::rectForPosAtIndex
 * Return a rectangle after modification of the point '@pos' at index '@index' of original rectangle '@old_rect'.
 * @param old_rect - the rectangle befor modification
 * @param pos - the new position of a key point
 * @param index - the index of the key point to modifie see QetGraphicsHandlerUtility::pointsForRect to know
 * the index of each keys points of a rectangle)
 * @return : the rectangle with modification. If index is lower than 0 or higher than 7, this method return old_rect.
 */
QRectF QetGraphicsHandlerUtility::rectForPosAtIndex(const QRectF &old_rect, const QPointF &pos, int index)
{
	if (index < 0 || index > 7) return old_rect;

	QRectF rect = old_rect;
	if (index == 0) rect.setTopLeft(pos);
	else if (index == 1) rect.setTop(pos.y());
	else if (index == 2) rect.setTopRight(pos);
	else if (index == 3) rect.setLeft(pos.x());
	else if (index == 4) rect.setRight(pos.x());
	else if (index == 5) rect.setBottomLeft(pos);
	else if (index == 6) rect.setBottom(pos.y());
	else if (index == 7) rect.setBottomRight(pos);

	return rect;
}

/**
 * @brief QetGraphicsHandlerUtility::mirrorRectForPosAtIndex
 * Return a rectangle after modification of the point '@pos' at index '@index' of original rectangle '@old_rect'.
 * the opposite edge is modified inversely (like a mirror)
 * @param old_rect : the rectangle befor modification
 * @param pos : the new position of a key point
 * @param index : the index of the key point to modifie see QetGraphicsHandlerUtility::pointsForRect to know
 * the index of each keys points of a rectangle)
 * @return : the rectangle with modification. If index is lower than 0 or higher than 7, this method return old_rect.
 */
QRectF QetGraphicsHandlerUtility::mirrorRectForPosAtIndex(const QRectF &old_rect, const QPointF &pos, int index)
{
	if (index < 0 || index > 7) return old_rect;

	QRectF rect = old_rect;
	QPointF center = rect.center();

	if (index == 0) {
		qreal x = pos.x() + (pos.x() - rect.topLeft().x());
		qreal y = pos.y() + (pos.y() - rect.topLeft().y());
		rect.setTopLeft(QPointF(x,y));
	}
	else if (index == 1) {
		qreal y = pos.y() + (pos.y() - rect.topLeft().y());
		rect.setTop(y);
	}
	else if (index == 2) {
		qreal x = pos.x() + (pos.x() - rect.topRight().x());
		qreal y = pos.y() + (pos.y() - rect.topLeft().y());
		rect.setTopRight(QPointF(x,y));
	}
	else if (index == 3) {
		qreal x = pos.x() + (pos.x() - rect.left());
		rect.setLeft(x);
	}
	else if (index == 4) {
		qreal x = pos.x() + (pos.x() - rect.right());
		rect.setRight(x);
	}
	else if (index == 5) {
		qreal x = pos.x() + (pos.x() - rect.bottomLeft().x());
		qreal y = pos.y() + (pos.y() - rect.bottomLeft().y());
		rect.setBottomLeft(QPointF(x,y));
	}
	else if (index == 6) {
		qreal y = pos.y() + (pos.y() - rect.bottom());
		rect.setBottom(y);
	}
	else if (index == 7) {
		qreal x = pos.x() + (pos.x() - rect.bottomRight().x());
		qreal y = pos.y() + (pos.y() - rect.bottomRight().y());
		rect.setBottomRight(QPointF(x,y));
	}

	rect.moveCenter(center);
	return rect;
}

/**
 * @brief QetGraphicsHandlerUtility::lineForPosAtIndex
 * Return a line after modification of @pos at index @index of @old_line.
 * @param old_line
 * @param pos
 * @param index
 * @return
 */
QLineF QetGraphicsHandlerUtility::lineForPosAtIndex(const QLineF &old_line, const QPointF &pos, int index) {
	QLineF line = old_line;
	index == 0 ? line.setP1(pos) : line.setP2(pos);
	return line;
}
