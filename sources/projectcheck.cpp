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
#include "projectcheck.h"

#include "dataBase/projectdatabase.h"
#include "devicetype/devicetypecheck.h"
#include "diagram.h"
#include "duplicatelabels.h"
#include "editor/terminalnamecheck.h"
#include "ElementsCollection/elementslocation.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "utils/qetsettings.h"
#include "wiringrules.h"

#include <QHash>
#include <QSqlQuery>

namespace {

	/// "3 — Power": the folio's number and title
	QString folioText(QETProject *project, Diagram *diagram)
	{
		if (!project || !diagram) return QString();
		return QStringLiteral("%1 — %2").arg(project->folioIndex(diagram) + 1)
										 .arg(diagram->title());
	}

	/// A place for a built item: its folio and the cell its middle is in
	ProjectCheck::Place itemPlace(QETProject *project, QGraphicsObject *item,
								  const QString &text)
	{
		ProjectCheck::Place place;
		place.text = text;
		place.item = item;
		if (Diagram *diagram = DuplicateLabels::diagramOf(item)) {
			place.folio = folioText(project, diagram);
			place.cell = diagram->convertPosition(
							 item->sceneBoundingRect().center()).toString();
		}
		return place;
	}

	/// A place for a symbol read from the database, built or not
	ProjectCheck::Place symbolPlace(QETProject *project, const QUuid &element,
									const QUuid &diagram, const QString &cell,
									const QString &text)
	{
		ProjectCheck::Place place;
		place.text = text;
		place.element = element;
		place.diagram = diagram;
		place.folio = folioText(project, project->diagramByUuid(diagram));
		place.cell = cell;
		return place;
	}

	/// A line of detail, which shows nothing when opened
	ProjectCheck::Place detail(const QString &text)
	{
		ProjectCheck::Place place;
		place.text = text;
		return place;
	}
}

QList<ProjectCheck::Kind> ProjectCheck::kinds()
{
	return {Kind::DuplicateLabels, Kind::PotentialTypes, Kind::DeviceTypes,
			Kind::WiresPerTerminal, Kind::TerminalNames};
}

QString ProjectCheck::title(Kind kind)
{
	switch (kind) {
		case Kind::DuplicateLabels: return tr("Duplicate signals and components");
		case Kind::PotentialTypes: return tr("Potential types");
		case Kind::DeviceTypes: return tr("Device types");
		case Kind::WiresPerTerminal: return tr("Wires per terminal");
		case Kind::TerminalNames: return tr("Terminal names");
	}
	return QString();
}

QString ProjectCheck::toolTip(Kind kind)
{
	switch (kind) {
		case Kind::DuplicateLabels:
			return tr("Project > Find duplicate signals and components... has the options of this check.");
		case Kind::PotentialTypes:
			return tr("Wires of different potential types joined on one potential. "
					  "Project > Check potential types... lists them too.");
		case Kind::DeviceTypes:
			return tr("Devices not drawn the way their device type says. "
					  "Project > Check device types... can place the missing parts.");
		case Kind::WiresPerTerminal:
			return tr("Terminals with more wires than the limit set in "
					  "Project properties > Wiring.");
		case Kind::TerminalNames:
			return tr("Symbols of the project with two terminals of one name, "
					  "which a wiring list cannot tell apart.");
	}
	return QString();
}

bool ProjectCheck::needsFolios(Kind kind)
{
	return kind == Kind::DuplicateLabels || kind == Kind::DeviceTypes;
}

ProjectCheck::Result ProjectCheck::run(QETProject *project, Kind kind)
{
	if (!project) {
		Result result;
		result.kind = kind;
		return result;
	}
	switch (kind) {
		case Kind::DuplicateLabels: return duplicateLabels(project);
		case Kind::PotentialTypes: return potentialTypes(project);
		case Kind::DeviceTypes: return deviceTypes(project);
		case Kind::WiresPerTerminal: return wiresPerTerminal(project);
		case Kind::TerminalNames: return terminalNames(project);
	}
	return Result();
}

