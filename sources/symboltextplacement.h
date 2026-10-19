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
#ifndef SYMBOLTEXTPLACEMENT_H
#define SYMBOLTEXTPLACEMENT_H

#include <QIcon>
#include <QList>
#include <Qt>

class Element;
class QUndoCommand;

/**
	Place the texts of a symbol -- its label and the notes beside it -- as
	one block at one of nine points around its drawing, the nine of the
	text alignment dialog. Left of the symbol, the lines are aligned right,
	so that the block hangs on the symbol and a longer text grows away
	from it; right of it, aligned left; above, below and on it, centred.
	The lines keep their order and their spacing; a group of texts is
	placed as one line of the block, with the same alignment.
*/
namespace SymbolTextPlacement
{
		/// Room between the drawing of the symbol and the block
	constexpr double gap = 3;

	/**
		@param elements : the symbols whose texts to place
		@param where : the side of the drawing, as a 9-point alignment:
		Qt::AlignLeft | Qt::AlignVCenter is left of the symbol, halfway
		up; Qt::AlignCenter is on it
		@return one undo step that places them (already placed), or
		nullptr when none of the symbols has a text
	*/
	QUndoCommand *place(const QList<Element *> &elements, Qt::Alignment where);

	/**
		@return a picture of @a where: a grey square for the symbol, and
		two bars for the lines of text where they will go, lined up as
		they will be (left of the symbol, aligned right)
	*/
	QIcon icon(Qt::Alignment where);
}

#endif // SYMBOLTEXTPLACEMENT_H
