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
#include "cablelistrows.h"

#include "../bomexport.h"
#include "../cable/cable.h"
#include "../conductorproperties.h"
#include "../diagram.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetinformation.h"
#include "../qetproject.h"

#include <QHash>
#include <QLineF>

#include <algorithm>
#include <climits>
#include <utility>

namespace {

/**
	@brief One place where the line of a cable ends.

	An end which lands on a piece carries that piece (its uuid for
	grouping and its pointer for reading the label, both valid as long
	as the plan lives); an end taken from the drawn line carries none,
	and its tie names the section instead.
*/
struct CableEnd
{
	Diagram *diagram = nullptr;
	QPointF scene;
	Element *element = nullptr;
	QUuid element_uuid;
	QString tie;

	bool isLanding() const {return !element_uuid.isNull();}
};

/**
	@brief endBefore
	The order the ends of the cables are read in: the earlier sheet
	first, then the higher point of that sheet, then the left one, then
	a stable name so that a redraw can never swap two ends and shuffle
	the rows around.
	@param a first end
	@param b second end
	@param project the project the sheet numbers come from
	@return true when a comes before b
*/
bool endBefore(const CableEnd &a, const CableEnd &b, const QETProject *project)
{
	const int folio_a = a.diagram ? project->folioIndex(a.diagram) : INT_MAX;
	const int folio_b = b.diagram ? project->folioIndex(b.diagram) : INT_MAX;
	if (folio_a != folio_b) {
		return folio_a < folio_b;
	}
	if (a.scene.y() != b.scene.y()) {
		return a.scene.y() < b.scene.y();
	}
	if (a.scene.x() != b.scene.x()) {
		return a.scene.x() < b.scene.x();
	}
	return a.tie < b.tie;
}

/**
	@brief folioOf
	@param end
	@return the sheet number the end lies on, empty when it lies on no
	sheet at all (a cable nothing was ever drawn for)
*/
QString folioOf(const CableEnd &end, const QETProject *project)
{
	return end.diagram ? QString::number(project->folioIndex(end.diagram) + 1)
					   : QString();
}

/**
	@brief plantOf
	@return the Installation the piece the end lands on carries -- the
	same field every other list reads for a piece. A piece without one
	leaves the cell empty: there is no second place to look for it, and
	an end of the drawn line, which lands on no piece at all, fills
	nothing either.
*/
QString plantOf(const CableEnd &end)
{
	if (!end.element) {
		return QString();
	}
	return end.element->elementInformations()
			.value(QETInformation::ELMT_PLANT).toString();
}

/**
	@brief locmachOf
	@return the Localisation the piece the end lands on carries, with
	the same rule as the Installation: what the piece carries or
	nothing at all.
*/
QString locmachOf(const CableEnd &end)
{
	if (!end.element) {
		return QString();
	}
	return end.element->elementInformations()
			.value(QETInformation::ELMT_LOCATION).toString();
}

/**
	@brief labelOfEnd
	@return the label (the BMK) of the piece the end lands on, empty for
	an end of the drawn line -- there is no piece there to carry one
*/
QString labelOfEnd(const CableEnd &end)
{
	if (!end.element) {
		return QString();
	}
	return end.element->elementInformations()
			.value(QETInformation::ELMT_LABEL).toString();
}

} // namespace

/**
	@brief CableList::allColumns
	@return every column of the cable list, in the order they are
	offered, drawn and exported
*/
const QList<CableList::Column> &CableList::allColumns()
{
	static const QList<Column> columns = {
		{QStringLiteral("nr"),                tr("No.")},
		{QStringLiteral("installation_cable"), tr("Plant (cable)")},
		{QStringLiteral("location_cable"),    tr("Location (cable)")},
		{QStringLiteral("bmk_cable"),         tr("Label (cable)")},
		{QStringLiteral("installation_start"), tr("Plant (start)")},
		{QStringLiteral("location_start"),    tr("Location (start)")},
		{QStringLiteral("bmk_start"),         tr("Label (start)")},
		{QStringLiteral("folio_start"),       tr("Sheet (start)")},
		{QStringLiteral("installation_end"),  tr("Plant (end)")},
		{QStringLiteral("location_end"),      tr("Location (end)")},
		{QStringLiteral("bmk_end"),           tr("Label (end)")},
		{QStringLiteral("folio_end"),         tr("Sheet (end)")},
		{QStringLiteral("type"),              tr("Cable type")},
		{QStringLiteral("length"),            tr("Length")},
		{QStringLiteral("cores_used"),        tr("Used cores")},
	};
	return columns;
}

