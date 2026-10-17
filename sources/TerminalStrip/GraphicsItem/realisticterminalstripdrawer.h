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
#ifndef REALISTICTERMINALSTRIPDRAWER_H
#define REALISTICTERMINALSTRIPDRAWER_H

#include "terminalstripdrawer.h"

class QPainter;

/**
 * Drawing of a terminal strip at scale, seen from above, like the
 * mounting drawing of a cabinet. Used by the styles
 * RealisticPushIn and RealisticScrew of TerminalStripLayoutPattern.
 *
 * The strip is drawn from the left to the right :
 * end stop, group marker carrier (with the name of the strip), terminals, end stop.
 * The width of each terminal is its real width (mm) multiplied by the scale of the layout.
 * A terminal with several levels is drawn with one row of wire entries by level,
 * at the top and at the bottom.
 *
 * paint() fill @a xref_rects with the zone of each terminal which have an xref,
 * with the same indexes than the schematic style : the item use them to go to the
 * element on double click, to show the xref in a tool tip and to make clickable
 * links in an exported PDF. The terminal under @a hover_pos is outlined in blue.
 */
namespace TerminalStripDrawer
{
	namespace Realistic
	{
		QRectF boundingRect(const AbstractTerminalStripInterface *strip,
							const TerminalStripLayoutPattern &pattern);
		void paint(QPainter *painter,
				   const AbstractTerminalStripInterface &strip,
				   const TerminalStripLayoutPattern &pattern,
				   TerminalStripLayoutPattern::DrawStyle style,
				   bool preview_draw = false,
				   const QPointF &hover_pos = QPointF(),
				   QVector<XrefRect> *xref_rects = nullptr);
	}
}

#endif // REALISTICTERMINALSTRIPDRAWER_H
