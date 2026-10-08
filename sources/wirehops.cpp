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
#include "wirehops.h"

#include <QtMath>
#include <algorithm>

namespace {
		//Coordinates closer than this are the same; points are on a grid
	constexpr qreal tolerance = 0.01;

	bool isHorizontal(const QPointF &a, const QPointF &b) {
		return qAbs(a.y() - b.y()) < tolerance && qAbs(a.x() - b.x()) >= tolerance;
	}

	bool isVertical(const QPointF &a, const QPointF &b) {
		return qAbs(a.x() - b.x()) < tolerance && qAbs(a.y() - b.y()) >= tolerance;
	}

		//True if @a value is strictly between @a a and @a b, by more than @a margin
	bool strictlyBetween(qreal value, qreal a, qreal b, qreal margin) {
		return value > qMin(a, b) + margin && value < qMax(a, b) - margin;
	}

		//The hops of @a hops lying on the segment @a a - @a b, ordered from @a a
	QList<QPointF> hopsOnSegment(const QPointF &a, const QPointF &b,
								 const QList<QPointF> &hops, bool horizontal)
	{
		QList<QPointF> on_segment;
		for (const QPointF &hop : hops)
		{
			if (horizontal
				? (qAbs(hop.y() - a.y()) < tolerance && strictlyBetween(hop.x(), a.x(), b.x(), 0))
				: (qAbs(hop.x() - a.x()) < tolerance && strictlyBetween(hop.y(), a.y(), b.y(), 0)))
			{
				on_segment.append(hop);
			}
		}
		std::sort(on_segment.begin(), on_segment.end(), [&a](const QPointF &p, const QPointF &q) {
			return QLineF(a, p).length() < QLineF(a, q).length();
		});
		return on_segment;
	}
}

/**
	@brief WireHops::toString
	@return the value written in the project file for @a mode
*/
QString WireHops::toString(Mode mode)
{
	switch (mode) {
		case Mode::Horizontal: return QStringLiteral("horizontal");
		case Mode::Vertical:   return QStringLiteral("vertical");
		case Mode::None:       break;
	}
	return QStringLiteral("none");
}

/**
	@brief WireHops::fromString
	@return the mode written as @a string in a project file,
	Mode::None for anything else, so an unknown value draws no hop.
*/
WireHops::Mode WireHops::fromString(const QString &string)
{
	if (string == QLatin1String("horizontal")) return Mode::Horizontal;
	if (string == QLatin1String("vertical"))   return Mode::Vertical;
	return Mode::None;
}

/**
	@brief WireHops::crossings
	@param wire : the points of the wire, in scene coordinates
	@param others : the points of every other wire that may cross it,
	in scene coordinates
	@param mode : which wire of a crossing hops
	@return the points where @a wire hops, in the order of @a wire.

	A crossing is kept only if it lies strictly inside the segment of the
	other wire (a wire ending or bending on this one is a junction, not a
	crossing) and at least one hop radius from the ends of this wire's
	segment, so the arc fits. Of two crossings closer than one arc
	width, only the first hops.
*/
QList<QPointF> WireHops::crossings(const QVector<QPointF> &wire,
								   const QList<QVector<QPointF>> &others,
								   Mode mode)
{
	QList<QPointF> hops;
	if (mode == Mode::None || wire.size() < 2) {
		return hops;
	}

	const bool horizontal = mode == Mode::Horizontal;
	for (int i = 0 ; i < wire.size() - 1 ; ++i)
	{
		const QPointF a = wire.at(i);
		const QPointF b = wire.at(i + 1);
		if (horizontal ? !isHorizontal(a, b) : !isVertical(a, b)) {
			continue;
		}

		QList<QPointF> on_segment;
		for (const auto &other : others)
		{
			for (int j = 0 ; j < other.size() - 1 ; ++j)
			{
				const QPointF c = other.at(j);
				const QPointF d = other.at(j + 1);
				if (horizontal ? !isVertical(c, d) : !isHorizontal(c, d)) {
					continue;
				}

				const QPointF crossing = horizontal ? QPointF(c.x(), a.y())
													: QPointF(a.x(), c.y());
				const bool inside_wire = horizontal
						? strictlyBetween(crossing.x(), a.x(), b.x(), radius)
						: strictlyBetween(crossing.y(), a.y(), b.y(), radius);
				const bool inside_other = horizontal
						? strictlyBetween(crossing.y(), c.y(), d.y(), tolerance)
						: strictlyBetween(crossing.x(), c.x(), d.x(), tolerance);
				if (inside_wire && inside_other) {
					on_segment.append(crossing);
				}
			}
		}

		std::sort(on_segment.begin(), on_segment.end(), [&a](const QPointF &p, const QPointF &q) {
			return QLineF(a, p).length() < QLineF(a, q).length();
		});
		for (const QPointF &crossing : std::as_const(on_segment))
		{
			if (!hops.isEmpty()
				&& QLineF(hops.last(), crossing).length() < 2 * radius + tolerance) {
				continue;
			}
			hops.append(crossing);
		}
	}
	return hops;
}

/**
	@brief WireHops::path
	@param wire : the points of the wire
	@param hops : the points where the wire hops, from WireHops::crossings,
	in the same coordinates as @a wire
	@param mode : which wire of a crossing hops
	@return the path of the wire, with a half circle at each hop: above a
	horizontal wire, right of a vertical one. With no hop, the same path
	as the plain wire.
*/
QPainterPath WireHops::path(const QVector<QPointF> &wire,
							const QList<QPointF> &hops,
							Mode mode)
{
	QPainterPath path;
	if (wire.isEmpty()) {
		return path;
	}

	const bool horizontal = mode == Mode::Horizontal;
	path.moveTo(wire.first());
	for (int i = 0 ; i < wire.size() - 1 ; ++i)
	{
		const QPointF a = wire.at(i);
		const QPointF b = wire.at(i + 1);
		const bool hops_here = mode != Mode::None
				&& (horizontal ? isHorizontal(a, b) : isVertical(a, b));

		if (hops_here)
		{
				//1 when the wire runs towards +x (or +y), -1 otherwise
			const qreal direction = horizontal ? (b.x() > a.x() ? 1 : -1)
											   : (b.y() > a.y() ? 1 : -1);
			for (const QPointF &hop : hopsOnSegment(a, b, hops, horizontal))
			{
				const QRectF circle(hop.x() - radius, hop.y() - radius,
									2 * radius, 2 * radius);
				if (horizontal) {
						//Over the top: from the left end to the right one, or back
					path.lineTo(hop.x() - direction * radius, hop.y());
					path.arcTo(circle, direction > 0 ? 180 : 0, direction > 0 ? -180 : 180);
				} else {
						//On the right: from the top end to the bottom one, or back
					path.lineTo(hop.x(), hop.y() - direction * radius);
					path.arcTo(circle, direction > 0 ? 90 : 270, direction > 0 ? -180 : 180);
				}
			}
		}
		path.lineTo(b);
	}
	return path;
}
