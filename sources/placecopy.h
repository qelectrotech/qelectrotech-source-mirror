// SPDX-License-Identifier: GPL-2.0-or-later
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
#ifndef PLACECOPY_H
#define PLACECOPY_H

#include <QList>
#include <QString>

class Diagram;
class QETProject;

/**
	@brief The PlaceCopy namespace
	Copies a plant (the = aspect) under a new name, in one undo step: every
	folio whose title block holds the plant is copied with all it holds, and
	the copies are put after the plant's last folio.

	In the copies the plant is the new name: in the title block, in the
	structure boxes and in the symbols that held the old one themselves.
	Labels are kept as they are, so a copied -K1 is =NEW-K1 beside =OLD-K1.
	Cross-references between copied symbols (coil and contacts, folio
	reports) link the copies with each other, never with the originals;
	a link to a symbol outside the plant is not copied. Every copied item
	gets a new uuid. The plant's wire counters are given to the new plant,
	so wires drawn in the copy continue after the copied numbers.
*/
namespace PlaceCopy
{
		/// What a copy would make
	struct Plan
	{
		QList<Diagram *> folios;        ///< title block holds the plant
			/// Symbols on those folios whose plant is another one (their
			/// own, or a structure box's): their copies keep that plant, so
			/// they have the same designation as the originals
		int other_plant_symbols = 0;
	};

	bool isValid(QETProject *project, const QString &plant,
				 const QString &new_name, QString *why = nullptr);
	Plan collect(QETProject *project, const QString &plant);
	bool copy(QETProject *project, const QString &plant, const QString &new_name);
}

#endif // PLACECOPY_H
