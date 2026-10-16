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
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef PROJECTORIGIN_H
#define PROJECTORIGIN_H

#include <QString>

/**
	@brief ProjectOrigin namespace
	The "origin" defines the corner from which the numbering of rows and columns on
	a sheet begins.
*/
namespace ProjectOrigin
{
	/// which corner to use as the origin (coordinate system origin)
	enum class Mode
	{
		TopLeft = 0,	///< Top - left corner(default)
		TopRight = 1,   ///< Top - right
		BottomLeft = 2,	///< Bottom - left
		BottomRight = 3 ///< Bottom - right
	};

	QString toString(Mode mode);
	Mode	fromString(const QString& string);
} // namespace ProjectOrigin

#endif // PROJECTORIGIN_H