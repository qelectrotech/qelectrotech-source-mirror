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
#include "wiringlistdb.h"

#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "wiringlistexport.h"

#include <QHash>
#include <QMap>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <algorithm>
#include <utility>

namespace {

	/**
		@brief canonicalUuid
		@param uuid as a definition or a database row holds it
		@return the uuid the way the project file writes it: with the
		braces and lowercase, whichever of the two the reader was given
	*/
QString canonicalUuid(const QString &uuid)
{
	const QUuid id(uuid);
	return id.isNull() ? uuid : id.toString();
}

	/**
		@brief connectionKey
		@param element the symbol a row's end sits on
		@param terminal its connection
		@return the key the chain reader looks such an end up by
*/
QString connectionKey(const QString &element, const QString &terminal)
{
	return element + QLatin1Char('|') + terminal;
}

	/// The columns of wiring_list_view this row is built from, in the
	/// order the query below reads them
enum Column
{
	ColConductorUuid,
	ColWireNumber,
	ColFromElementUuid,
	ColFromElementLabel,
	ColFromTerminal,
	ColFromTerminalUuid,
	ColFromTerminalIndex,
	ColToElementUuid,
	ColToElementLabel,
	ColToTerminal,
	ColToTerminalUuid,
	ColToTerminalIndex,
	ColDiagramPosition
};

} // namespace

QStringList WiringListRow::columns()
{
	return QStringList{
		QStringLiteral("wire_number"),
		QStringLiteral("from_element_label"),
		QStringLiteral("from_terminal"),
		QStringLiteral("to_element_label"),
		QStringLiteral("to_terminal"),
		QStringLiteral("diagram_position"),
		QStringLiteral("conductor_uuid"),
		QStringLiteral("from_terminal_index"),
		QStringLiteral("from_terminal_uuid"),
		QStringLiteral("to_terminal_index"),
		QStringLiteral("to_terminal_uuid")};
}

QStringList WiringListRow::values() const
{
	return QStringList{wire_number,
					   from_element_label,
					   from_terminal,
					   to_element_label,
					   to_terminal,
					   diagram_position,
					   conductor_uuid,
					   from_terminal_index,
					   from_terminal_uuid,
					   to_terminal_index,
					   to_terminal_uuid};
}

