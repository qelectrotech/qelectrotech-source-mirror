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

/**
 * @file kwidgetsaddons_export.h
 * @brief Minimal export macros for KWidgetsAddons.
 * Empty export macro;  original KDE uses dllexport/dllimport on Windows for shared builds.
 * BUILD_DEPRECATED_SINCE always 1 for bundled build.
 * If ever the thirdparty/kcoreaddons minimal library needs to be dynamically loaded, CMake's GenerateExportHeader
 * function can be used.
 */

#define KWIDGETSADDONS_EXPORT
#define KWIDGETSADDONS_BUILD_DEPRECATED_SINCE(major, minor) 1
