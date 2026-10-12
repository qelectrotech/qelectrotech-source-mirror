/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef CABLELISTROWS_H
#define CABLELISTROWS_H

#include <QByteArray>
#include <QCoreApplication>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

class QETProject;

/**
	@brief The CableList class

	The columns and the rows of the cable list (menu "Listes": the entry
	which puts such a table on a sheet, and the CSV export which writes
	the same table to a file).

	Everything which decides what a cable list row says lives here, so
	that the table drawn on the sheet and the exported CSV can never
	disagree: one row per cable, worked out once and read twice.

	The two ends of a cable are worked out the way the user asked for
	it:

	- wherever the cores of the cable land on a real piece (a terminal
	  strip, a device) is an end. Several cores landing on the same
	  piece are one end. A core which ends on a folio report is not an
	  end at all -- the cable goes on over there, and the landing on the
	  far sheet is the end instead, which is what makes a cable running
	  over several sheets show its start on the earlier sheet and its
	  target on the later one;
	- ends are ordered by sheet first (the earlier sheet wins), then by
	  height in the plan (the higher one wins), then left to right;
	  the first end is the start, the last one the target;
	- a cable whose cores never landed anywhere (not drawn yet, or only
	  wired in the middle) falls back to the two free ends of its drawn
	  line; when there is only one landing, the other side falls back to
	  the line end farthest away from it.
*/
class CableList
{
	Q_DECLARE_TR_FUNCTIONS(CableList)

	public:
		struct Column
		{
			/// The name the row hash, the saved file and the CSV use
			QString key;
			/// What the header over the column says
			QString label;
		};

			/// Every column the list can offer, in the order they are
			/// offered and exported
		static const QList<Column> &allColumns();
			/// The columns a fresh list starts with: all of them but the
			/// ones marked optional above
		static const QStringList &defaultKeys();
			/// The header text of one column, empty when the key is not
			/// a column of this list
		static QString labelOf(const QString &key);

			/// One row per cable, sorted: first by the sheet its start
			/// lies on, then from the top of that sheet down, the number
			/// column filled in along the way
		static QVector<QMap<QString, QString>> rows(QETProject *project);

			/// The whole list as CSV bytes, in the format every CSV
			/// export of this program writes (UTF-8 BOM, ';', every
			/// field quoted): every column, every cable, the line of
			/// column names included
		static QByteArray toCsv(QETProject *project);
			/// The same, but only the given columns, in the given
			/// order, and only with the line of column names when asked
			/// for -- what the export dialog writes. Unknown keys are
			/// dropped, and no usable key at all falls back to the
			/// default columns
		static QByteArray toCsv(QETProject *project, const QStringList &keys,
								bool include_headers);
};

#endif // CABLELISTROWS_H
