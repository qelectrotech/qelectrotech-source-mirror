// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "conductordashpattern.h"

#include <cmath>

/**
	@brief ConductorDashPattern::valid
	@return true if every length is a finite number inside the allowed range
*/
bool ConductorDashPattern::valid() const
{
	for (const double length : {dash, gap, dot, dot_gap})
	{
		if (!std::isfinite(length) || length < minimum || length > maximum)
			return false;
	}
	return true;
}

/**
	@brief ConductorDashPattern::apply
	Set the dashes of \a pen. The style is always set on the pen; the
	lengths only when this pattern is enabled, valid and \a style is
	dashed or dash-dotted.
	@param pen : pen to change
	@param style : style of the conductor
	@return true if the custom lengths were applied, false if the pen has
	only the plain style and the caller must set the default dashes
*/
bool ConductorDashPattern::apply(QPen &pen, Qt::PenStyle style) const
{
	pen.setStyle(style);
	if (!enabled || !valid()
			|| (style != Qt::DashLine && style != Qt::DashDotLine))
		return false;

		//Qt counts the lengths in pen widths, and treats a zero width
		//cosmetic pen as one pixel wide
	const qreal width = pen.widthF() > 0 ? pen.widthF() : 1.0;

	QVector<qreal> pattern{dash / width, gap / width};
	if (style == Qt::DashDotLine)
		pattern << dot / width << dot_gap / width;

		//A square cap would make every dash longer, and every gap shorter,
		//by the width of the pen
	pen.setCapStyle(Qt::FlatCap);
	pen.setDashPattern(pattern);
	return true;
}

bool ConductorDashPattern::operator==(const ConductorDashPattern &other) const
{
	return enabled == other.enabled
			&& dash == other.dash
			&& gap == other.gap
			&& dot == other.dot
			&& dot_gap == other.dot_gap;
}