/**
	@brief ProjectCheck::duplicateLabels
	DuplicateLabels::find() with the options last chosen in its window.
*/
ProjectCheck::Result ProjectCheck::duplicateLabels(QETProject *project)
{
	Result result;
	result.kind = Kind::DuplicateLabels;
	const auto groups = DuplicateLabels::find(
		project,
		QetSettings::duplicateLabelsCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive,
		QetSettings::duplicateLabelsByDesignation()
			? DuplicateLabels::ComponentRule::SameDesignation
			: DuplicateLabels::ComponentRule::LinkedOnly,
		QetSettings::duplicateLabelsSignalsByPlant()
			? DuplicateLabels::SignalScope::Plant
			: DuplicateLabels::SignalScope::Project);
	for (const auto &group : groups) {
		Finding finding;
		const bool signal = group.kind == DuplicateLabels::Kind::Signal;
		if (group.place.isEmpty())
			finding.text = signal ? tr("Duplicate signal: %1").arg(group.label)
								  : tr("Duplicate component: %1").arg(group.label);
		else
			//: %2 is the plant, or the plant and location, such as "=P1+UV1".
			finding.text = signal ? tr("Duplicate signal: %1 in %2").arg(group.label, group.place)
								  : tr("Duplicate component: %1 in %2").arg(group.label, group.place);
		if (!group.parts.isEmpty())
			//: %1 is the finding above, %2 the part numbers, "A9, B1".
			finding.text = tr("%1 — different part numbers: %2")
						   .arg(finding.text, group.parts.join(QStringLiteral(", ")));
		for (const auto &occurrence : group.occurrences)
			if (occurrence.item)
				finding.places << itemPlace(project, occurrence.item, occurrence.label);
		result.findings << finding;
	}
	return result;
}

/**
	@brief ProjectCheck::potentialTypes
	projectDataBase::potentialTypeMismatches(): read from the database, the
	wires are found on their folios when the finding is opened.
*/
ProjectCheck::Result ProjectCheck::potentialTypes(QETProject *project)
{
	Result result;
	result.kind = Kind::PotentialTypes;
	const auto mismatches = project->dataBase()->potentialTypeMismatches();
	if (mismatches.isEmpty() && !project->potentialTypes().isEnabled()) {
		result.note = tr("Potential types are off for this project.");
		return result;
	}
	for (const auto &mismatch : mismatches) {
		Finding finding;
		//: %1 is the potential types, such as "L1 / N"
		finding.text = tr("One potential with wires of types %1")
					   .arg(mismatch.types.join(QStringLiteral(" / ")));
		Place place;
		place.text = tr("%n wire(s)", nullptr, int(mismatch.wires.size()));
		place.wires = mismatch.wires;
		finding.places << place;
		result.findings << finding;
	}
	return result;
}

