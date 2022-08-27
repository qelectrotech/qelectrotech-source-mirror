/*
	Copyright 2006-2022 The QElectroTech Team
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
#include "terminalstripitem.h"
#include "../../qetgraphicsitem/qgraphicsitemutility.h"

TerminalStripItem::TerminalStripItem(QPointer<TerminalStrip> strip, QGraphicsItem *parent) :
	QetGraphicsItem{parent},
	m_strip{strip},
	m_drawer{strip}
{
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
	setAcceptHoverEvents(true);
}

void TerminalStripItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)

	m_drawer.paint(painter);

	if (isSelected() || isHovered())
	{
		QGIUtility::drawBoundingRectSelection(this, painter);
	}
}

QRectF TerminalStripItem::boundingRect() const
{
	auto br_ = m_drawer.boundingRect();
	br_.adjust(-5,-5,5,5);

	return br_;
}
