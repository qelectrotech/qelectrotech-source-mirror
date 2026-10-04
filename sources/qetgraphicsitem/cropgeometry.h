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
#ifndef CROPGEOMETRY_H
#define CROPGEOMETRY_H

#include <QPointF>
#include <QRect>
#include <QRectF>

#include <algorithm>
#include <cmath>

/**
	The geometry of cropping a picture directly on the folio, in the
	picture's own pixels. Free of any item so that it can be tested on
	its own.
*/
namespace CropGeometry
{
	/// Which edges of the crop frame a handle moves.
	struct Edges
	{
		bool left = false;
		bool top = false;
		bool right = false;
		bool bottom = false;
	};

	/**
		Edges moved by the handle at @a handle on a frame of @a size, from
		where that handle sits: on the left side it moves the left edge,
		in the middle of a side only that side's edge.
	*/
	inline Edges edgesForHandle(const QPointF &handle, const QSizeF &size)
	{
		Edges e;
		e.left = qFuzzyIsNull(handle.x());
		e.right = qFuzzyCompare(handle.x(), size.width());
		e.top = qFuzzyIsNull(handle.y());
		e.bottom = qFuzzyCompare(handle.y(), size.height());
		return e;
	}

	/**
		@a crop with the @a edges moved to @a point, kept inside @a bounds
		and never smaller than @a minimum on either side: an edge stops
		at the opposite one instead of crossing it.
	*/
	inline QRectF dragEdges(const QRectF &crop, Edges edges, const QPointF &point,
							const QRectF &bounds, qreal minimum)
	{
		const qreal x = std::clamp(point.x(), bounds.left(), bounds.right());
		const qreal y = std::clamp(point.y(), bounds.top(), bounds.bottom());
		QRectF r = crop;
		if (edges.left)   r.setLeft(std::min(x, crop.right() - minimum));
		if (edges.right)  r.setRight(std::max(x, crop.left() + minimum));
		if (edges.top)    r.setTop(std::min(y, crop.bottom() - minimum));
		if (edges.bottom) r.setBottom(std::max(y, crop.top() + minimum));
		return r;
	}

	/// @a crop moved by @a delta, but no further than @a bounds allow.
	inline QRectF moveWithin(const QRectF &crop, const QPointF &delta, const QRectF &bounds)
	{
		QRectF r = crop.translated(delta);
		r.moveLeft(std::clamp(r.left(), bounds.left(), bounds.right() - r.width()));
		r.moveTop(std::clamp(r.top(), bounds.top(), bounds.bottom() - r.height()));
		return r;
	}

	/// The whole-pixel crop rectangle closest to @a crop.
	inline QRect toPixels(const QRectF &crop)
	{
		const int left = int(std::lround(crop.left()));
		const int top = int(std::lround(crop.top()));
		return QRect(left, top,
					 int(std::lround(crop.right())) - left,
					 int(std::lround(crop.bottom())) - top);
	}
}

#endif // CROPGEOMETRY_H
