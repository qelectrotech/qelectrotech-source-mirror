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
#ifndef CABLECOPY_H
#define CABLECOPY_H

#include "cable.h"

#include <QList>
#include <QPointer>

class CablePart;
class Conductor;
class Diagram;
class QDomDocument;
class QDomElement;
class QUndoCommand;

/**
	@brief Carrying a drawn cable line over into a copy -- through the
	clipboard (Ctrl+C then Ctrl+V) or as a duplicate (Ctrl+D).

	What travels is the line and everything which made the cable what it
	was: its type, its installation, the place it runs through, its
	length, how many cores it has, which colours those have -- and where
	the colour labels of the copied section stand, so a copy arrives as
	a whole even onto a folio which holds no wire at all. What does not
	travel is which wire each core is on: a copy is a cable of its own
	with a number of its own, and it takes the wire each colour label
	happens to stand on at the place it is put -- those of the folio it
	lands on which no other cable describes. A wire another cable
	already holds is left to that cable, and a label put where there is
	no wire goes on standing there, to be wired when the line is pulled
	into place.

	A cable line is not one of the folio's items (the cable owns it),
	so Diagram::toXml() never writes it: the block travels beside the
	folio's own XML rather than inside it, and Diagram::fromXml() reads
	it back only when its caller asks to be handed the lines it built.
*/
namespace CableCopy
{
		///Put the cable lines selected on @a diagram into @a document
	void write(Diagram *diagram, QDomDocument &document);

		///Build new cables of their own for the lines @a root holds,
		///at the coordinates the fragment carries, with the colour
		///labels of the copied section carried over as they were.
		///Nothing is asked and no wire is taken yet: that needs the
		///place the lines have really been put at, which only the
		///caller knows
	QList<CablePart *> read(Diagram *diagram, const QDomElement &root);

		///One core of a line, and the wire it was given
	struct Bound
	{
		int core = -1;
		Conductor *conductor = nullptr;
	};

		///One line which was built, with what it had to take away
	struct Wired
	{
		QPointer<CablePart> part;
		QList<CableCoreTaken> taken;
			///The wires the colour labels of this line were given,
			///so they can be pointed at them again afterwards
		QList<Bound> bound;
	};

		///Give each colour label of a line the wire it stands on now
		///that the line is where it will stay.
		///
		///@a fresh lists the wires which came along with the copy
		///itself. They are new wires on this folio: the cable field
		///they carry describes the folio they were copied from, where
		///they used to be one of that cable's own wires -- and carried
		///over, that reference would make every one of them look like
		///a wire some other cable holds, so the copy would be refused
		///all of them and arrive without a single entry although it is
		///standing on wires of its own. They are let go of first, the
		///same way read() lets go of the wires the cores used to name.
		///
		///A wire the folio already held, which another cable
		///describes, is never taken away from that cable: nothing is
		///asked and nothing is written for it, and the colour label
		///standing on it simply names nothing until the user drags it
		///onto a free wire himself, which is where the question is
		///still asked. A label standing on no wire at all goes on
		///standing where it is, to be wired when the line is pulled
		///into place.
	QList<Wired> wire(Diagram *diagram,
					  const QList<CablePart *> &parts,
					  const QList<Conductor *> &fresh);

		///Point the cores @a wired lists at the wires they were given,
		///again: putting the copy on the undo stack may have renewed
		///the identity of a pasted conductor (PasteDiagramCommand::redo),
		///and a cable binds a wire by its identity. Nothing is asked and
		///nothing moves here. The cable fields are written out once more
		///afterwards, from the cables themselves, so the entries a copy
		///was given as it was put down are on the wires it really
		///landed on rather than on the identity they had one moment
		///earlier.
	void repoint(const QList<Wired> &wired);

		///Take a built line back off the folio without leaving a trace
		///in the undo history, for a copy which was refused
	void discard(Diagram *diagram, CablePart *part);
}

#endif // CABLECOPY_H
