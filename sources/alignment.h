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

#include <QPointF>

/**
	The geometry behind the align commands, kept free of any scene so it
	can be tested on its own.
*/
namespace Alignment
{
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
}

#endif // ALIGNMENT_H
