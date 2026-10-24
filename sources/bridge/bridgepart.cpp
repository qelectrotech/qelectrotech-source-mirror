/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "bridgepart.h"

#include "../diagram.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

/**
	@brief BridgePart::BridgePart
	@param data the section of the bridge this line stands for
	@param parent
*/
BridgePart::BridgePart(const BridgePartData &data, QGraphicsItem *parent) :
	QetGraphicsItem(parent),
	m_part_uuid(data.uuid),
	m_p1(data.p1),
	m_p2(data.p2)
{
	setMovable(false); //the shape is the user's own, changing it comes later
	setZValue(10); //over the conductors, like any line drawn above a wire
	setFlag(QGraphicsItem::ItemIsSelectable);
	setAcceptHoverEvents(true);
}

/**
	@param bridge the bridge this line is drawn for
*/
void BridgePart::setBridge(Bridge *bridge)
{
	m_bridge = bridge;
}

/**
	@return the bridge this line is drawn for, or nullptr
*/
Bridge *BridgePart::bridge() const {
	return m_bridge;
}

/**
	@return which section of its bridge this line is
*/
QUuid BridgePart::partUuid() const {
	return m_part_uuid;
}

/**
	@param p1
	@param p2
*/
void BridgePart::setLine(const QPointF &p1, const QPointF &p2)
{
	prepareGeometryChange();
	m_p1 = p1;
	m_p2 = p2;
	update();
}

/**
	@return the line this section is drawn as
*/
QLineF BridgePart::line() const {
	return QLineF(m_p1, m_p2);
}

/**
	@return the area the line is drawn in, with a margin for the pen
	and the end grips
*/
QRectF BridgePart::boundingRect() const
{
	const QRectF rect(m_p1, m_p2);
	return rect.normalized().adjusted(-4, -4, 4, 4);
}

/**
	@return the area which answers the mouse: the line itself, thick
	enough to be clicked without aiming
*/
QPainterPath BridgePart::shape() const
{
	QPainterPath path(m_p1);
	path.lineTo(m_p2);
	QPainterPathStroker stroker;
	stroker.setWidth(8);
	return stroker.createStroke(path);
}

/**
	@brief BridgePart::paint
	@param painter
	@param option
	@param widget
*/
void BridgePart::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)

	QPen pen(drawColor(), 1, Qt::SolidLine, Qt::RoundCap);
	pen.setCosmetic(true);
	painter->setPen(pen);
	painter->drawLine(m_p1, m_p2);

		//The two ends of the line when it is chosen, so the hand has
		//something to hold on to (stretching the bridge comes later)
	if (isSelected()) {
		QPen grip_pen(QColor(28, 134, 238), 1);
		grip_pen.setCosmetic(true);
		painter->setPen(grip_pen);
		painter->setBrush(Qt::white);
		painter->drawEllipse(m_p1, 3, 3);
		painter->drawEllipse(m_p2, 3, 3);
	}
}

/**
	@return the name of this kind of item, for the undo texts
*/
QString BridgePart::name() const {
	return tr("bridge");
}

/**
	@param event
*/
void BridgePart::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(event)
	editProperty();
}

/**
	The dialog editing the properties of the whole bridge: colour and
	how many terminals it may hold. It comes with the bridge tool.
*/
void BridgePart::editProperty()
{
}

/**
	@return the colour this line is drawn in: its bridge's colour,
	black when it has no bridge (yet)
*/
QColor BridgePart::drawColor() const {
	return m_bridge ? m_bridge->color() : QColor(Qt::black);
}
