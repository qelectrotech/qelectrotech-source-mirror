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
#ifndef BORDERCELLLABELS_H
#define BORDERCELLLABELS_H

#include <QString>

/// The labels the border of a folio writes on its rows and columns, shared
/// by BorderTitleBlock::draw() and the cell rulers of DiagramView so the
/// two cannot disagree. Header-only so it can be unit-tested directly --
/// see tests/qttest/tst_bordercelllabels.cpp.
namespace BorderCellLabels {

	/// @return the label of row \a row, counted from 1: A..Z, then AA, AB...
	/// (the sequence BorderTitleBlock::incrementLetters() walks through).
	inline QString rowLabel(int row)
	{
		QString label;
		while (row > 0) {
			--row;
			label.prepend(QChar('A' + row % 26));
			row /= 26;
		}
		return label;
	}

	/// @return the label of column \a column, counted from 1. When
	/// \a starts_at_zero (the "border-columns_0" setting) the first column
	/// is labelled 0.
	inline QString columnLabel(int column, bool starts_at_zero)
	{
		return QString::number(starts_at_zero ? column - 1 : column);
	}
}

#endif // BORDERCELLLABELS_H
