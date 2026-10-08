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
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef WIREHOPS_H
#define WIREHOPS_H

#include <QList>
#include <QPainterPath>
#include <QPointF>
#include <QString>
#include <QVector>

/**
	@brief The WireHops namespace
	Hops (small arcs) drawn where two wires cross without being connected
	(issue #436). Only the drawing changes: no element is added and no wire
	is split. Kept free of any QGraphicsItem so the geometry can be tested
	on its own.

	Wires in QElectroTech are made of horizontal and vertical segments
	only, so a crossing is a segment of one orientation passing through a
	segment of the other, away from the ends of both. A wire that ends or
	bends on another one is a junction, not a crossing, and never hops.
*/
namespace WireHops
{
		///Which wire of a crossing draws the hop
	enum class Mode {
		None,       ///< no hop, the default: crossings are drawn as plain lines
		Horizontal, ///< the horizontal wire hops over the vertical one
		Vertical    ///< the vertical wire hops over the horizontal one
	};

		///Radius of a hop, in scene units (a grid step is 10)
	constexpr qreal radius = 4.0;

	QString toString(Mode mode);
	Mode fromString(const QString &string);

	QList<QPointF> crossings(const QVector<QPointF> &wire,
							 const QList<QVector<QPointF>> &others,
							 Mode mode);
	QPainterPath path(const QVector<QPointF> &wire,
					  const QList<QPointF> &hops,
					  Mode mode);
}

#endif // WIREHOPS_H
