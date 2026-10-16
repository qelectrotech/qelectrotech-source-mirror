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
#ifndef TERMINALCATALOG_H
#define TERMINALCATALOG_H

#include <QVector>

/**
 * @brief The TerminalCatalog namespace
 * Data shared between the terminal strip editor and the element editor :
 * usual sections (mm2) and widths (mm) of the terminals sold by the manufacturers.
 * Only the values found in the catalogs of the manufacturers are listed,
 * any other value can be typed by the user.
 */
namespace TerminalCatalog
{
	/**
	 * Width (mm) of a terminal sold by a manufacturer.
	 * For ABB Entrelec the value is the spacing (pitch) given by the catalog.
	 */
	struct WidthEntry { double mm; const char *label; double section; };

	inline const QVector<double> &sections()
	{
		static const QVector<double> sections_ {0.5, 0.75, 1, 1.5, 2.5, 4, 6, 10, 16, 25, 35, 50, 70, 95, 120, 150, 185, 240, 300};
		return sections_;
	}

	inline const QVector<WidthEntry> &widthEntries()
	{
		static const QVector<WidthEntry> entries_ {
			{5.1,  "Weidmüller WDU 2.5", 2.5},
			{5.2,  "WAGO 2002 (TOPJOB S)", 2.5},
			{6.0,  "ABB Entrelec M 4/6", 4.0},
			{6.1,  "Weidmüller WDU 4", 4.0},
			{6.2,  "Phoenix Contact UT 4", 4.0},
			{8.0,  "ABB Entrelec M 6/8", 6.0},
			{8.0,  "ABB Entrelec M 4/8", 4.0},
			{8.2,  "Phoenix Contact UT 6", 6.0},
			{10.0, "ABB Entrelec M 10/10", 10.0},
			{10.2, "Phoenix Contact UT 10", 10.0},
			{11.9, "Weidmüller WDU 16/ZA", 16.0},
			{12.2, "Phoenix Contact UT 16", 16.0},
			{16.0, "Phoenix Contact UT 35", 35.0},
			{16.0, "Weidmüller WDU 35N", 35.0},
			{26.0, "ABB Entrelec M 35/26.FF", 35.0},
			{31.0, "ABB Entrelec M 70/31.AF", 70.0},
			{35.0, "ABB Entrelec M 120/35.F", 120.0},
			{42.0, "ABB Entrelec M 300/42.FF", 300.0}
		};
		return entries_;
	}
}

#endif // TERMINALCATALOG_H
