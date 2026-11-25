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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "openwireends.h"

#include <dataBase/projectdatabase.h>
#include <properties/elementdata.h>

#include <QHash>
#include <QSet>
#include <QSqlQuery>

#include <algorithm>

namespace {

struct ElementRow
{
	ElementData::Type kind = ElementData::Simple;
	QString folio;
	QString position;
	QString label;
	int wireEnds = 0;
	bool linked = false;
};

struct FolioRow
{
	int number = 0;
	QString title;
};

using TerminalKey = std::pair<QString, QString>;   //element uuid, terminal uuid

QUuid firstWire(const QList<QUuid> &wires)
{
	return wires.isEmpty() ? QUuid() : *std::min_element(wires.begin(), wires.end());
}

} // namespace

int OpenWireEnds::compareCells(const QString &a, const QString &b)
{
	const auto split = [](const QString &cell) {
		int digits = cell.size();
		while (digits > 0 && cell.at(digits - 1).isDigit())
			--digits;
		const qlonglong column = digits < cell.size() ? QStringView(cell).mid(digits).toLongLong() : -1;
		return std::make_pair(cell.left(digits), column);
	};
	const auto [row_a, column_a] = split(a);
	const auto [row_b, column_b] = split(b);
	if (row_a.size() != row_b.size())   //Z before AA
		return row_a.size() < row_b.size() ? -1 : 1;
	if (const int order = row_a.compare(row_b))
		return order;
	if (column_a != column_b)
		return column_a < column_b ? -1 : 1;
	return 0;
}

