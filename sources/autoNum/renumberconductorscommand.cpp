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
#include "renumberconductorscommand.h"

#include <QCoreApplication>
#include <QSet>
#include <algorithm>

#include <QSqlQuery>

#include <autoNum/numerotationcontextcommands.h>
#include <dataBase/projectdatabase.h>
#include <diagram.h>
#include <qetgraphicsitem/conductor.h>
#include <qetgraphicsitem/terminal.h>
#include <qetproject.h>

namespace {

	//Where a wire starts on its folio: the upper, then the left, of its ends
QPointF startOf(const Conductor *wire)
{
	QPointF a = wire->terminal1 ? wire->terminal1->dockConductor() : QPointF();
	QPointF b = wire->terminal2 ? wire->terminal2->dockConductor() : QPointF();
	return (a.y() < b.y() || (a.y() == b.y() && a.x() <= b.x())) ? a : b;
}

bool readsBefore(const Conductor *a, const Conductor *b)
{
	const QPointF pa = startOf(a), pb = startOf(b);
	if (pa.y() != pb.y()) return pa.y() < pb.y();
	return pa.x() < pb.x();
}

bool hasPlantCounter(const NumerotationContext &nc)
{
	for (int i = 0 ; i < nc.size() ; ++i)
		if (nc.itemAt(i).value(0) == QLatin1String("unitplant"))
			return true;
	return false;
}

}

