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
#ifndef CONNECTIONSYMBOLS_H
#define CONNECTIONSYMBOLS_H

#include "ElementsCollection/elementslocation.h"

#include <QList>
#include <QString>

/**
	The symbols that sit on the wires themselves: a corner, a T-junction,
	an interruption point and the two folio cross-references. The
	Connections toolbar places each with one click. Which symbol a button
	places is kept in the settings, so a house collection can bring its
	own; the defaults come from the common collection, which has no
	interruption point.
*/
namespace ConnectionSymbols {
	/// TeeMirrored is the mirror image of Tee: a T-junction whose slant
	/// shows the feeding side cannot be turned into its mirror image, so
	/// both are offered, each in four turns
	/// TeeBranchLeft and TeeBranchRight: the double wire at the branch,
	/// the run from one side turning into it at a corner and the run from
	/// the other side meeting it with a slant; the common collection has
	/// neither
	enum Kind { Corner, Tee, Interruption, ReportOutgoing, ReportIncoming, TeeMirrored,
		    TeeBranchLeft, TeeBranchRight };

	QList<Kind> kinds();
		/// The command's id, for shortcuts and toolbars
	QString commandId(Kind kind);
	QString title(Kind kind);
	QString defaultPath(Kind kind);
		/// The collection path of the symbol, the default unless one was chosen
	QString path(Kind kind);
	ElementsLocation location(Kind kind);
	void setPath(Kind kind, const QString &path);
	void resetPath(Kind kind);
}

#endif
