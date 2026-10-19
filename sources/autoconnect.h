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
#ifndef AUTOCONNECT_H
#define AUTOCONNECT_H

#include <QList>
#include <QRectF>
#include <QSet>

class Diagram;
class Element;
class Terminal;
class QUndoCommand;

/**
	Automatic connections kept up to date.

	A terminal connects to the nearest free terminal that lies in line with
	it, along its orientation, and faces it. The body of any other symbol
	on the way stops the search: a corner, a T-junction or a small
	interruption symbol is how the user steers it. The wires made this way
	are ordinary conductors flagged as automatic (Conductor::isAutomatic()):
	when a change leaves one of them out of line, or a symbol comes between
	its ends, it is removed, and its ends look for a new partner. A wire
	drawn by hand is never touched; the terminal it ends on is not free.

	Every function adds its steps under @a parent, so the change that
	caused them and the wires it made or removed undo as one step.
*/
namespace AutoConnect
{
	bool isOn(const Diagram *diagram);

	/**
		Bring the automatic wires of @a diagram up to date after a change.
		@param changed : symbols moved or placed; they are at their new place
		@param old_rects : the scene rects the moved symbols had before, so
		that terminals they no longer stand between find each other
		@param leaving : symbols being removed by the command @a parent
		holds; their wires go with them, the terminals at the far ends look
		for a new partner
	*/
	void update(Diagram *diagram,
				const QList<Element *> &changed,
				const QList<QRectF> &old_rects,
				QUndoCommand *parent,
				const QSet<Element *> &leaving = {});

	/**
		Connect every free terminal of @a diagram that has a partner.
		@return the number of wires made
	*/
	int connectWholeDiagram(Diagram *diagram, QUndoCommand *parent);

	/// The terminal @a from would connect to now, or nullptr
	Terminal *partner(const Terminal *from);
}

#endif // AUTOCONNECT_H
