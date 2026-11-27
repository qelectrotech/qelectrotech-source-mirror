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
#include "terminalbranches.h"

#include "openwireends.h"

#include <properties/elementdata.h>
#include <qetgraphicsitem/terminal.h>

#include <QDomDocument>
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
	QString definition;
};

using TerminalKey = std::pair<QString, QString>;   //element uuid, terminal uuid

} // namespace

QList<TerminalBranches::Finding> TerminalBranches::check(const QSqlDatabase &db)
{
	QHash<QString, std::pair<int, QString>> folios;   //number, title
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
		query.exec(QStringLiteral("SELECT e.uuid, e.type, e.diagram_uuid, e.pos, ei.label, e.definition "
								  "FROM element e LEFT JOIN element_info ei ON ei.element_uuid = e.uuid"));
		while (query.next())
			elements.insert(query.value(0).toString(),
							{ElementData::typeFromString(query.value(1).toString()),
							 query.value(2).toString(), query.value(3).toString(),
							 query.value(4).toString(), query.value(5).toString()});
	}
	const auto kindOf = [&](const QString &element) {
		const auto row = elements.constFind(element);
		return row == elements.constEnd() ? ElementData::Simple : row->kind;
	};

	struct WireRow { QString uuid, folio; TerminalKey end1, end2; };
	QList<WireRow> wires;
	QHash<QString, int> junction_wires;   //wire ends on each junction
	QHash<QString, QHash<QUuid, int>> connection_wires;   //and on each of its connections
	{
		QSqlQuery query(db);
		query.exec(QStringLiteral("SELECT uuid, diagram_uuid, terminal1_element_uuid, terminal1_uuid, "
								  "terminal2_element_uuid, terminal2_uuid FROM conductor"));
		while (query.next()) {
			const WireRow wire{query.value(0).toString(), query.value(1).toString(),
							   {query.value(2).toString(), query.value(3).toString()},
							   {query.value(4).toString(), query.value(5).toString()}};
			for (const TerminalKey &end : {wire.end1, wire.end2})
				if (kindOf(end.first) & ElementData::Junction) {
					++junction_wires[end.first];
					++connection_wires[end.first][QUuid(end.second)];
				}
			wires << wire;
		}
	}

		//A junction whose definition gives its chain (#1500): the wire comes
		//in at the source and goes on to the targets in turn, so the
		//wiring list runs it from terminal to terminal and nothing is
		//split. It holds only while each of its wires is on a connection
		//of the chain, one wire each; else the list cannot say which way
		//a wire goes. The definition is the project's own copy, in
		//symbol_def, so a .qetdb read by itself answers alike
	QHash<QString, JunctionRoles> roles_of;   //by definition
	QSqlQuery symbol(db);
	symbol.prepare(QStringLiteral("SELECT content FROM symbol_def WHERE path = :path"));
	const auto followsChain = [&](const QString &junction) {
		const QString definition = elements.value(junction).definition;
		auto roles = roles_of.constFind(definition);
		if (roles == roles_of.constEnd()) {
			JunctionRoles read;
			const QString embedded = QStringLiteral("embed://");
			if (definition.startsWith(embedded)) {
				symbol.bindValue(QStringLiteral(":path"), definition.mid(embedded.size()));
				QDomDocument document;
				if (symbol.exec() && symbol.next()
					&& document.setContent(symbol.value(0).toString())) {
					QDomElement root = document.documentElement();
					if (root.tagName() != QLatin1String("definition"))
						root = root.firstChildElement(QStringLiteral("definition"));
					read = junctionRolesOf(root);
				}
			}
			roles = roles_of.insert(definition, read);
		}
		if (roles->source.isEmpty())
			return false;
		QList<QUuid> chain{QUuid(roles->source)};
		for (const QString &target : roles->targets)
			chain << QUuid(target);
		const QHash<QUuid, int> wired = connection_wires.value(junction);
		for (auto it = wired.constBegin() ; it != wired.constEnd() ; ++it)
			if (it.value() > 1 || !chain.contains(it.key()))
				return false;
		return true;
	};
	QSet<QString> chained;
	for (auto it = junction_wires.constBegin() ; it != junction_wires.constEnd() ; ++it)
		if (followsChain(it.key()))
			chained.insert(it.key());

		//What only passes a wire on: a folio report, a junction with no
		//chain. A chained junction is where its wires land, as a terminal
	const auto passesOn = [&](const QString &element) {
		const ElementData::Type kind = kindOf(element);
		return (kind & ElementData::AllReport)
			|| ((kind & ElementData::Junction) && !chained.contains(element));
	};

		//The runs: what passes a wire on, joined by the wires between, and
		//a report to the first symbol it is linked to, as
		//projectDataBase::potentialGroupsOf() follows a potential
	QHash<QString, QString> parent;
	const auto find = [&](QString x) {
		for (auto up = parent.constFind(x) ; up != parent.constEnd() ; up = parent.constFind(x))
			x = up.value();
		return x;
	};
	const auto join = [&](const QString &a, const QString &b) {
		const QString ra = find(a), rb = find(b);
		if (ra != rb) parent[rb] = ra;
	};
	for (const WireRow &wire : std::as_const(wires))
		if (passesOn(wire.end1.first) && passesOn(wire.end2.first))
			join(wire.end1.first, wire.end2.first);
	{
		QSqlQuery query(db);
		query.exec(QStringLiteral("SELECT element_uuid, linked_uuid FROM link ORDER BY element_uuid, ord, rowid"));
		QSet<QString> seen;
		while (query.next()) {
			const QString element = query.value(0).toString();
			if (seen.contains(element)) continue;
			seen.insert(element);
			const QString partner = query.value(1).toString();
			if ((kindOf(element) & ElementData::AllReport) && passesOn(partner))
				join(element, partner);
		}
	}

	struct Run {
		QSet<TerminalKey> ends;
		QList<QUuid> wires;
		QList<QUuid> folios;
		QSet<QString> junctions;   ///< the junctions it is split at: three wires or more
	};
	QHash<QString, Run> runs;
	for (const WireRow &wire : std::as_const(wires)) {
		const bool pass1 = passesOn(wire.end1.first);
		if (!pass1 && !passesOn(wire.end2.first))
			continue;   //from terminal to terminal: in no run
		Run &run = runs[find(pass1 ? wire.end1.first : wire.end2.first)];
		run.wires << QUuid(wire.uuid);
		const QUuid folio(wire.folio);
		if (!run.folios.contains(folio))
			run.folios << folio;
		for (const TerminalKey &end : {wire.end1, wire.end2}) {
			if (!passesOn(end.first))
				run.ends.insert(end);
			else if ((kindOf(end.first) & ElementData::Junction)
					 && junction_wires.value(end.first) >= 3)
				run.junctions.insert(end.first);
		}
	}

		//A strip terminal is named as the strip draws it: by its own
		//label, else by its strip and its place on it, "X1:7"
		//(RealTerminal's endDesignation()). qet_terminals counts from 0
	QSqlQuery strip_place(db);
	strip_place.prepare(QStringLiteral("SELECT strip, position FROM qet_terminals "
									   "WHERE element_uuid = :element AND COALESCE(strip, '') <> ''"));
	QSqlQuery terminal_name(db);
	terminal_name.prepare(QStringLiteral("SELECT name FROM terminal WHERE uuid = :uuid AND element_uuid = :element"));
	const auto place = [&](const QString &element, QUuid &folio, int &number, QString &position, QString &label) {
		const ElementRow row = elements.value(element);
		folio = QUuid(row.folio);
		number = folios.value(row.folio).first;
		position = row.position;
		label = row.label;
	};
	const auto before = [](int number_a, const QString &cell_a, const QUuid &a,
						   int number_b, const QString &cell_b, const QUuid &b) {
		if (number_a != number_b)
			return number_a < number_b;
		if (const int order = OpenWireEnds::compareCells(cell_a, cell_b))
			return order < 0;
		return a < b;
	};

	QList<Finding> found;
	for (const Run &run : std::as_const(runs)) {
		if (run.junctions.isEmpty())
			continue;
		QList<End> strip;
		QSet<QString> strip_elements;
		int others = 0;
			//In a fixed order, so a strip terminal reached on two of its
			//terminals is named by the same one on every run
		QList<TerminalKey> ends(run.ends.begin(), run.ends.end());
		std::sort(ends.begin(), ends.end());
		for (const TerminalKey &end : std::as_const(ends)) {
			if (!(kindOf(end.first) & ElementData::Terminal)) {
				++others;
				continue;
			}
			if (strip_elements.contains(end.first))
				continue;
			strip_elements.insert(end.first);
			End e;
			e.element = QUuid(end.first);
			place(end.first, e.folio, e.folioNumber, e.position, e.label);
			if (e.label.isEmpty()) {
				strip_place.bindValue(QStringLiteral(":element"), end.first);
				if (strip_place.exec() && strip_place.next())
					e.label = QStringLiteral("%1:%2").arg(strip_place.value(0).toString())
							  .arg(strip_place.value(1).toInt() + 1);
			}
			terminal_name.bindValue(QStringLiteral(":uuid"), end.second);
			terminal_name.bindValue(QStringLiteral(":element"), end.first);
			if (terminal_name.exec() && terminal_name.next())
				e.terminal = terminal_name.value(0).toString();
			strip << e;
		}
		if (strip_elements.size() < 2)
			continue;
		std::sort(strip.begin(), strip.end(), [&](const End &a, const End &b) {
			return before(a.folioNumber, a.position, a.element,
						  b.folioNumber, b.position, b.element);
		});

		Finding f;
		f.terminals = strip;
		f.others = others;
		f.wires = run.wires;
		std::sort(f.wires.begin(), f.wires.end());
		f.folios = run.folios;
			//The junction the run is split at: the one with the most wires,
			//the first in folio order of those
		QString best;
		for (const QString &junction : run.junctions) {
			Finding j;
			place(junction, j.folio, j.folioNumber, j.position, j.label);
			j.junction = QUuid(junction);
			const int mine = junction_wires.value(junction);
			const int most = junction_wires.value(best);
			if (best.isEmpty() || mine > most
				|| (mine == most && before(j.folioNumber, j.position, j.junction,
										   f.folioNumber, f.position, f.junction))) {
				best = junction;
				f.junction = j.junction;
				f.folio = j.folio;
				f.folioNumber = j.folioNumber;
				f.position = j.position;
				f.label = j.label;
			}
		}
		f.folioTitle = folios.value(f.folio.toString()).second;
		found << f;
	}

	std::sort(found.begin(), found.end(), [&](const Finding &a, const Finding &b) {
		return before(a.folioNumber, a.position, a.junction,
					  b.folioNumber, b.position, b.junction);
	});
	return found;
}
