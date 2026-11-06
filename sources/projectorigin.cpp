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
#include "projectorigin.h"

/**
	@brief ProjectOrigin::toString
	@return the value written in the project file for @a mode
*/
QString ProjectOrigin::toString(Mode mode)
{
	switch (mode)
	{
	case Mode::TopLeft: return QStringLiteral("top-left");
	case Mode::TopRight: return QStringLiteral("top-right");
	case Mode::BottomLeft: return QStringLiteral("bottom-left");
	case Mode::BottomRight: return QStringLiteral("bottom-right");
	}
	return QStringLiteral("top-left");
}

/**
	@brief ProjectOrigin::fromString
	@return the mode written as @a string in a project file,
	Mode::TopLeft for anything else, so an unknown value.
*/
ProjectOrigin::Mode ProjectOrigin::fromString(const QString& string)
{
	if (string == QLatin1String("top-left")) return Mode::TopLeft;
	if (string == QLatin1String("top-right")) return Mode::TopRight;
	if (string == QLatin1String("bottom-left")) return Mode::BottomLeft;
	if (string == QLatin1String("bottom-right")) return Mode::BottomRight;
	return Mode::TopLeft;
}

/**
	@brief ProjectOrigin::reversedIsColumns
	@return true, for the sake of order, from right to left
*/
bool ProjectOrigin::reversedIsColumns(Mode mode)
{
	return (
		mode == ProjectOrigin::Mode::TopRight
		|| mode == ProjectOrigin::Mode::BottomRight);
}

/**
	@brief ProjectOrigin::reversedIsRows
	@return true, for the sake of order, from bottom to top
*/
bool ProjectOrigin::reversedIsRows(Mode mode)
{
	return (
		mode == ProjectOrigin::Mode::BottomLeft
		|| mode == ProjectOrigin::Mode::BottomRight);
}
