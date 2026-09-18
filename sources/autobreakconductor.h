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
#ifndef AUTOBREAKCONDUCTOR_H
#define AUTOBREAKCONDUCTOR_H

#include <QSet>
#include <QList>

class Diagram;
class Element;
class Terminal;
class Conductor;
class QUndoCommand;

/**
	@brief autoBreakConductors
	For each terminal of @a element, check if its dock point lies on an
	existing conductor. If so, break the conductor and reconnect through the
	element's terminal.  Broken conductors from the same circuit (sharing the
	same far-end endpoint) are broken together; independent crossing
	conductors are left untouched.
	@param diagram            the diagram containing the conductors
	@param element            the element whose terminals trigger breaks
	@param parent             undo command under which break/reconnect
	                          sub-commands are created
	@param conductors_handled shared list of conductors already claimed by
	                          a previous call in the same batch (prevents
	                          double-processing when multiple elements are
	                          broken in one pass)
	@param used_terminals     shared set of terminals already used as
	                          connect_to or other_terminal in the same batch
	@return set of terminals that were connected to a broken conductor's
	        far-end (useful for preventing duplicate auto-connect)
*/
QSet<Terminal *> autoBreakConductors(
	Diagram *diagram,
	Element *element,
	QUndoCommand *parent,
	QList<Conductor *> &conductors_handled,
	QSet<Terminal *> &used_terminals);

#endif // AUTOBREAKCONDUCTOR_H
