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
#ifndef CONDUCTORDASHPATTERN_H
#define CONDUCTORDASHPATTERN_H

#include <QPen>

/**
	@brief The ConductorDashPattern struct
	Lengths of the dashes of a dashed or dash-dotted conductor.

	The lengths are in diagram units, not in multiples of the conductor
	width, so a thicker conductor keeps the same dashes. Scaling the page
	scales the pattern just as it scales the conductor.
	A pattern that is not enabled changes nothing: the conductor is drawn
	with QElectroTech's own dashes.
	Idea from PR #1650 by victorienea.
*/
struct ConductorDashPattern
{
	static constexpr double minimum = 0.1;
	static constexpr double maximum = 1000.0;

	bool   enabled = false;
	double dash    = 6.0;
	double gap     = 4.0;
	double dot     = 1.0;
	double dot_gap = 4.0;

	bool valid() const;
	bool apply(QPen &pen, Qt::PenStyle style) const;

	bool operator==(const ConductorDashPattern &other) const;
	bool operator!=(const ConductorDashPattern &other) const
	{
		return !(*this == other);
	}
};

#endif // CONDUCTORDASHPATTERN_H
