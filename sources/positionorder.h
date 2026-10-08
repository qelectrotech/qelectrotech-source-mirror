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
#ifndef POSITIONORDER_H
#define POSITIONORDER_H

#include <QPointF>
#include <QtGlobal>

/**
	The order of items by their position on a folio, for std::sort: left to
	right then top to bottom, or top to bottom then left to right. Each
	function is a strict weak ordering, which std::sort requires: two items
	at the same position come out in either order, but never "both before
	each other", and the order is transitive. A comparison with "<=", or
	with a tolerance ("within 1 px counts as aligned"), is neither, and
	std::sort may then read past the range or assert.
*/
namespace PositionOrder
{
	/// a before b when a is left of b, or level with it and above it.
	inline bool xThenY(const QPointF &a, const QPointF &b)
	{
		if (a.x() != b.x()) return a.x() < b.x();
		return a.y() < b.y();
	}

	/// a before b when a is above b, or level with it and left of it.
	inline bool yThenX(const QPointF &a, const QPointF &b)
	{
		if (a.y() != b.y()) return a.y() < b.y();
		return a.x() < b.x();
	}

	/// The position rounded to whole pixels, so that items placed a
	/// fraction of a pixel apart count as aligned.
	inline QPointF rounded(const QPointF &p)
	{
		return QPointF(qRound(p.x()), qRound(p.y()));
	}

	/// xThenY() on the positions rounded to whole pixels.
	inline bool roundedXThenY(const QPointF &a, const QPointF &b)
	{
		return xThenY(rounded(a), rounded(b));
	}

	/// yThenX() on the positions rounded to whole pixels.
	inline bool roundedYThenX(const QPointF &a, const QPointF &b)
	{
		return yThenX(rounded(a), rounded(b));
	}
}

#endif // POSITIONORDER_H
