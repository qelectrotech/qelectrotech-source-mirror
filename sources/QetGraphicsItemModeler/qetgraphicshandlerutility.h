/*
	Copyright 2006-2016 The QElectroTech Team
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
		QetGraphicsHandlerUtility (qreal size = 1);
		void setSize(qreal size) {m_size = size;}
		void drawHandler (QPainter *painter, const QPointF & point);
		void drawHandler(QPainter *painter, const QVector<QPointF> &points);
		QPointF posForHandler(const QPointF &point) const;
		bool pointIsInHandler (const QPointF &point, const QPointF &key_point) const;
		int pointIsHoverHandler (const QPointF &point, const QVector<QPointF> &vector) const;
		QVector<QRectF> handlerRect (const QVector<QPointF> &vector) const;
		void setInnerColor (QColor color);
		void setOuterColor (QColor color);

	private:
		QRectF getRect (const QPointF &point) const;
		qreal m_size;
		qreal m_zoom_factor = 1;
		QColor m_inner_color = Qt::white,
			   m_outer_color = Qt::blue;

	public:
		static QVector <QPointF> pointsForRect (const QRectF &rect);
		static QVector <QPointF> pointsForLine (const QLineF &line);
		static QVector <QPointF> pointsForArc  (const QRectF &rect, qreal start_angle, qreal span_angle);
		static QRectF rectForPosAtIndex (const QRectF &old_rect, const QPointF &pos, int index);
		static QRectF mirrorRectForPosAtIndex (const QRectF &old_rect, const QPointF &pos, int index);
		static QLineF lineForPosAtIndex (const QLineF &old_line, const QPointF &pos, int index);
};

#endif // QETGRAPHICSHANDLERUTILITY_H
