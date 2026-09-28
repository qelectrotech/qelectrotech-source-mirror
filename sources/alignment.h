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
#ifndef ALIGNMENT_H
#define ALIGNMENT_H

#include "textgrid.h"

#include <QList>
#include <QPointF>
#include <QRectF>

#include <algorithm>

/**
	The geometry behind the align commands, kept free of any scene so it
	can be tested on its own.
*/
namespace Alignment
{
	/**
		What the items of a selection are lined up on.
	*/
	enum Edge {
		Left,    ///< left edges, on the left-most one
		HCenter, ///< reference points, on the mean of their x
		Right,   ///< right edges, on the right-most one
		Top,     ///< top edges, on the top-most one
		VCenter, ///< reference points, on the mean of their y
		Bottom   ///< bottom edges, on the bottom-most one
	};

	/**
		One item to align: its edges, and the point that counts as its
		centre. For a symbol that point is its origin, where its wires
		usually leave, not the middle of its drawn shape.
	*/
	struct Item {
		QRectF edges;
		QPointF ref;
	};

	/**
		@return @a items taken as one piece, the way a group lines up:
		their edges together, and the middle of that box as its centre.
		A single item keeps its own centre.
	*/
	inline Item combined(const QList<Item> &items)
	{
		Item result;
		for (const Item &item : items)
			result.edges = result.edges.isNull() ? item.edges
							     : result.edges.united(item.edges);
		if (items.size() == 1)
			result.ref = items.first().ref;
		else
			result.ref = result.edges.center();
		return result;
	}

	/**
		@return true if aligning on @a edge moves items along x
	*/
	inline bool isHorizontal(Edge edge)
	{
		return edge == Left || edge == HCenter || edge == Right;
	}

	/**
		@return the movement of each of @a items that lines them up on
		@a edge, in the same order. Items only move across the line they
		are aligned on: aligning left never moves anything up or down.
	*/
	inline QList<QPointF> alignOffsets(const QList<Item> &items, Edge edge)
	{
		QList<qreal> values;
		for (const Item &item : items)
		{
			switch (edge) {
				case Left:    values << item.edges.left();   break;
				case HCenter: values << item.ref.x();        break;
				case Right:   values << item.edges.right();  break;
				case Top:     values << item.edges.top();    break;
				case VCenter: values << item.ref.y();        break;
				case Bottom:  values << item.edges.bottom(); break;
			}
		}

		QList<QPointF> offsets;
		if (values.isEmpty())
			return offsets;

		qreal target = 0;
		switch (edge) {
			case Left:
			case Top:
				target = *std::min_element(values.cbegin(), values.cend());
				break;
			case Right:
			case Bottom:
				target = *std::max_element(values.cbegin(), values.cend());
				break;
			case HCenter:
			case VCenter:
				for (qreal v : std::as_const(values))
					target += v;
				target /= values.size();
				break;
		}

		for (qreal v : std::as_const(values))
			offsets << (isHorizontal(edge) ? QPointF(target - v, 0)
						       : QPointF(0, target - v));
		return offsets;
	}

	/**
		@return the movement that puts p on a grid of x_grid by y_grid,
		divided by divisor as TextGrid::snap() does, or a null point when
		p is already on it. Unlike Diagram::snapToGrid(), this never looks
		at the keyboard: a command run from a shortcut with Ctrl in it must
		not quietly round to the pixel instead.
		Less than a millionth of a pixel counts as on the grid: positions
		that went through arithmetic carry residues of that size, and
		qFuzzyIsNull() (1e-12) is too strict to absorb them.
	*/
	inline QPointF gridOffset(const QPointF &p, int x_grid, int y_grid, qreal divisor = 1)
	{
		const QPointF offset = TextGrid::snap(p, x_grid, y_grid, divisor) - p;
		auto clean = [](qreal v) { return qAbs(v) < 1e-6 ? 0.0 : v; };
		return QPointF(clean(offset.x()), clean(offset.y()));
	}

	/**
		@return @a offset, the movement alignOffsets() gave an item at
		@a pos, rounded so the item lands on the grid along the line it is
		aligned on. The other coordinate is left as it is, even off the
		grid: aligning left must not also move items up or down.
		Edges of items whose width is not a whole number of grid steps
		apart from their origin cannot all land on one line; they end up as
		close to it as the grid allows.
	*/
	inline QPointF snappedOffset(const QPointF &pos, const QPointF &offset, Edge edge,
				     int x_grid, int y_grid, qreal divisor = 1)
	{
		const QPointF snap = gridOffset(pos + offset, x_grid, y_grid, divisor);
		QPointF result = offset + (isHorizontal(edge) ? QPointF(snap.x(), 0)
							       : QPointF(0, snap.y()));
		auto clean = [](qreal v) { return qAbs(v) < 1e-6 ? 0.0 : v; };
		return QPointF(clean(result.x()), clean(result.y()));
	}
}

#endif // ALIGNMENT_H