QList<OpenWireEnds::Finding> OpenWireEnds::check(const QSqlDatabase &db)
{
	QHash<QString, FolioRow> folios;
	{
		QSqlQuery query(db);
		query.exec(QStringLiteral("SELECT d.uuid, d.pos, di.title FROM diagram d "
								  "LEFT JOIN diagram_info di ON di.diagram_uuid = d.uuid"));
		while (query.next())
			folios.insert(query.value(0).toString(),
						  {query.value(1).toInt(), query.value(2).toString()});
	}

	QHash<QString, ElementRow> elements;
	{
		QSqlQuery query(db);
		query.exec(QStringLiteral("SELECT e.uuid, e.type, e.diagram_uuid, e.pos, ei.label FROM element e "
								  "LEFT JOIN element_info ei ON ei.element_uuid = e.uuid"));
		while (query.next()) {
			ElementRow row;
			row.kind = ElementData::typeFromString(query.value(1).toString());
			row.folio = query.value(2).toString();
			row.position = query.value(3).toString();
			row.label = query.value(4).toString();
			elements.insert(query.value(0).toString(), row);
		}
	}
	{
		QSqlQuery query(db);
		query.exec(QStringLiteral("SELECT DISTINCT element_uuid FROM link"));
		while (query.next()) {
			const auto row = elements.find(query.value(0).toString());
			if (row != elements.end())
				row->linked = true;
		}
	}

	struct WireRow { QString uuid, folio; TerminalKey end1, end2; };
	QList<WireRow> wires;
	{
		QSqlQuery query(db);
		query.exec(QStringLiteral("SELECT uuid, diagram_uuid, terminal1_element_uuid, terminal1_uuid, "
								  "terminal2_element_uuid, terminal2_uuid FROM conductor"));
		while (query.next()) {
			WireRow wire{query.value(0).toString(), query.value(1).toString(),
						 {query.value(2).toString(), query.value(3).toString()},
						 {query.value(4).toString(), query.value(5).toString()}};
			for (const TerminalKey &end : {wire.end1, wire.end2}) {
				const auto row = elements.find(end.first);
				if (row != elements.end())
					++row->wireEnds;
			}
			wires << wire;
		}
	}

	const auto finding = [&](Kind kind, const QString &element) {
		Finding f;
		f.kind = kind;
		f.element = QUuid(element);
		const ElementRow row = elements.value(element);
		f.label = row.label;
		f.position = row.position;
		f.folio = QUuid(row.folio);
		return f;
	};

		//A report or a junction which ends a wire, and the wires of each
	QList<Finding> found;
	QSet<QString> flagged;
	for (auto it = elements.constBegin() ; it != elements.constEnd() ; ++it) {
		if (!it->wireEnds)
			continue;
		if ((it->kind & ElementData::AllReport) && !it->linked) {
			found << finding(Kind::UnlinkedReport, it.key());
			flagged.insert(it.key());
		} else if ((it->kind & ElementData::Junction) && it->wireEnds == 1) {
			found << finding(Kind::LoneJunction, it.key());
			flagged.insert(it.key());
		}
	}

		//The potentials, and on each the symbol terminals it reaches: a
		//terminal of anything but a report or a junction, which only pass
		//the potential on
	const projectDataBase::PotentialGroups groups = projectDataBase::potentialGroupsOf(db);
	struct Potential {
		QSet<TerminalKey> ends;
		QList<QUuid> wires;
		QList<QUuid> folios;
		bool flagged = false;
	};
	QHash<int, Potential> potentials;
	QHash<QString, int> group_of_element;   //a flagged report or junction -> its potential
	for (const WireRow &wire : std::as_const(wires)) {
		const auto group = groups.group_of.constFind(QUuid(wire.uuid));
		if (group == groups.group_of.constEnd())
			continue;
		Potential &potential = potentials[group.value()];
		potential.wires << QUuid(wire.uuid);
		const QUuid folio(wire.folio);
		if (!potential.folios.contains(folio))
			potential.folios << folio;
		for (const TerminalKey &end : {wire.end1, wire.end2}) {
			if (flagged.contains(end.first)) {
				potential.flagged = true;
				group_of_element.insert(end.first, group.value());
				continue;
			}
			const ElementData::Type kind = elements.value(end.first).kind;
			if (!(kind & ElementData::AllReport) && !(kind & ElementData::Junction))
				potential.ends.insert(end);
		}
	}

		//The report's or junction's wires are those of its potential
	for (Finding &f : found) {
		const auto group = group_of_element.constFind(f.element.toString());
		if (group != group_of_element.constEnd()) {
			f.wires = potentials.value(group.value()).wires;
			f.folios = potentials.value(group.value()).folios;
		}
	}

	QSqlQuery terminal_name(db);
	terminal_name.prepare(QStringLiteral("SELECT name FROM terminal WHERE uuid = :uuid AND element_uuid = :element"));
	for (auto it = potentials.constBegin() ; it != potentials.constEnd() ; ++it) {
		if (it->flagged || it->ends.size() > 1)
			continue;
		Finding f;
		if (it->ends.isEmpty()) {
				//No symbol to show: the potential's first folio
			f.folio = it->folios.value(0);
			for (const QUuid &folio : it->folios)
				if (folios.value(folio.toString()).number < folios.value(f.folio.toString()).number)
					f.folio = folio;
		} else {
			const TerminalKey end = *it->ends.constBegin();
			f = finding(Kind::OneEnd, end.first);
			terminal_name.bindValue(QStringLiteral(":uuid"), end.second);
			terminal_name.bindValue(QStringLiteral(":element"), end.first);
			if (terminal_name.exec() && terminal_name.next())
				f.terminal = terminal_name.value(0).toString();
		}
		f.kind = Kind::OneEnd;
		f.wires = it->wires;
		f.folios = it->folios;
		found << f;
	}

	for (Finding &f : found) {
		const FolioRow folio = folios.value(f.folio.toString());
		f.folioNumber = folio.number;
		f.folioTitle = folio.title;
	}
		//Cells as row then column, so A9 comes before A10; the rest only
		//so the rows come in the same order on every run
	std::sort(found.begin(), found.end(), [](const Finding &a, const Finding &b) {
		if (a.folioNumber != b.folioNumber)
			return a.folioNumber < b.folioNumber;
		if (const int order = compareCells(a.position, b.position))
			return order < 0;
		if (a.kind != b.kind)
			return a.kind < b.kind;
		if (a.element != b.element)
			return a.element < b.element;
		return firstWire(a.wires) < firstWire(b.wires);
	});
	return found;
}
