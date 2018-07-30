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
#include <utility>

/**
 * @brief QetGraphicsHandlerItem::QetGraphicsHandlerItem
 * @param size, the size of the handler
 */
QetGraphicsHandlerItem::QetGraphicsHandlerItem(qreal size) :
    m_size(size)
{
	setFlag(QGraphicsItem::ItemIgnoresTransformations);
	
	m_handler_rect.setRect(0-m_size/2, 0-m_size/2, m_size, m_size);
	m_br.setRect(-1-m_size/2, -1-m_size/2, m_size+2, m_size+2);
}

/**
 * @brief QetGraphicsHandlerItem::boundingRect
 * @return 
 */
QRectF QetGraphicsHandlerItem::boundingRect() const {
	return m_br;
}

/**
 * @brief QetGraphicsHandlerItem::setColor
 * @param color, set the color of the handler
 */
void QetGraphicsHandlerItem::setColor(QColor color)
{
	m_color = std::move(color);
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

    painter->save();
    painter->setBrush(QBrush(m_color));
    QPen pen(QBrush(m_color), 2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
	pen.setCosmetic(true);
    painter->setPen(pen);
	painter->setRenderHint(QPainter::Antialiasing, true);
    painter->drawEllipse(m_handler_rect);
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
