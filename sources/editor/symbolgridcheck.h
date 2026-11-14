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
#ifndef SYMBOLGRIDCHECK_H
#define SYMBOLGRIDCHECK_H

#include <QList>
#include <QPointF>
#include <QString>
#include <QStringList>

#include <cmath>

/**
	The grid check run by --check-elements --grid: a symbol is placed by
	its insertion point (the origin of its drawing), snapped to the folio
	grid of 10, and clicked through its frame (width, height, hotspot).
	When the insertion point is a connection point -- or on the outline of
	the frame, as for a corner or a T-junction whose connection points end
	its lines -- the frame lies on the grid and every connection point is on the grid
	too, the symbol lines up with the wires drawn to it and its outline
	with the grid.
*/
namespace SymbolGridCheck
{
	inline bool onGrid(qreal value)
	{
		return std::abs(value - 10.0 * std::round(value / 10.0)) < 0.01;
	}

		/// What keeps a symbol off the grid, for messages; empty if nothing.
		/// @p terminals : the connection points, in the symbol's coordinates
	inline QStringList problems(int hotspot_x, int hotspot_y, int width, int height,
								const QList<QPointF> &terminals)
	{
		QStringList found;
		if (!terminals.isEmpty()) {
				//The origin lies hotspot_x, hotspot_y into the frame
			const bool inside = hotspot_x >= 0 && hotspot_x <= width
								&& hotspot_y >= 0 && hotspot_y <= height;
			bool on_origin = inside && (hotspot_x == 0 || hotspot_x == width
										|| hotspot_y == 0 || hotspot_y == height);
			for (const QPointF &t : terminals)
				if (std::abs(t.x()) < 0.01 && std::abs(t.y()) < 0.01)
					on_origin = true;
			if (!on_origin)
				found << QStringLiteral("insertion point on no connection point and off the outline of the frame");
		}

		if (!onGrid(hotspot_x) || !onGrid(hotspot_y) || !onGrid(width) || !onGrid(height))
			found << QStringLiteral("frame off the grid (hotspot %1,%2, size %3x%4)")
					 .arg(hotspot_x).arg(hotspot_y).arg(width).arg(height);

		int off = 0;
		for (const QPointF &t : terminals)
			if (!onGrid(t.x()) || !onGrid(t.y()))
				++off;
		if (off)
			found << QStringLiteral("%1 of %2 connection points off the grid")
					 .arg(off).arg(terminals.size());
		return found;
	}
}

#endif // SYMBOLGRIDCHECK_H
