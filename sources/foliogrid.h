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
#include <QVariant>
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
}

#endif // FOLIOGRID_H
