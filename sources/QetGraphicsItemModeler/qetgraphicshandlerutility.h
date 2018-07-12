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
#ifndef QETGRAPHICSHANDLERUTILITY_H
#define QETGRAPHICSHANDLERUTILITY_H

#include <QRectF>
#include <QLineF>
#include <QColor>
#include <QPolygonF>

class QPainter;

/**
 * @brief The QetGraphicsHandlerUtility class
 * This class provide some methods to create and use handler for
 * modify graphics shape like line rectangle etc...
 * They also provide some conveniance static method.
 */
class QetGraphicsHandlerUtility
{
	public:
		static QVector <QPointF> pointsForRect (const QRectF &rect);
		static QVector <QPointF> pointsForLine (const QLineF &line);
		static QVector <QPointF> pointsForArc  (const QRectF &rect, qreal start_angle, qreal span_angle);
		static QRectF rectForPosAtIndex (const QRectF &old_rect, const QPointF &pos, int index);
		static QRectF mirrorRectForPosAtIndex (const QRectF &old_rect, const QPointF &pos, int index);
		static QLineF lineForPosAtIndex (const QLineF &old_line, const QPointF &pos, int index);
		static QPolygonF polygonForInsertPoint(const QPolygonF &old_polygon, bool closed, const QPointF &pos);
		static QVector <QPointF> pointForRadiusRect (const QRectF &rect, qreal xRadius, qreal yRadius, Qt::SizeMode mode = Qt::AbsoluteSize);
		static qreal radiusForPosAtIndex (const QRectF &rect, const QPointF &pos, int index, Qt::SizeMode mode = Qt::AbsoluteSize);
		static qreal percentageInRange(qreal min, qreal max, qreal value);
};

#endif // QETGRAPHICSHANDLERUTILITY_H
