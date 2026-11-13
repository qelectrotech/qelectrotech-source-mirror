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
#ifndef REPLACESYMBOLS_H
#define REPLACESYMBOLS_H

#include "ElementsCollection/elementslocation.h"

#include <QList>

class Diagram;
class Element;
class QUndoCommand;

/**
	Replace placed symbols by another symbol from a collection, in one
	undo step: each new symbol takes the place, rotation, mirroring and
	identification (label, formula, plant, location, function, comment)
	of the one it replaces, and the wires come back on its terminals.
*/
namespace ReplaceSymbols
{
	struct Result {
			/// The undo step, or nullptr when nothing can be replaced
		QUndoCommand *command = nullptr;
			/// Wires that found no terminal on the new symbol: removed
		int lost_wires = 0;
			/// Why nothing is replaced, when command is nullptr
		QString problem;
	};

	Result build(Diagram *diagram, const QList<Element *> &elements,
				 const ElementsLocation &location);
}

#endif // REPLACESYMBOLS_H
