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
#ifndef SHAPETRANSFORM_H
#define SHAPETRANSFORM_H

#include <QPointF>
#include <QTransform>

/**
	@brief The ShapeTransform struct
	Rotation, skew and scale applied to a shape's local geometry around an
	arbitrary local pivot point.

	These five scalars plus a pivot are a convenient, directly-editable
	basis for 2D affine transforms: every handle in the UI changes exactly
	one field, which is what makes per-handle drag math simple. Note that
	a plain 2x2 linear map only has 4 true degrees of freedom, one fewer
	than (rotation, skewX, skewY, scaleX, scaleY) -- so this is not a
	*unique* representation of a matrix, only a convenient one. That
	redundancy is harmless for editing (nothing here ever needs to invert
	the forward direction) and only matters to decomposeLinear() below,
	which exists for later consumers (flattening a group transform onto
	its children, importing a foreign matrix) rather than everyday use.

	The resulting matrix is meant to be handed directly to
	QGraphicsItem::setTransform(). QGraphicsItem::transformOriginPoint()
	is deliberately NOT used anywhere in this design: it only centers
	QGraphicsItem's own rotation()/scale() convenience properties, and has
	no effect on a custom transform() matrix -- the pivot has to be baked
	into the matrix itself, as toMatrix() does. QGraphicsItem::pos()
	supplies the translation on top, unchanged from how shapes are
	already positioned and moved today.
*/
struct ShapeTransform
{
	qreal rotation = 0;   // degrees
	qreal skewX = 0;      // degrees
	qreal skewY = 0;      // degrees
	qreal scaleX = 1;
	qreal scaleY = 1;
	QPointF pivot;        // local coordinates; caller decides the default (usually local bbox center)

	bool isIdentity() const;

	// Rotate+shear+scale about the origin, ignoring pivot -- this is the
	// "linear part" used by compensatedPositionForNewPivot() and by
	// anything that needs to map a direction/offset rather than a point.
	QTransform linearPart() const;

	// The pivot-centered matrix to pass to QGraphicsItem::setTransform().
	QTransform toMatrix() const;

	bool operator==(const ShapeTransform &other) const;
	bool operator!=(const ShapeTransform &other) const { return !(*this == other); }
};

// Canonical decomposition of an arbitrary 2x2 linear map into the five
// scalars above, always returning skewY == 0 (all shear folded into
// skewX -- matching how most authoring tools represent shear on export).
// Rebuilding via ShapeTransform::linearPart() from the result reproduces
// the input matrix exactly; the individual scalar values are not
// guaranteed to match whatever scalars (if any) originally produced that
// matrix, only the matrix itself is guaranteed to match.
ShapeTransform decomposeLinear(const QTransform &linear);

// Position adjustment needed when the pivot moves, so the shape does not
// visibly jump on screen: call once, at the end of a pivot-handle drag,
// alongside setting the new pivot.
QPointF compensatedPositionForNewPivot(
		const QPointF &position,
		const QPointF &oldPivot,
		const QPointF &newPivot,
		const QTransform &linearPart);

#endif // SHAPETRANSFORM_H
