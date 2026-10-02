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
#ifndef CONDUCTORROUTER_H
#define CONDUCTORROUTER_H

#include <QList>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

/**
	@brief The ConductorRouter namespace
	Finds a path for a conductor that goes around obstacles -- the bounding
	rectangles of the symbols on the folio -- instead of the default two or
	three straight segments, which run through whatever lies between the two
	terminals. Kept free of any QGraphicsItem so the geometry can be tested
	on its own.

	The path is made of horizontal and vertical segments only, like every
	conductor. It leaves each terminal in the terminal's own direction,
	runs on the folio grid, and is the cheapest one found by a search that
	charges for length, for every bend, and for running along or crossing
	the wires already drawn.
*/
namespace ConductorRouter
{
		///The direction a terminal points, outward from its symbol.
		///Same order as Qet::Orientation.
	enum class Direction { North, East, South, West };

	struct Request
	{
		QPointF start;              ///< the first terminal's docking point
		Direction start_direction = Direction::North;
		QPointF end;                ///< the second terminal's docking point
		Direction end_direction = Direction::North;
			///Areas no segment may cross. A margin is added to each.
		QList<QRectF> obstacles;
			///Each terminal's own symbol, when known, as it appears in
			///obstacles. A route steps out of it first and never walks
			///through another symbol to get out; an obstacle drawn around
			///it (a cabinet made as one element) is left out.
		QRectF start_symbol;
		QRectF end_symbol;
			///The other wires on the folio, each as its list of points.
			///Running along one or crossing one costs extra.
		QList<QVector<QPointF>> wires;
			///The route stays inside this, when it is valid (the folio).
		QRectF bounds;
		qreal grid = 10.0;
			///Added around each obstacle, so a route keeps clear of it
		qreal margin = 5.0;
	};

	struct Result
	{
			///From start to end, both included; empty if none was found
		QList<QPointF> points;
			///Why there is no route, when points is empty
		QString error;
	};

	Result route(const Request &request);
}

#endif // CONDUCTORROUTER_H