/**
	@brief ProjectCheck::deviceTypes
	DeviceTypeCheck::check(): the devices not drawn the way their type
	says, those with more contacts than it has, and the contacts linked to
	no master. A complete device is not a finding.
*/
ProjectCheck::Result ProjectCheck::deviceTypes(QETProject *project)
{
	Result result;
	result.kind = Kind::DeviceTypes;
	QList<Element *> unlinked;
	const auto devices = DeviceTypeCheck::check(project, &unlinked);
	if (devices.isEmpty() && unlinked.isEmpty()) {
		result.note = tr("No device of the project has a device type.");
		return result;
	}
	for (const auto &device : devices) {
		const int extra = DeviceTypeCheck::extraContacts(device);
		if (device.complete() && !extra) continue;
		Finding finding;
		Element *head = device.symbols.isEmpty() ? nullptr : device.symbols.first().data();
		if (head) finding.places << itemPlace(project, head, device.designation);
		if (!device.known()) {
			//: %1 is the device, such as "=P1+CP1-K12"; %2 its device type
			finding.text = tr("%1: the project has no device type %2")
						   .arg(device.designation, device.typeName);
			result.findings << finding;
			continue;
		}
		if (device.connector) {
			int pins = 0;
			for (int part : DeviceTypeCheck::pinParts(device.type))
				pins += int(DeviceTypeCheck::pinsOf(device.type.parts.at(part)).size());
			//: %1 is the connector, such as "=P1+CP1-X1"
			finding.text = tr("%1: %2 of %n pins drawn", nullptr, pins)
						   .arg(device.designation)
						   .arg(pins - DeviceTypeCheck::freePins(device).size());
			for (const QString &pin : DeviceTypeCheck::freePins(device))
				finding.places << detail(tr("Free pin: %1").arg(pin));
		} else {
			const int parts = int(device.type.parts.size());
			//: %1 is the device, such as "=P1+CP1-K12"
			finding.text = tr("%1: %2 of %n parts drawn", nullptr, parts)
						   .arg(device.designation).arg(parts - device.missing.size());
			for (int part : device.missing)
				//: %1 describes the part, such as "simple NO 13 14"
				finding.places << detail(tr("Not drawn: %1")
						.arg(DeviceTypeCheck::partText(device.type.parts.at(part))));
		}
		if (extra)
			finding.places << detail(tr("%n contact(s) more than the device type has", nullptr, extra));
		for (int i = 0 ; i < device.symbols.size() ; ++i) {
			if (device.drawn.at(i) != DeviceTypeCheck::Beyond) continue;
			Element *symbol = device.symbols.at(i).data();
			if (!symbol) continue;
			const QString pin = device.connector ? device.pinNames.value(i) : QString();
			QString text;
			if (device.connector && device.doubled.contains(pin))
				text = tr("Pin %1 is drawn twice").arg(pin);
			else if (device.connector && !pin.isEmpty())
				//: %1 is a connector, such as "X1", %2 a pin name, such as "7"
				text = tr("%1 has no pin %2").arg(device.label, pin);
			else if (device.connector)
				text = tr("%1: a terminal drawn without a pin name").arg(device.label);
			else
				text = tr("Beyond the type: %1").arg(symbol->name());
			finding.places << itemPlace(project, symbol, text);
		}
		result.findings << finding;
	}
	if (!unlinked.isEmpty()) {
		Finding finding;
		finding.text = tr("%n contact(s) linked to no master", nullptr, int(unlinked.size()));
		for (Element *symbol : unlinked) {
			const QString label = symbol->elementInformations()
					.value(QStringLiteral("label")).toString();
			//: %1 is the symbol's name, %2 its label
			finding.places << itemPlace(project, symbol, label.isEmpty()
						? symbol->name() : tr("%1 %2").arg(symbol->name(), label));
		}
		result.findings << finding;
	}
	return result;
}

