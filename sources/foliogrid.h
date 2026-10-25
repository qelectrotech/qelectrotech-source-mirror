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
#ifndef FOLIOGRID_H
#define FOLIOGRID_H

#include <QString>
#include <QtGlobal>
#include <QVariant>
#include <cmath>
#include <limits>

/**
	The folio grid: the step, in pixels, that symbols snap to. Both steps
	are kept in the settings; the preferences page only offers 1 and up,
	but a hand-edited or damaged settings file can hold 0, a negative
	number or text, and the code that divides by the step or loops over it
	must never see such a value.
*/
namespace FolioGrid
{
		/// QSettings keys holding the two steps.
	inline const QString x_key{QStringLiteral("diagrameditor/Xgrid")};
	inline const QString y_key{QStringLiteral("diagrameditor/Ygrid")};

	/**
		@return the step held in @p value, a settings entry; or @p fallback,
		the built-in step, when the entry is missing, not a number, less
		than 1, or more than an int holds (QVariant::toInt() wraps such a
		number around instead of failing).
	*/
	inline int step(const QVariant &value, int fallback)
	{
		bool ok = false;
		const qlonglong s = value.toLongLong(&ok);
		if (!ok || s < 1 || s > std::numeric_limits<int>::max())
			return fallback;
		return int(s);
	}

		/// QSettings key of the smallest distance, in pixels on screen,
		/// at which grid dots are still drawn (General > Appearance)
	inline const QString min_dot_spacing_key{QStringLiteral("diagrameditor/grid_min_dot_spacing")};
		/// Grid dots closer than this on screen, in pixels, read as a
		/// gray haze rather than as a grid. With the default step of 10,
		/// the dots go below a zoom of 75 %.
	constexpr double default_min_dot_spacing = 7.5;
		/// The smallest and largest distance the preferences page offers
	constexpr double min_dot_spacing_lowest = 1.0;
	constexpr double min_dot_spacing_highest = 50.0;

	/**
		@return the smallest dot distance held in @p value, a settings
		entry, or the default when it is missing, not a number, not
		finite, or outside what the preferences page offers: a hand-edited
		"inf" or 1e9 would hide the grid for good while the page showed
		another value.
	*/
	inline double minDotSpacing(const QVariant &value)
	{
		bool ok = false;
		const double spacing = value.toDouble(&ok);
		return ok && std::isfinite(spacing)
				&& spacing >= min_dot_spacing_lowest && spacing <= min_dot_spacing_highest
				? spacing : default_min_dot_spacing;
	}

	/**
		@return true when the grid dots are drawn: when two of them, @p x_step
		and @p y_step apart on the folio, stand at least @p min_spacing
		pixels apart on screen at @p zoom (1 is 100 %). Whatever the step,
		they go when they would crowd; with the default step of 10 and
		spacing of 7.5, below a zoom of 75 %.
	*/
	inline bool dotsShown(int x_step, int y_step, double zoom,
			      double min_spacing = default_min_dot_spacing)
	{
		return qMin(x_step, y_step) * zoom >= min_spacing;
	}
}

#endif // FOLIOGRID_H
