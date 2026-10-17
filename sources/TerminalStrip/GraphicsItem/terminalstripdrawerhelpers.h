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
#ifndef TERMINALSTRIPDRAWERHELPERS_H
#define TERMINALSTRIPDRAWERHELPERS_H

#include "terminalstripdrawer.h"

class QPainter;

/**
 * Functions shared by the drawing styles of a terminal strip
 * (schematic style in terminalstripdrawer.cpp, realistic style in realisticterminalstripdrawer.cpp).
 */
namespace TerminalStripDrawer
{
	namespace Helpers
	{
			//Width (mm) of the reference terminal (2.5 mm2), used when the width of a terminal is unknown
		qreal referenceTerminalWidthMm();
			//Fill color and stripe color (can be invalid) of a terminal
		void terminalColors(const QSharedPointer<AbstractRealTerminalInterface> &real_t,
							QColor &fill, QColor &stripe);
			//Black or white, the more readable on background
		QColor contrastColor(const QColor &background);
			//Number of a terminal : "1" for "XAUZ1:1"
		QString terminalNumber(const QString &label);
			//Symbol of the type of the terminal (fuse, diode, led...) scaled to fit in box
		void drawTypeSymbol(QPainter *painter, const QRectF &box,
							ElementData::TerminalType type, bool led);
	}
}

#endif // TERMINALSTRIPDRAWERHELPERS_H
