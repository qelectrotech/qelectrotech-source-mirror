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
#ifndef BRIDGECOPY_H
#define BRIDGECOPY_H

class Bridge;
class Diagram;
class QDomDocument;
class QDomElement;

#include <QList>
#include <QUndoCommand>

/**
	@brief Carrying a drawn bridge line over into a copy -- through the
	clipboard (Ctrl+C then Ctrl+V) or as a duplicate (Ctrl+D).

	What travels is the line and everything which made the bridge what
	it was: its colour, how thick it is drawn, how many Klemmen it may
	hold, the name it answers to and the voltage it carries. What does
	not travel is where its ends were docked and which terminals it
	landed on: a copy is a bridge of its own, and it lands on the
	terminals its line happens to cross where it is put down -- nothing
	at all where the rules of a bridge say that place holds none.

	A bridge line is one of the folio's items (the folio draws it), but
	its bridge belongs to the project: the block travels beside the
	folio's own XML rather than inside it, the way a cable line does.
*/
namespace BridgeCopy
{
		///Put the bridge lines selected on @a diagram into @a document
	void write(Diagram *diagram, QDomDocument &document);

		/**
			Build new bridges of their own for the lines @a root holds,
			their lines on @a diagram at the very coordinates they were
			copied at. The reading which brought them in moves them to
			where they are put down (@sa Diagram::fromXml()); what they
			land on is worked out afterwards, once they are there.
			@return the bridges built, each of them not held by the
			project yet: the step which puts them down holds them
			(@sa AddBridgeCommand).
		*/
	QList<Bridge *> read(Diagram *diagram, const QDomElement &root);

		/**
			Give the lines of @a bridge the terminals they cross where
			they stand now -- the way a drawn line takes them -- and
			@return the commands which put those lines down: one per
			line, the first of them the one which holds the bridge.
			What the rules of a bridge refuse (its limit, its
			Klemmleiste, the tracks already run in) leaves the line
			standing where it is, landing on nothing until the hand
			pulls it into a place it may be built.
		*/
	QList<QUndoCommand *> land(Diagram *diagram, Bridge *bridge);

		/**
			Take a pasted bridge back off the folio before anything was
			written for it -- the placement was called off or never
			finished, so there is no step to take it away.
		*/
	void discard(Diagram *diagram, Bridge *bridge);
}

#endif // BRIDGECOPY_H