/**
	@brief CableList::defaultKeys
	@return the columns a freshly drawn cable list starts with: all of
	them
*/
const QStringList &CableList::defaultKeys()
{
	static const QStringList keys = []() {
		QStringList list;
		for (const Column &column : allColumns()) {
			list << column.key;
		}
		return list;
	}();
	return keys;
}

/**
	@brief CableList::labelOf
	@param key
	@return the header text of that column, empty when the key names no
	column of this list
*/
QString CableList::labelOf(const QString &key)
{
	for (const Column &column : allColumns()) {
		if (column.key == key) {
			return column.label;
		}
	}
	return QString();
}

/**
	@brief CableList::rows
	Work out one row per cable, in plan order.
	@param project the project whose cables are listed
	@return the rows, sorted, the number column already filled in
*/
QVector<QMap<QString, QString>> CableList::rows(QETProject *project)
{
	QVector<QMap<QString, QString>> result;
	if (!project) {
		return result;
	}

		//Every conductor of every cable is walked once, and where one
		//lands on a real piece becomes an arrival of that cable. A core
		//which ends on a folio report is skipped on that side: the
		//cable goes on over there, and the landing on the far sheet is
		//what counts as its end.
	QHash<QUuid, QVector<CableEnd>> arrivals;
	for (Diagram *diagram : project->diagrams())
	{
		for (Conductor *conductor : diagram->conductors())
		{
			const ConductorProperties properties = conductor->properties();
			if (properties.m_cable_uuid.isNull()) {
				continue;
			}
			if (!conductor->terminal1 || !conductor->terminal2) {
				continue;
			}

			Terminal *terminals[2] = {conductor->terminal1, conductor->terminal2};
			for (Terminal *terminal : terminals)
			{
				Element *element = terminal->parentElement();
				if (!element) {
						//a free end is not an arrival
					continue;
				}
				if (element->linkType() & Element::AllReport) {
						//the cable continues on the linked sheet
					continue;
				}

				CableEnd end;
				end.diagram = element->diagram();
				end.scene = terminal->scenePos();
				end.element = element;
				end.element_uuid = element->uuid();
				end.tie = conductor->uuid().toString();
				arrivals[properties.m_cable_uuid].append(end);
			}
		}
	}

	struct Entry
	{
		Cable *cable = nullptr;
		CableEnd start;
		CableEnd target;
	};

	auto byPlan = [project](const CableEnd &a, const CableEnd &b) {
		return endBefore(a, b, project);
	};

	QVector<Entry> entries;
	entries.reserve(project->cables().count());

	for (Cable *cable : project->cables())
	{
			//Several cores landing on one piece are one end: the highest
			//of their landings speaks for it.
		QHash<QUuid, CableEnd> grouped;
		for (const CableEnd &end : arrivals.value(cable->uuid()))
		{
			auto it = grouped.find(end.element_uuid);
			if (it == grouped.end() || byPlan(end, it.value())) {
				grouped.insert(end.element_uuid, end);
			}
		}
		QList<CableEnd> landings = grouped.values();
		std::sort(landings.begin(), landings.end(), byPlan);

			//The free ends of the drawn line, for the sides no core ever
			//landed on
		QList<CableEnd> line_ends;
		for (const CablePartData &part : cable->parts())
		{
			Diagram *diagram = project->diagramByUuid(part.diagram);
			if (!diagram) {
				continue;
			}
			for (int i = 0; i < 2; ++i)
			{
				CableEnd end;
				end.diagram = diagram;
				end.scene = i ? part.p2 : part.p1;
				end.tie = part.uuid.toString() + (i ? QStringLiteral("2")
													: QStringLiteral("1"));
				line_ends.append(end);
			}
		}
		std::sort(line_ends.begin(), line_ends.end(), byPlan);

		Entry entry;
		entry.cable = cable;

		if (landings.size() >= 2)
		{
				//The rule as asked for: the first finding counts for
				//both ends. First landing in plan order is the start,
				//the very next finding is the target -- not the last
				//one. A sheet further along finds the same cable again
				//but start and target are already taken by then, so it
				//adds nothing: on the first sheet, the upper piece is
				//the start and the lower one the target.
			entry.start = landings.first();
			entry.target = landings.at(1);
		}
		else if (landings.size() == 1)
		{
				//One side is wired: that side carries the label, the
				//other side falls back to the drawn line, farthest end
				//first so a tie goes to the later point of the plan.
			entry.start = landings.first();
			entry.target = landings.first();
			qreal farthest = -1.0;
			for (const CableEnd &candidate : std::as_const(line_ends))
			{
				const qreal distance = QLineF(entry.start.scene, candidate.scene).length();
				if (distance >= farthest) {
					farthest = distance;
					entry.target = candidate;
				}
			}
		}
		else if (line_ends.size() >= 2)
		{
			entry.start = line_ends.first();
			entry.target = line_ends.last();
		}
		else if (line_ends.size() == 1)
		{
			entry.start = line_ends.first();
			entry.target = line_ends.first();
		}
			//A cable with neither a core nor a line draws no ends at all;
			//its own fields still fill the row.

		entries.append(entry);
	}

		//Plan order again for the rows themselves: by the sheet the start
		//lies on, then from the top of that sheet down. The name and the
		//uuid only ever break ties, so the numbering is stable between
		//two openings of the same project.
	std::sort(entries.begin(), entries.end(), [project](const Entry &a, const Entry &b) {
		if (endBefore(a.start, b.start, project)) return true;
		if (endBefore(b.start, a.start, project)) return false;
		if (endBefore(a.target, b.target, project)) return true;
		if (endBefore(b.target, a.target, project)) return false;
		if (a.cable->designation() != b.cable->designation()) {
			return a.cable->designation() < b.cable->designation();
		}
		return a.cable->uuid().toString() < b.cable->uuid().toString();
	});

	int number = 0;
	for (const Entry &entry : std::as_const(entries))
	{
		Cable *cable = entry.cable;
		QMap<QString, QString> row;
		row.insert(QStringLiteral("nr"), QString::number(++number));
		row.insert(QStringLiteral("installation_cable"), cable->installation());
		row.insert(QStringLiteral("location_cable"), cable->location());
		row.insert(QStringLiteral("bmk_cable"), cable->designation());
		row.insert(QStringLiteral("installation_start"), plantOf(entry.start));
		row.insert(QStringLiteral("location_start"), locmachOf(entry.start));
		row.insert(QStringLiteral("bmk_start"), labelOfEnd(entry.start));
		row.insert(QStringLiteral("folio_start"), folioOf(entry.start, project));
		row.insert(QStringLiteral("installation_end"), plantOf(entry.target));
		row.insert(QStringLiteral("location_end"), locmachOf(entry.target));
		row.insert(QStringLiteral("bmk_end"), labelOfEnd(entry.target));
		row.insert(QStringLiteral("folio_end"), folioOf(entry.target, project));
		row.insert(QStringLiteral("type"), cable->type());
		row.insert(QStringLiteral("length"), cable->length());
		row.insert(QStringLiteral("cores_used"),
				   QStringLiteral("%1/%2")
						   .arg(cable->usedCoreCount())
						   .arg(cable->coreCount()));
		result.append(row);
	}

	return result;
}

