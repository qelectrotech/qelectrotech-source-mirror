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
#ifndef SYMBOLSCALE_H
#define SYMBOLSCALE_H

#include <QList>
#include <QPointF>
#include <QtMath>

/**
	Which factors an element can be scaled by, in the element editor, without
	taking its terminals off the grid of the folio.

	A terminal's position is its wire end, relative to the hotspot, and the
	hotspot of a placed element sits on a grid point. So the wires of an
	element line up with the grid when every terminal coordinate is a
	multiple of the grid size. The element is scaled about its hotspot, so a
	factor is safe when every coordinate times the factor is still a
	multiple of the grid size.

	The same rule covers an element whose terminals are off the grid today:
	the factors offered are the ones that bring them all on to it.
*/
namespace SymbolScale
{
		/// The folio grid the collection is drawn to. Fixed rather than read
		/// from the user's settings: elements are shared between users.
	constexpr int grid = 10;

		/// The factors offered, in the order shown.
	inline QList<qreal> candidates()
	{
		return {0.5, 1.5, 2.0, 2.5, 3.0, 4.0};
	}

		/// True if \a value is a multiple of \a grid_size, ignoring the
		/// rounding left by saving with two decimals.
	inline bool onGrid(qreal value, int grid_size = grid)
	{
		const qreal steps = value / grid_size;
		return qAbs(steps - qRound(steps)) * grid_size < 0.005;
	}

		/// True if every terminal in \a terminals is on the grid.
	inline bool allOnGrid(const QList<QPointF> &terminals, int grid_size = grid)
	{
		for (const QPointF &p : terminals) {
			if (!onGrid(p.x(), grid_size) || !onGrid(p.y(), grid_size)) {
				return false;
			}
		}
		return true;
	}

		/// How many of \a terminals are off the grid.
	inline int offGridCount(const QList<QPointF> &terminals, int grid_size = grid)
	{
		int count = 0;
		for (const QPointF &p : terminals) {
			if (!onGrid(p.x(), grid_size) || !onGrid(p.y(), grid_size)) {
				++count;
			}
		}
		return count;
	}

		/// The candidate factors that leave every terminal of \a terminals on
		/// the grid. An element with no terminal can take any of them.
	inline QList<qreal> safeFactors(const QList<QPointF> &terminals, int grid_size = grid)
	{
		QList<qreal> safe;
		for (const qreal factor : candidates()) {
			QList<QPointF> scaled;
			for (const QPointF &p : terminals) {
				scaled << p * factor;
			}
			if (allOnGrid(scaled, grid_size)) {
				safe << factor;
			}
		}
		return safe;
	}

		/// Font size after scaling by \a factor: whole points, and never
		/// below 4 pt, so a halved element keeps readable text.
	inline int scaledFontSize(qreal size, qreal factor)
	{
		return qMax(4, qRound(size * factor));
	}
}

#endif // SYMBOLSCALE_H
