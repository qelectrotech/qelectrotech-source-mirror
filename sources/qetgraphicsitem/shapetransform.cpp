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
#include "shapetransform.h"

#include <QtCore/qmath.h>

bool ShapeTransform::isIdentity() const
{
	return qFuzzyIsNull(rotation) && qFuzzyIsNull(skewX) && qFuzzyIsNull(skewY)
		&& qFuzzyCompare(scaleX, qreal(1)) && qFuzzyCompare(scaleY, qreal(1));
}

QTransform ShapeTransform::linearPart() const
{
	QTransform t;
	t.rotate(rotation);
	t.shear(qTan(qDegreesToRadians(skewX)), qTan(qDegreesToRadians(skewY)));
	t.scale(scaleX, scaleY);
	return t;
}

QTransform ShapeTransform::toMatrix() const
{
	QTransform t;
	t.translate(pivot.x(), pivot.y());
	t.rotate(rotation);
	t.shear(qTan(qDegreesToRadians(skewX)), qTan(qDegreesToRadians(skewY)));
	t.scale(scaleX, scaleY);
	t.translate(-pivot.x(), -pivot.y());
	return t;
}

bool ShapeTransform::operator==(const ShapeTransform &other) const
{
	return qFuzzyCompare(rotation, other.rotation)
		&& qFuzzyCompare(skewX, other.skewX)
		&& qFuzzyCompare(skewY, other.skewY)
		&& qFuzzyCompare(scaleX, other.scaleX)
		&& qFuzzyCompare(scaleY, other.scaleY)
		&& pivot == other.pivot;
}

ShapeTransform decomposeLinear(const QTransform &m)
{
	const qreal scaleX = qSqrt(m.m11() * m.m11() + m.m12() * m.m12());
	const qreal rotation = qRadiansToDegrees(qAtan2(m.m12(), m.m11()));
	const qreal rad = qDegreesToRadians(rotation);
	const qreal c = qCos(rad), s = qSin(rad);

	// v * R(-rotation): undo the rotation, leaving the pure shear+scale
	// contribution the local y-axis picked up from Shear(sh,0)*Scale(sx,sy).
	const qreal vx = m.m21() * c + m.m22() * s;
	const qreal scaleY = -m.m21() * s + m.m22() * c;
	const qreal skewX = qFuzzyIsNull(scaleY) ? 0.0 : qRadiansToDegrees(qAtan(vx / scaleY));

	ShapeTransform result;
	result.rotation = rotation;
	result.skewX = skewX;
	result.skewY = 0;
	result.scaleX = scaleX;
	result.scaleY = scaleY;
	return result;
}

QPointF compensatedPositionForNewPivot(
		const QPointF &position,
		const QPointF &oldPivot,
		const QPointF &newPivot,
		const QTransform &linearPart)
{
	return position + (oldPivot - linearPart.map(oldPivot))
	                 - (newPivot - linearPart.map(newPivot));
}
