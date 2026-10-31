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
#ifndef TERMINALSTRIPLABELS_H
#define TERMINALSTRIPLABELS_H

#include <QString>

class Diagram;
class TerminalStrip;

/**
	The label of a terminal strip (-XN1) at the first of its terminals on
	each folio, as EPLAN shows it: no free text to place and keep in line,
	it belongs to that terminal and moves with it. When a terminal is put
	before the first one, or the first one leaves the strip, the label goes
	to the new first terminal by itself. "First" is in reading order: the
	terminal furthest left, then furthest up.

	A project option, off by default (QETProject::terminalStripLabelsShown()).
	The label is worked out, never saved: it is not part of the file.

	The same goes for the bridges of the terminal strip editor, with an
	option of their own (QETProject::terminalStripBridgesShown()): two
	terminals of a strip side by side in a row, turned the same way and
	bridged together, are joined by a line along the row, from the last
	connection point of the first to the first of the next. It goes behind
	the terminals, so their connection points cover its ends.
*/
namespace TerminalStripLabels
{
		/// What the label of @a strip reads: its name, with the "-" of an
		/// item designation in front unless it has one
	QString text(const TerminalStrip *strip);

		/// Work the labels of @a diagram out again soon, once for many
		/// changes in a row (a drag moves the terminals on every step)
	void schedule(Diagram *diagram);

		/// Work the labels of @a diagram out now
	void refresh(Diagram *diagram);
}

#endif // TERMINALSTRIPLABELS_H
