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
			//What is needed to draw the cable under a terminal
		struct CableCell
		{
			qreal x{0};          ///< x of the middle of the terminal, in the strip coordinates
			qreal bottom{0};     ///< y of the bottom of the terminal
			QString cable;       ///< name of the cable (hose)
			QString wire;        ///< color / number of the wire
			bool shield{false};  ///< true if the terminal is the shield of the cable
			QSharedPointer<AbstractRealTerminalInterface> terminal; ///< to ask where the cable goes
		};
			//Draw the cables under the terminals, whichever style draws them :
			//consecutive terminals with the same cable share one cable, which ends
			//together in a short stub and a box naming where it goes (see drawCables()
			//in terminalstripdrawer.cpp for the whole drawing)
		void drawCables(QPainter *painter, const QVector<CableCell> &cells,
						const TerminalStripLayoutPattern &pattern);
			//How much deeper than the end zone (m_cable_end_length) the boxes of
			//the cables need: a box reaching into another one is set lower than
			//the others, and the strip must reserve the room for it or the lower
			//box is cut off. The same rule the drawing uses, worked out of the
			//same cells, so the two can never disagree.
		qreal cableBoxesExtraDepth(const QVector<CableCell> &cells,
								   const TerminalStripLayoutPattern &pattern);
	}
}

#endif // TERMINALSTRIPDRAWERHELPERS_H