/**
	@brief ProjectCheck::wiresPerTerminal
	The terminals which already have more wires than WiringRules::limit():
	the limit refuses a new wire, but not one drawn before it was set, nor
	one from a project made elsewhere. Read from the conductor table.
*/
ProjectCheck::Result ProjectCheck::wiresPerTerminal(QETProject *project)
{
	Result result;
	result.kind = Kind::WiresPerTerminal;
	const auto rules = project->wiringRules();
	const bool master = WiringRules::masterEnabled();
	const int limit = WiringRules::limit(rules, master, false);
	const int report_limit = WiringRules::limit(rules, master, true);
	if (limit <= 0 && report_limit <= 0) {
		result.note = tr("No limit is set for the wires of a terminal.");
		return result;
	}

	QSqlQuery query = project->dataBase()->newQuery(QStringLiteral(
		"WITH ends AS ("
		" SELECT terminal1_uuid AS tu, terminal1_element_uuid AS eu FROM conductor"
		" UNION ALL SELECT terminal2_uuid, terminal2_element_uuid FROM conductor) "
		"SELECT e.uuid, e.diagram_uuid, e.pos, e.type, i.label, t.name, t.terminal_index, "
		" COUNT(*) AS wires "
		"FROM ends JOIN element e ON e.uuid = ends.eu "
		"LEFT JOIN terminal t ON t.uuid = ends.tu AND t.element_uuid = ends.eu "
		"LEFT JOIN element_info i ON i.element_uuid = e.uuid "
		"GROUP BY ends.eu, ends.tu HAVING COUNT(*) > 1 "
		"ORDER BY (SELECT pos FROM diagram WHERE uuid = e.diagram_uuid), e.pos, i.label"));
	while (query.next()) {
		const QString type = query.value(3).toString();
		const bool report = type == QLatin1String("next_report")
							|| type == QLatin1String("previous_report");
		const int max = report ? report_limit : limit;
		const int wires = query.value(7).toInt();
		if (max <= 0 || wires <= max) continue;
		QString terminal = query.value(5).toString().trimmed();
			// terminal_index counts from 0 in a fixed order of the
			// symbol's terminals; NULL when two share one point
		if (terminal.isEmpty() && !query.value(6).isNull())
			//: %1 numbers the terminals of a symbol, from 1
			terminal = tr("terminal %1").arg(query.value(6).toInt() + 1);
		else if (terminal.isEmpty())
			terminal = tr("a terminal");
		const QString label = query.value(4).toString().trimmed();
		Finding finding;
		//: %1 is the symbol's label and terminal, such as "-K1:A1"; %2 and %3 count wires
		finding.text = tr("%1: %2 wires, the limit is %3")
					   .arg(label.isEmpty() ? terminal : label + QLatin1Char(':') + terminal)
					   .arg(wires).arg(max);
		finding.places << symbolPlace(project, QUuid(query.value(0).toString()),
									  QUuid(query.value(1).toString()),
									  query.value(2).toString(), label);
		result.findings << finding;
	}
	return result;
}

/**
	@brief ProjectCheck::terminalNames
	The symbols placed in the project whose definition gives two terminals
	one name: TerminalNameCheck::repeatedNames(), which the symbol editor
	warns about on saving. Read from each symbol's definition once, since
	the terminal table has a row only for a terminal a wire ends on.
*/
ProjectCheck::Result ProjectCheck::terminalNames(QETProject *project)
{
	Result result;
	result.kind = Kind::TerminalNames;
	QSqlQuery query = project->dataBase()->newQuery(QStringLiteral(
		"SELECT e.definition, e.uuid, e.diagram_uuid, e.pos, i.label "
		"FROM element e LEFT JOIN element_info i ON i.element_uuid = e.uuid "
		"ORDER BY (SELECT pos FROM diagram WHERE uuid = e.diagram_uuid), e.pos"));
	QStringList order;
	QHash<QString, QList<Place>> placed;
	while (query.next()) {
		const QString definition = query.value(0).toString();
		if (definition.isEmpty()) continue;
		if (!placed.contains(definition)) order << definition;
		placed[definition] << symbolPlace(project, QUuid(query.value(1).toString()),
										  QUuid(query.value(2).toString()),
										  query.value(3).toString(),
										  query.value(4).toString());
	}
	for (const QString &definition : std::as_const(order)) {
		const ElementsLocation location = DeviceTypeCheck::symbolLocation(definition, project);
		const QDomNodeList terminals = location.xml().elementsByTagName(QStringLiteral("terminal"));
		QStringList names;
		for (int i = 0 ; i < terminals.count() ; ++i)
			names << terminals.at(i).toElement().attribute(QStringLiteral("name"));
		const auto repeated = TerminalNameCheck::repeatedNames(names);
		if (repeated.isEmpty()) continue;
		Finding finding;
		//: %1 is the symbol's name; %2 the names, such as "L (2), N (2)"
		finding.text = tr("%1: terminals of one name: %2")
					   .arg(location.name(), TerminalNameCheck::describe(repeated));
		finding.places = placed.value(definition);
		result.findings << finding;
	}
	return result;
}