/**
	@brief RenumberConductorsCommand::plan
	@return what renumbering the wires of @p folios with the conductor
	numbering @p scheme of @p project would do, nothing changed yet. Empty
	when the numbering does not exist.
*/
RenumberConductorsCommand::Plan RenumberConductorsCommand::plan(
		QETProject *project, const QString &scheme, const QList<Diagram *> &folios)
{
	Plan plan;
	if (!project) {
		return plan;
	}
	plan.old_context = project->conductorAutoNum(scheme);
	if (plan.old_context.isEmpty()) {
		return plan;
	}

		//The chosen folios, in the project's order, built
	const QSet<Diagram *> chosen(folios.begin(), folios.end());
	QList<Diagram *> order;
	for (Diagram *d : project->diagrams())
		if (chosen.contains(d))
			order << d;
	project->buildFolios(order);

		//The potentials, each from its first wire: folio order, then from
		//the top left of each folio
	struct Potential {
		QList<Conductor *> wires;
		Diagram *folio = nullptr;
	};
	QVector<Potential> potentials;
	QSet<Conductor *> seen;
	for (Diagram *d : std::as_const(order)) {
		QList<Conductor *> wires = d->conductors();
		std::sort(wires.begin(), wires.end(), readsBefore);
		for (Conductor *wire : std::as_const(wires)) {
			if (seen.contains(wire)) continue;
			QSet<Conductor *> group = wire->relatedPotentialConductors(true);
			group.insert(wire);
			seen.unite(group);
			QList<Conductor *> members(group.begin(), group.end());
			std::sort(members.begin(), members.end(), [](Conductor *a, Conductor *b) {
				if (a->diagram() != b->diagram())
					return a->diagram()->folioIndex() < b->diagram()->folioIndex();
				return readsBefore(a, b);
			});
			potentials.append({members, d});
		}
	}
	plan.potentials = potentials.size();

		//A plant counter numbers each plant apart: numbers clash only there
	const bool per_plant = hasPlantCounter(plan.old_context);
	const auto key = [per_plant](Diagram *folio, const QString &text) {
		return per_plant ? folio->border_and_titleblock.plant() + QChar(0x1f) + text : text;
	};

		//Kept as they are: a frozen wire, or a folio not chosen
	QSet<QString> reserved;
	QVector<bool> keep(potentials.size(), false);
	for (int i = 0 ; i < potentials.size() ; ++i) {
		const Potential &p = potentials.at(i);
		QString reason;
		for (Conductor *wire : p.wires) {
			if (wire->isFreezeLabel()) {
				reason = QCoreApplication::translate("RenumberConductorsCommand", "frozen");
				break;
			}
			if (!chosen.contains(wire->diagram())) {
				reason = QCoreApplication::translate(
						"RenumberConductorsCommand", "reaches a folio not chosen");
				break;
			}
		}
		if (reason.isEmpty()) continue;
		keep[i] = true;
		const QString text = p.wires.first()->properties().text;
		plan.kept.append({p.wires.first(), text, reason});
		if (!text.isEmpty())
			reserved.insert(key(p.folio, text));
	}

		//The numbers of the folios not chosen, which stay as they are: read
		//from the project database, so those folios need not be built
	if (projectDataBase *db = project->dataBase()) {
		db->updateDB();
		QSet<QString> chosen_uuids;
		for (Diagram *d : std::as_const(order))
			chosen_uuids.insert(d->uuid().toString());
		QString error;
		QSqlQuery query = db->newQuery(QStringLiteral(
				"SELECT c.diagram_uuid, c.text, di.plant FROM conductor c "
				"LEFT JOIN diagram_info di ON di.diagram_uuid = c.diagram_uuid "
				"WHERE c.text <> ''"), &error);
		while (error.isEmpty() && query.next()) {
			if (chosen_uuids.contains(query.value(0).toString())) continue;
			const QString text = query.value(1).toString();
			reserved.insert(per_plant ? query.value(2).toString() + QChar(0x1f) + text : text);
		}
	}

		//The rest, numbered from the start
	NumerotationContext context = autonum::resetContextCounters(plan.old_context);
	QHash<QString, QStringList> plant_counts;
	Diagram *folio = nullptr;
	for (int i = 0 ; i < potentials.size() ; ++i) {
		if (keep.at(i)) continue;
		const Potential &p = potentials.at(i);
		if (p.folio != folio) {
			folio = p.folio;
			context = autonum::resetFolioCounters(context);
		}
		const QString plant = folio->border_and_titleblock.plant();
		QString text, formula;
		autonum::sequentialNumbers sequence;
			//A number a kept potential has is skipped; bounded, should the
			//rule give the same text again and again (only strings)
		for (int attempt = 0 ; attempt < 100000 ; ++attempt) {
			const NumerotationContext used =
					autonum::plantCounters(context, plant_counts.value(plant));
			formula = autonum::numerotationContextToFormula(used);
			sequence = autonum::sequenceOf(used);
			text = autonum::AssignVariables::formulaToLabel(
					formula, sequence, folio, nullptr, p.wires.first());
			NumerotationContextCommands next(used, folio);
			context = next.next();
			if (per_plant)
				plant_counts.insert(plant, autonum::plantCounterValues(used));
			if (!reserved.contains(key(folio, text)))
				break;
		}
		reserved.insert(key(folio, text));
		for (Conductor *wire : p.wires) {
			Change change;
			change.wire = wire;
			change.old_properties = wire->properties();
			change.new_properties = change.old_properties;
			change.new_properties.text = text;
			change.new_properties.m_formula = formula;
			change.old_sequence = wire->sequenceNum();
			change.new_sequence = sequence;
			plan.changes.append(change);
		}
	}
	plan.new_context = context;
	for (auto it = plant_counts.constBegin() ; it != plant_counts.constEnd() ; ++it) {
		plan.old_plant_counters.insert(it.key(), project->conductorPlantCounter(scheme, it.key()));
		plan.new_plant_counters.insert(it.key(), it.value());
	}
	return plan;
}

RenumberConductorsCommand::RenumberConductorsCommand(
		QETProject *project, const QString &scheme, Plan plan,
		const QString &text, QUndoCommand *parent) :
	QUndoCommand(text, parent),
	m_project(project),
	m_scheme(scheme),
	m_plan(std::move(plan))
{}

void RenumberConductorsCommand::undo()
{
	apply(false);
}

void RenumberConductorsCommand::redo()
{
	apply(true);
}

void RenumberConductorsCommand::apply(bool use_new)
{
	if (!m_project) {
		return;
	}
	m_project->addConductorAutoNum(m_scheme, use_new ? m_plan.new_context
													 : m_plan.old_context);
	const auto &counts = use_new ? m_plan.new_plant_counters : m_plan.old_plant_counters;
	for (auto it = counts.constBegin() ; it != counts.constEnd() ; ++it)
		m_project->setConductorPlantCounter(m_scheme, it.key(), it.value());
	for (const Change &change : std::as_const(m_plan.changes)) {
		if (!change.wire) continue;
		change.wire->replaceSequenceNum(use_new ? change.new_sequence : change.old_sequence);
		change.wire->setProperties(use_new ? change.new_properties : change.old_properties);
	}
}