/**
	@brief CableList::toCsv
	The whole list as CSV bytes: every column of the list, every cable
	of the project, in the format every CSV export of this program
	writes.
	@param project the project whose cables are listed
	@return the bytes to write
*/
/**
	@brief CableList::toCsv
	@param project the project whose cables are listed
	@param keys the columns to write, in that order; unknown keys are
	dropped, and a list without a single usable one falls back to the
	default columns rather than writing a blank file
	@param include_headers whether the line of column names is written
	as the first line
	@return the CSV bytes in the format every CSV export of this
	program writes (UTF-8 BOM, ';', every field quoted)
*/
QByteArray CableList::toCsv(QETProject *project, const QStringList &keys,
							bool include_headers)
{
	QStringList kept;
	for (const QString &key : keys)
	{
		if (kept.contains(key) || labelOf(key).isEmpty()) {
			continue;
		}
		kept << key;
	}
	if (kept.isEmpty()) {
		kept = defaultKeys();
	}

	QByteArray csv("\xEF\xBB\xBF");
	if (include_headers) {
		QStringList headers;
		for (const QString &key : std::as_const(kept)) {
			headers << labelOf(key);
		}
		csv += BomExport::csvRecord(headers);
	}

	const QVector<QMap<QString, QString>> rows_ = rows(project);
	for (const QMap<QString, QString> &row : rows_)
	{
		QStringList values;
		for (const QString &key : std::as_const(kept)) {
			values << row.value(key);
		}
		csv += BomExport::csvRecord(values);
	}

	return csv;
}

/**
	@brief CableList::toCsv
	The whole list as it has always been written: every column, every
	cable, the line of column names included
	@param project the project whose cables are listed
	@return the CSV bytes
*/
QByteArray CableList::toCsv(QETProject *project)
{
	return toCsv(project, defaultKeys(), true);
}