QVector<WiringListRow> wiringListRows(QETProject *project,
									  int *unassigned_junctions,
									  QString *error)
{
	QVector<WiringListRow> result;
	if (unassigned_junctions) {
		*unassigned_junctions = 0;
	}
	if (error) {
		error->clear();
	}
	if (!project) {
		return result;
	}

		//The database is built lazily, and a conductor's row is only as
		//fresh as the last thing that touched it: refresh before reading
	project->dataBase()->updateDB();

	QString query_error;
	QSqlQuery query = project->dataBase()->newQuery(
		QStringLiteral(
			"SELECT conductor_uuid, wire_number, from_element_uuid, from_element_label,"
			" from_terminal, from_terminal_uuid, from_terminal_index,"
			" to_element_uuid, to_element_label, to_terminal, to_terminal_uuid,"
			" to_terminal_index, diagram_position"
			" FROM wiring_list_view"
			//Wire numbers are text, so a plain sort puts "10" before "9".
			//Numeric ones first, ordered by value; anything non-numeric
			//after, ordered as text. The trailing wire_number keeps ties
			//stable.
			" ORDER BY diagram_position,"
			" CASE WHEN wire_number GLOB '[0-9]*' THEN 0 ELSE 1 END,"
			" CAST(wire_number AS INTEGER),"
			" wire_number"),
		&query_error);
	if (!query_error.isEmpty() || !query.exec()) {
		if (error) {
			*error = query_error.isEmpty() ? query.lastError().text() : query_error;
		}
		return result;
	}

	QVector<WiringListRow> rows;
	while (query.next())
	{
		WiringListRow row;
		row.conductor_uuid = query.value(ColConductorUuid).toString();
		row.wire_number = query.value(ColWireNumber).toString();
		row.from_element_uuid = query.value(ColFromElementUuid).toString();
		row.from_element_label = query.value(ColFromElementLabel).toString();
		row.from_terminal = query.value(ColFromTerminal).toString();
		row.from_terminal_uuid = query.value(ColFromTerminalUuid).toString();
		row.from_terminal_index = query.value(ColFromTerminalIndex).toString();
		row.to_element_uuid = query.value(ColToElementUuid).toString();
		row.to_element_label = query.value(ColToElementLabel).toString();
		row.to_terminal = query.value(ColToTerminal).toString();
		row.to_terminal_uuid = query.value(ColToTerminalUuid).toString();
		row.to_terminal_index = query.value(ColToTerminalIndex).toString();
		row.diagram_position = query.value(ColDiagramPosition).toString();
		rows.append(row);
	}

		//The symbols the rows name, by the labels the database holds for
		//them, and the junctions among them with the roles their
		//definitions give. The roles are not in the database: they belong
		//to the symbol, so every copy of it answers alike (#1500)
	QMap<QString, ElementInfo> elements;
	for (const WiringListRow &row : std::as_const(rows))
	{
		ElementInfo &from = elements[row.from_element_uuid];
		if (from.label.isEmpty()) {
			from.label = row.from_element_label;
		}
		ElementInfo &to = elements[row.to_element_uuid];
		if (to.label.isEmpty()) {
			to.label = row.to_element_label;
		}
	}

	const auto addJunction = [&elements](Element *element) {
		if (!element || !(element->linkType() & Element::Junction)) {
			return;
		}
		const JunctionRoles roles = junctionRolesOf(element);
		ElementInfo &info = elements[element->uuid().toString()];
		info.isJunction = true;
		info.junctionSource = canonicalUuid(roles.source);
		for (const QString &target : roles.targets) {
			info.junctionTargets << canonicalUuid(target);
		}
	};
	for (Diagram *folio : project->folios())
	{
		if (folio->isLoaded())
		{
			for (Element *element : folio->elements()) {
				addJunction(element);
			}
		}
		else
		{
			const QHash<QUuid, Element *> partial = folio->partialElements();
			for (Element *element : partial) {
				addJunction(element);
			}
		}
	}

		//The same rows the CSV export works out, written through the one
		//rule both lists share
	QList<ConductorData> conductors;
	conductors.reserve(rows.size());
	for (int i = 0 ; i < rows.size() ; ++i)
	{
		const WiringListRow &row = rows.at(i);
		ConductorData conductor;
		conductor.index = i;
		conductor.el1_uuid = row.from_element_uuid;
		conductor.el2_uuid = row.to_element_uuid;
		conductor.terminal1_uuid = row.from_terminal_uuid;
		conductor.terminal2_uuid = row.to_terminal_uuid;
		conductor.element1_label = row.from_element_label;
		conductor.element2_label = row.to_element_label;
		conductor.terminalname1 = row.from_terminal;
		conductor.terminalname2 = row.to_terminal;
		conductor.folio = row.diagram_position;
		conductor.folio_index = row.diagram_position.toInt();
		conductors.append(conductor);
	}
	WiringListExport::resolveJunctionRuns(conductors, elements,
										  unassigned_junctions);

		//Where each connection stands in the list: a resolved row takes
		//its ends from other wires, and the index of a terminal belongs
		//to the terminal rather than to any one of them
	QHash<QString, QString> terminal_indexes;
	for (const WiringListRow &row : std::as_const(rows))
	{
		terminal_indexes.insert(
			connectionKey(row.from_element_uuid, row.from_terminal_uuid),
			row.from_terminal_index);
		terminal_indexes.insert(
			connectionKey(row.to_element_uuid, row.to_terminal_uuid),
			row.to_terminal_index);
	}

		//A resolved row takes the place of the row it replaces -- its
		//index is that row's -- so the list keeps the order the database
		//gave it instead of collecting the resolved ones at the end
	std::sort(conductors.begin(), conductors.end(),
			  [](const ConductorData &a, const ConductorData &b) {
				  return a.index < b.index;
			  });

	result.reserve(conductors.size());
	for (const ConductorData &conductor : std::as_const(conductors))
	{
		const WiringListRow &replaced = rows.at(conductor.index);
		WiringListRow row;
			//The wire the row stands for: the one it kept, or the one
			//it was written from
		row.conductor_uuid = replaced.conductor_uuid;
		row.wire_number = replaced.wire_number;
		row.from_element_uuid = conductor.el1_uuid;
		row.from_element_label = conductor.element1_label;
		row.from_terminal = conductor.terminalname1;
		row.from_terminal_uuid = conductor.terminal1_uuid;
		row.from_terminal_index = terminal_indexes.value(
			connectionKey(row.from_element_uuid, row.from_terminal_uuid));
		row.to_element_uuid = conductor.el2_uuid;
		row.to_element_label = conductor.element2_label;
		row.to_terminal = conductor.terminalname2;
		row.to_terminal_uuid = conductor.terminal2_uuid;
		row.to_terminal_index = terminal_indexes.value(
			connectionKey(row.to_element_uuid, row.to_terminal_uuid));
		row.diagram_position = conductor.folio;
		result.append(row);
	}

	return result;
}
