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
#ifndef PARTSEARCH_H
#define PARTSEARCH_H

#include "materiallist.h"

#include <QList>
#include <QString>
#include <QStringList>

/**
	@brief The PartSearch class
	Finds the parts of a material list a few words describe, the way a
	person types them: "contacter 24v schneider". No widget, so it stays
	testable.

	Every word must match one of the searched fields (references,
	designation, description, manufacturer, device type, category,
	supplier), case and accents ignored. A word matches, best first, as a
	whole word, as the beginning of a word, inside a field (a part number
	is kept whole, "100-M09" is found in it), or roughly: a word of 4 or
	more letters without digit may miss one letter (two from 10 letters).
	A "*" in a word stands for any characters ("LC1D*BD").
*/
class PartSearch
{
	public:
			/// The words of @p text (split on blanks)
		static QStringList words(const QString &text);
			/// @p text lower-cased, without its accents
		static QString fold(const QString &text);
			/// The distance of two words: inserted, deleted, replaced or
			/// swapped (neighbours) letters
		static int distance(const QString &a, const QString &b);
			/// -1 when a word of @p words does not match @p record, else how
			/// well they match: higher is better
		static int score(const MaterialRecord &record, const QStringList &words);
			/// The indexes of @p records @p text finds, best first, then by
			/// part number; all of them in list order for an empty text.
			/// At most @p limit of them when it is not negative. The parts
			/// numbered as @p first come before the others of an equal rank
		static QList<int> rank(const QList<MaterialRecord> &records, const QString &text, int limit = -1,
							   const QStringList &first = QStringList());
			/// The part number of @p record: its reference, the machine's
			/// else, else its designation
		static QString partNumber(const MaterialRecord &record);
};

#endif // PARTSEARCH_H
