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
#ifndef LISTQUERYWIDGET_H
#define LISTQUERYWIDGET_H

#include <QCoreApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

/**
	@brief The ListQueryWidget class
	What a list table shows, in the "Add a table" dialog: a list the
	project database already answers (qet_device_list, ...), as a table on
	a folio that follows it (#1878 B9-B11). Nothing to choose: the table's
	look is set by the dialog around it, and a table too long for its
	folio goes on to further linked tables as the parts list does.
	Saved as a database table: its identifier and its query.
*/
class ListQueryWidget : public QWidget
{
	public:
		enum Kind {
			Device, ///< one row per device (qet_device_list)
			Wiring, ///< one row per wire, both ends as -K1:A1, cable and core
			Plc,    ///< one row per entry of a PLC's I/O table (qet_plc_io)
			Signal  ///< one row per signal: the wires that carry one potential (potential_view)
		};

		explicit ListQueryWidget(Kind kind, QWidget *parent = nullptr) :
			QWidget(parent), m_kind(kind)
		{
			auto *layout = new QVBoxLayout(this);
			auto *text = new QLabel(description(kind), this);
			text->setWordWrap(true);
			layout->addWidget(text);
			layout->addStretch();
		}

		Kind kind() const {return m_kind;}

		static QString modelIdentifier(Kind kind)
		{
			switch (kind) {
				case Device: return QStringLiteral("device_list");
				case Wiring: return QStringLiteral("wiring_table");
				case Plc: return QStringLiteral("plc_list");
				case Signal: return QStringLiteral("signal_list");
			}
			return QString();
		}

		static QString description(Kind kind)
		{
			switch (kind) {
				case Device: return QCoreApplication::translate("ListQueryWidget",
					"The table shows the project's devices, one row each: reference, "
					"description, designation, article, number of symbols and first "
					"sheet. It follows the labels and the information of the symbols.");
				case Wiring: return QCoreApplication::translate("ListQueryWidget",
					"The table shows the project's wires, one row each: wire number, "
					"both ends as device:terminal, cable, core and sheet. It follows the "
					"wiring and the labels.");
				case Plc: return QCoreApplication::translate("ListQueryWidget",
					"The table shows the I/O of the project's PLCs, one row per entry: PLC, "
					"address, type, function, comment, the device wired to it and its sheet. "
					"It follows the PLC's I/O table.");
				case Signal: return QCoreApplication::translate("ListQueryWidget",
					"The table shows the project's signals, one row each: the wire numbers "
					"and names of one potential, its type, the terminals it joins and the "
					"sheets it is on. It follows the wiring.");
			}
			return QString();
		}

		static QString queryStr(Kind kind)
		{
			switch (kind) {
				case Device:
					return QStringLiteral(
						"SELECT COALESCE(structure_id, '') AS device, "
						"COALESCE(description, '') AS description, "
						"COALESCE(designation, '') AS designation, "
						"COALESCE(articles, '') AS articles, "
						"COALESCE(symbols, 0) AS symbols, "
						"COALESCE(folio_number, '') AS folio_number "
						"FROM qet_device_list ORDER BY structure_id");
				case Wiring:
						//The wiring list's rows and order (wiring_list_view as
						//WiringListDialog reads it), a wire's ends named as
						//-K1:A1 -- the device, then its terminal unless the
						//device's name already carries it -- with its cable
						//and the core it is on
					return QStringLiteral(
						"WITH core AS (SELECT wire_uuid, MIN(core) AS core FROM qet_cable_cores "
						"WHERE wire_uuid IS NOT NULL GROUP BY wire_uuid) "
						"SELECT COALESCE(w.wire_number, '') AS wire_number, ")
						+ endSql(QStringLiteral("from")) + QStringLiteral(" AS from_end, ")
						+ endSql(QStringLiteral("to")) + QStringLiteral(" AS to_end, "
						"COALESCE(w.cable, '') AS cable, COALESCE(k.core, '') AS core, "
						"COALESCE(w.folio_number, '') AS folio_number "
						"FROM qet_wires w LEFT JOIN core k ON k.wire_uuid = w.wire_uuid "
						"WHERE w.from_device_uuid IS NOT NULL AND w.to_device_uuid IS NOT NULL "
						"ORDER BY w.folio_number, "
						"CASE WHEN w.wire_number GLOB '[0-9]*' THEN 0 ELSE 1 END, "
						"CAST(w.wire_number AS INTEGER), w.wire_number");
				case Plc:
					return QStringLiteral(
						"SELECT COALESCE(p.structure_id, '') AS plc, COALESCE(p.address, '') AS address, "
						"COALESCE(p.type, '') AS io_type, COALESCE(p.function, '') AS function, "
						"COALESCE(p.comment, '') AS comment, COALESCE(d.structure_id, '') AS device, "
						"COALESCE(p.folio_number, '') AS folio_number "
						"FROM qet_plc_io p LEFT JOIN qet_devices d ON d.device_uuid = p.slave_uuid "
						"ORDER BY p.structure_id, p.entry");
				case Signal:
						//potential_view is made when a query names it
						//(projectDataBase::newQuery)
					return QStringLiteral(
						"SELECT COALESCE(wire_numbers, '') AS wire_numbers, COALESCE(names, '') AS names, "
						"COALESCE(potential_type, '') AS potential_type, terminals, wires, "
						"COALESCE(first_folio_number, '') AS folio_number, folios "
						"FROM potential_view ORDER BY first_folio_number, wire_numbers");
			}
			return QString();
		}

	private:
			/// A wire's end @p side ("from" or "to") as -K1:A1
		static QString endSql(const QString &side)
		{
			return QStringLiteral(
				"COALESCE(w.%1_structure_id, '') || CASE WHEN COALESCE(w.%1_terminal, '') = '' "
				"OR substr(COALESCE(w.%1_structure_id, ''), -length(w.%1_terminal) - 1) = ':' || w.%1_terminal "
				"THEN '' ELSE ':' || w.%1_terminal END").arg(side);
		}

		Kind m_kind;
};

#endif // LISTQUERYWIDGET_H
