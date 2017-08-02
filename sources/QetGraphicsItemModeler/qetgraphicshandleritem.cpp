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
#include "qetgraphicshandleritem.h"
#include <QPainter>
#include <QDebug>

/**
 * @brief QetGraphicsHandlerItem::QetGraphicsHandlerItem
 * @param size, the size of the handler
 */
QetGraphicsHandlerItem::QetGraphicsHandlerItem(qreal size) :
    m_size(size)
{}

/**
 * @brief QetGraphicsHandlerItem::boundingRect
 * @return 
 */
QRectF QetGraphicsHandlerItem::boundingRect() const
{
    qreal rect_size = m_size * m_previous_zoom_factor;
    QRectF rect(0-rect_size/2, 0-rect_size/2, rect_size, rect_size);
    rect.adjust(-2, -2, 2, 2);
	return rect;
}

/**
 * @brief QetGraphicsHandlerItem::setColor
 * @param color, set the color of the handler
 */
void QetGraphicsHandlerItem::setColor(QColor color)
{
	m_color = color;
	update();
}

/**
 * @brief QetGraphicsHandlerItem::paint
 * @param painter
 * @param option
 * @param widget
 */
void QetGraphicsHandlerItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    qreal zoom_factor = 1.0/painter->transform().m11();
    if(zoom_factor != m_previous_zoom_factor)
    {
        prepareGeometryChange();
        m_previous_zoom_factor = zoom_factor;
    }

    qreal rect_size = m_size * m_previous_zoom_factor;
    QRectF rect(0-rect_size/2, 0-rect_size/2, rect_size, rect_size);

    painter->save();
    painter->setBrush(QBrush(m_color));
    QPen pen(QBrush(m_color), 2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
	pen.setCosmetic(true);
    painter->setPen(pen);
	painter->setRenderHint(QPainter::Antialiasing, true);
    painter->drawEllipse(rect);
	painter->restore();
}

/**
 * @brief QetGraphicsHandlerItem::handlerForPoint
 * @param points
 * @return A list of handler with pos at point
 */
QVector<QetGraphicsHandlerItem *> QetGraphicsHandlerItem::handlerForPoint(const QVector<QPointF> &points, int size)
{
    QVector <QetGraphicsHandlerItem *> list_;
    for (QPointF point : points)
    {
        QetGraphicsHandlerItem *qghi = new QetGraphicsHandlerItem(size);
        qghi->setPos(point);
        list_ << qghi;
    }

	return list_;
}
