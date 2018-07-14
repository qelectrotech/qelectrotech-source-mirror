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

/**
 * @brief QetGraphicsHandlerUtility::polygonForInsertPoint
 * @param old_polygon : the polygon which we insert a new point. 
 * @param closed : polygon is closed or not
 * @param pos : the pos where the new point must be added
 * @return the new polygon
 */
QPolygonF QetGraphicsHandlerUtility::polygonForInsertPoint(const QPolygonF &old_polygon, bool closed, const QPointF &pos)
{
	qreal max_angle = 0;
	int index = 0;
	
	for (int i=1 ; i<old_polygon.size() ; i++)
	{
		QPointF A = old_polygon.at(i-1);
		QPointF B = old_polygon.at(i);
		QLineF line_a(A, pos);
		QLineF line_b(pos, B);
		qreal angle = line_a.angleTo(line_b);
		if(angle<180)
			angle = 360-angle;
		
		if (i==1)
		{
			max_angle = angle;
			index=i;
		}
		if (angle > max_angle)
		{
			max_angle = angle;
			index=i;
		}
	}
		//Special case when polygon is close
	if (closed)
	{
		QLineF line_a(old_polygon.last(), pos);
		QLineF line_b(pos, old_polygon.first());
		
		qreal angle = line_a.angleTo(line_b);
		if (angle<180)
			angle = 360-angle;
		
		if (angle > max_angle)
		{
			max_angle = angle;
			index=old_polygon.size();
		}
	}
	
	QPolygonF polygon = old_polygon;
	polygon.insert(index, pos);
	return polygon;
}

/**
 * @brief QetGraphicsHandlerUtility::pointForRadiusRect
 * @param rect the rectangle.
 * @param xRadius : x radius
 * @param yRadius : y radius
 * @param mode : absolute or relative size: NOTE this argument is not used, this function always compute with relative size.
 * @return the points of x and y radius of a rounded rect.
 * The points are always based on the top right corner of the rect.
 * the first point of vector is X the second Y
 */
QVector<QPointF> QetGraphicsHandlerUtility::pointForRadiusRect(const QRectF &rect, qreal xRadius, qreal yRadius, Qt::SizeMode mode)
{
	QVector<QPointF> v;
	
	if(mode == Qt::AbsoluteSize)
	{
		QPointF X = rect.topRight();
		X.rx() -= xRadius;
		v << X;
		
		QPointF Y = rect.topRight();
		Y.ry() += yRadius;
		v << Y;
	}
	else
	{
		qreal half_width = rect.width()/2;
		qreal x_percent = std::min(xRadius, 100.00)/100;
		QPointF X(rect.right() - half_width*x_percent,
				  rect.top());
		v << X;
		
		qreal half_height = rect.height()/2;
		qreal y_percent = std::min(yRadius, 100.00)/100;
		QPointF Y(rect.right(),
				  rect.top()+ half_height*y_percent);
		v << Y;
	}
	
	return v;
}

/**
 * @brief QetGraphicsHandlerUtility::radiusForPosAtIndex
 * @param rect the rectangle
 * @param pos : the pos of the new radius
 * @param index : index of radius  0=X 1=Y
 * @param mode
 * @return 
 */
qreal QetGraphicsHandlerUtility::radiusForPosAtIndex(const QRectF &rect, const QPointF &pos, int index, Qt::SizeMode mode)
{
	if (mode == Qt::AbsoluteSize)
	{
		if (index == 0)
		{
			QPointF tr = rect.topRight();
			qreal x = tr.x() - pos.x();
			if (x < 0) {
				x = 0;
			}
			else if (x > rect.width()/2) {
				x = rect.width()/2;
			}
			
			return x;
		}
		else if (index == 1)
		{
			QPointF tr = rect.topRight();
			qreal y = pos.y() - tr.y();
			if (y < 0) {
				y = 0;
			}
			else if (y > rect.height()/2) {
				y = rect.height()/2;
			}

			return y;
		}
		else {
			return 0;
		}
	}
	else
	{
		if(index == 0) //X
		{
			if (pos.x() < rect.center().x()) {
				return 100;
			}
			else if (pos.x() > rect.right()) {
				return 0;
			}
			else {
				return (100 - percentageInRange(rect.center().x(), rect.right(), pos.x()));
			}
		}
		else if (index == 1) //Y
		{
			if (pos.y() < rect.top()) {
				return 0;
			}
			else if (pos.y() > rect.center().y()) {
				return 100;
			}
			else {
				return percentageInRange(rect.top(), rect.center().y(), pos.y());
			}
		}
		else {
			return 0;
		}
	}
}

qreal QetGraphicsHandlerUtility::percentageInRange(qreal min, qreal max, qreal value) {
	return ((value - min) * 100) / (max - min);
}

