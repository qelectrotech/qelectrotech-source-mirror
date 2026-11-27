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
#ifndef CLEARANCECHECK_H
#define CLEARANCECHECK_H

#include <QList>
#include <QPointF>

class Conductor;
class Diagram;
class DiagramTextItem;

/**
	@brief The ClearanceCheck namespace
	The two drawing rules of IEC 61082-1 that need a unit, read from the
	built items of one folio (step 8 of docs/mm-scale.md):

	- 7.1.3.4: parallel wire segments are at least 1 M apart where they run
	  side by side, and at least 2 M apart where a text lies between them;
	- 7.1.3.5: a text does not cross a wire other than its own.

	M is one step of the grid, 10 px, whatever length that is on paper
	(QETProject::moduleMm()). Wires of QElectroTech run along the
	axes, so each rule is a sweep over the segments sorted across their
	direction: O(n log n) and what the neighbours found cost, not n squared.
	Segments of one wire, of two wires on one terminal, and segments that
	touch end to end are never reported. Nothing is changed.
*/
namespace ClearanceCheck
{
	struct Clearance
	{
		Conductor *first = nullptr;
		Conductor *second = nullptr;
		qreal distance = 0;       ///< between the two segments, in px
		bool text = false;        ///< closer than 2 M, with a text between
		QPointF where;            ///< in the folio, the middle of what
								  ///< they share
	};

	struct TextOnLine
	{
		DiagramTextItem *text = nullptr;
			/// The wire under the text, the first met on this folio
		Conductor *conductor = nullptr;
			/// The potential the text crosses, by its wire with the
			/// lowest uuid, which may be on another folio: what the
			/// finding is told from another by
		Conductor *potential = nullptr;
		QPointF where;                    ///< in the folio
	};

		/// The most findings of one folio each check returns: past it the
		/// folio is a mess the first findings show, and the check says how
		/// many more there are
	constexpr int limit = 200;

		/// 7.1.3.4 on @p diagram: one finding for each pair of wires, the
		/// first @p cap from the top of the folio; @p omitted, if given,
		/// gets how many more there are
	QList<Clearance> clearances(Diagram *diagram, int *omitted = nullptr,
								int cap = limit);
		/// 7.1.3.5 on @p diagram: one finding for each text and
		/// potential, capped as clearances() is
	QList<TextOnLine> textsOnLines(Diagram *diagram, int *omitted = nullptr,
								   int cap = limit);
}

#endif // CLEARANCECHECK_H
