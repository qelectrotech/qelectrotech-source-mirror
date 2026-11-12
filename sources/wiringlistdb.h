/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef WIRINGLISTDB_H
#define WIRINGLISTDB_H

#include <QString>
#include <QStringList>
#include <QVector>

class QETProject;

/**
	@brief One row of wiring_list_view: the wiring list as the project
	database holds it, one row per conductor.

	The ends a row names are already worked out: a wire which ends on a
	junction names what the junction's chain leads to instead of the
	junction itself (#1500).
*/
struct WiringListRow
{
	QString wire_number;
	QString from_element_uuid;
	QString from_element_label;
	QString from_terminal;
	QString from_terminal_uuid;
	QString from_terminal_index;
	QString to_element_uuid;
	QString to_element_label;
	QString to_terminal;
	QString to_terminal_uuid;
	QString to_terminal_index;
	QString diagram_position;
	QString conductor_uuid;

		/**
			@brief columns
			@return the names of the columns the wiring list export
			has always written, in the order it writes them
		*/
	static QStringList columns();

		/**
			@brief values
			@return this row's values, in the order of columns()
		*/
	QStringList values() const;
};

/**
	@brief wiringListRows
	@return the rows of wiring_list_view, in the order the list has
	always been written in, every end which is a junction replaced by
	where the junction's chain leads.

	The chain is the one rule both wiring lists share: the CSV export and
	this database list are meant to agree, so neither works it out on its
	own (#1500).

	@param project the project whose database is read
	@param unassigned_junctions, when given, is set to how many junctions
	stayed in the list unresolved: they have a wire and no source
	connection set, and more than one way on, so there was nothing to
	follow. A junction with a single way out of it is passed on without
	one (#1500)
	@param error, when given, receives why nothing could be read
*/
QVector<WiringListRow> wiringListRows(QETProject *project,
									  int *unassigned_junctions = nullptr,
									  QString *error = nullptr);

#endif // WIRINGLISTDB_H
