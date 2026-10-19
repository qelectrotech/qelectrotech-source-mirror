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
#include "conductorcreator.h"

#include "../conductorautonumerotation.h"
#include "../dataBase/projectdatabase.h"
#include "../diagram.h"
#include "../qetproject.h"
#include "../undocommand/addgraphicsobjectcommand.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminal.h"
#include "../ui/potentialselectordialog.h"
#include "../wiringrules.h"
#include "qgraphicsitem.h"

#include <QPolygonF>

/**
	@brief ConductorCreator::ConductorCreator
	Create an electrical potential between all terminals of terminals_list.
	the terminals of the list must be in the same diagram.
	@param d Diagram
	@param terminals_list QList<Terminal *>
*/
ConductorCreator::ConductorCreator(Diagram *d, QList<Terminal *> terminals_list) :
	m_terminals_list(terminals_list)
{
	if (m_terminals_list.size() <= 1) {
		return;
	}
	m_properties = m_terminals_list.first()->diagram()->defaultConductorProperties;

	if (!setUpPropertieToUse()) {
		return;
	}
	d->undoStack().beginMacro(QObject::tr("Creation of conductors"));
	
	const bool chain = d->project()
			&& WiringRules::chainsWires(d->project()->wiringRules(), WiringRules::masterEnabled());
	QList<Conductor *> c_list;
	for (const auto &pair : terminalPairs(chain))
	{
			//Checked as the chain is built: the wire before this one may
			//have just filled a terminal.
		if (chain && !pair.first->canBeLinkedTo(pair.second)) {
			continue;
		}
		Conductor *cond = new Conductor(pair.first, pair.second);
		cond->setProperties(m_properties);
		cond->setSequenceNum(m_sequential_number);
		d->undoStack().push(new AddGraphicsObjectCommand(cond, d));
		
		c_list.append(cond);
	}
	d->undoStack().endMacro();
	
	for(Conductor *c : c_list) {
		c->refreshText();
			//refreshText() resolves an auto-numbering formula into
			//properties.text without emitting propertiesChange, which is
			//what the project database listens to. The row was inserted
			//while text was still the raw formula ("W%sequ_1"), so without
			//this the wiring list and BOM read the formula, not "W1",
			//until something forces a full rebuild.
		if (d->project() && d->project()->dataBase()) {
			d->project()->dataBase()->updateConductor(c);
		}
	}
}

/**
	@brief ConductorCreator::create
	Create an electrical potential between the terminals of the diagram d, contained in the polygon
	@param d Diagram
	@param polygon : polygon in diagram coordinate
*/
void ConductorCreator::create(Diagram *d, const QPolygonF &polygon)
{
	QList<Terminal *> t_list;
	
	for (QGraphicsItem *item : d->items(polygon))
	{
		if (item->type() == Terminal::Type) {
			t_list.append(qgraphicsitem_cast<Terminal *>(item));
		}
	}
	
	if (t_list.size() <= 1) {
		return;
	} else {
		ConductorCreator cc(d, t_list);
	}
}

/**
	@brief ConductorCreator::needsPotentialChoice
	Whether creating a potential between these terminals would ask the user
	to choose which of several existing potentials to inherit from -- that
	is, whether the constructor would reach PotentialSelectorDialog.

	This exists for callers with nobody there to answer: the dialog is a
	plain QDialog::exec(), not routed through QET::QetMessageBox, so its
	non-interactive mode does not cover it and a headless caller would hang
	on it indefinitely. Such a caller can check this first and decline.
	Exposed here, rather than reimplemented by the caller, so the condition
	cannot drift away from the one setUpPropertieToUse() actually applies.
	@param terminals_list the terminals a potential would be created between
	@return true if the constructor would open the dialog
*/
bool ConductorCreator::needsPotentialChoice(const QList<Terminal *> &terminals_list)
{
	if (terminals_list.size() <= 1) {
		return false;
	}
	return distinctProperties(existingPotential(terminals_list)).size() >= 2;
}

/**
	@brief ConductorCreator::inheritedProperties
	The properties a new wire takes from @p conductor's potential. A
	cable's core names the cable and the core in m_cable, m_cable_uuid
	and m_cable_slot (written by CableManager): none of that belongs to a
	new wire, which is in no cable yet. The text is the potential's wire
	number and is kept, unless it only repeats the core label.
	@param conductor : a conductor of the potential
	@return the properties for a new conductor of that potential
*/
ConductorProperties ConductorCreator::inheritedProperties(const Conductor *conductor)
{
	ConductorProperties properties = conductor->properties();
	if (!properties.m_cable_uuid.isNull())
	{
		if (properties.text == properties.m_cable)
			properties.text.clear();
		properties.m_cable.clear();
		properties.m_cable_uuid = QUuid();
		properties.m_cable_slot = -1;
	}
	return properties;
}

/**
	@brief ConductorCreator::distinctProperties
	@param potentials : one conductor of each potential (existingPotential())
	@return the properties a new wire could inherit from them, each once:
	two potentials that agree leave nothing to choose
*/
QList<ConductorProperties> ConductorCreator::distinctProperties(const QList<Conductor *> &potentials)
{
	QList<ConductorProperties> list;
	for (const Conductor *c : potentials)
	{
		const ConductorProperties properties = inheritedProperties(c);
		if (!list.contains(properties)) {
			list.append(properties);
		}
	}
	return list;
}

/**
	@brief ConductorCreator::propertieToUse
	@return true if the caller should proceed with conductor creation,
	false if the user cancelled the potential-selection dialog (in which
	case no properties were chosen and creation must be aborted rather
	than proceeding with blank/default properties).
*/
bool ConductorCreator::setUpPropertieToUse()
{
	QList<Conductor *> potentials = existingPotential(m_terminals_list);

		//There is an existing potential
		//we get one of them
	if (!potentials.isEmpty())
	{
		const QList<ConductorProperties> cp_list = distinctProperties(potentials);
		if (cp_list.size() >= 2)
		{
			bool cancelled = false;
			m_properties = PotentialSelectorDialog::chosenProperties(cp_list, nullptr, &cancelled);
			if (cancelled) {
				return false;
			}
		}
		else
		{
			m_properties = cp_list.first();
		}
		for (Conductor *c : potentials) {
			if (inheritedProperties(c) == m_properties) {
				m_sequential_number = c->sequenceNum();
				break;
			}
		}
		return true;
	}

		//get a new properties
	ConductorAutoNumerotation::newProperties(m_terminals_list.first()->diagram(), m_properties, m_sequential_number);
	return true;
}

/**
	@brief ConductorCreator::existingPotential
	Return one conductor of each existing potential the terminals of
	@p terminals_list are on.

	A terminal without a wire of its own can still be on a potential: the
	other side of a terminal element, a folio report linked to one that is
	wired. Such a terminal is followed through relatedPotentialTerminal()
	until a wired terminal is found -- the rule Conductor::
	relatedPotentialConductors() applies, so that a wire drawn to the
	free side of a terminal takes the properties (number, colour, line
	style...) of the potential the terminal already carries instead of the
	folio's defaults.
	Two terminals on the same potential give that potential once.
	@param terminals_list the terminals to inspect
	@return c_list QList<Conductor *>
*/
QList<Conductor *> ConductorCreator::existingPotential(const QList<Terminal *> &terminals_list)
{
	QList<Conductor *> c_list;
	QList<QSet<Conductor *>> potentials;

	for (Terminal *t : terminals_list)
	{
		Conductor *seed = nullptr;
		QList<Terminal *> to_visit {t};
		QSet<const Terminal *> visited;
		while (!to_visit.isEmpty() && !seed)
		{
			Terminal *current = to_visit.takeFirst();
			if (visited.contains(current)) {
				continue;
			}
			visited.insert(current);
			if (!current->conductors().isEmpty()) {
				seed = current->conductors().first();
			} else {
				to_visit += relatedPotentialTerminal(current, true);
			}
		}
		if (!seed) {
			continue;
		}

		bool known = false;
		for (const QSet<Conductor *> &potential : std::as_const(potentials)) {
			if (potential.contains(seed)) {
				known = true;
				break;
			}
		}
		if (known) {
			continue;
		}

		QSet<Conductor *> potential = seed->relatedPotentialConductors(true);
		potential.insert(seed);
		potentials.append(potential);
		c_list.append(seed);
	}

	return c_list;
}

/**
	@brief ConductorCreator::hubTerminal
	@return hub_terminal
*/
/**
	@brief ConductorCreator::terminalPairs
	@param chain : the project limits the wires per terminal
	(discussion #1158, WiringRules::chainsWires())
	@return the pairs of terminals to wire: all to one hub terminal (a
	star), or with \p chain one after another (WiringRules::chainOrder()),
	since a star gives the hub a wire per other terminal.
*/
QList<QPair<Terminal *, Terminal *>> ConductorCreator::terminalPairs(bool chain)
{
	QList<QPair<Terminal *, Terminal *>> pairs;
	if (chain)
	{
		QList<QPointF> points;
		for (Terminal *t : std::as_const(m_terminals_list)) {
			points << t->scenePos();
		}
		const QList<int> order = WiringRules::chainOrder(points);
		for (int i = 1 ; i < order.size() ; ++i)
		{
			pairs << qMakePair(m_terminals_list.at(order.at(i - 1)),
							   m_terminals_list.at(order.at(i)));
		}
		return pairs;
	}

	Terminal *hub_terminal = hubTerminal();
	for (Terminal *t : std::as_const(m_terminals_list)) {
		if (t != hub_terminal) {
			pairs << qMakePair(hub_terminal, t);
		}
	}
	return pairs;
}

Terminal *ConductorCreator::hubTerminal()
{
	Terminal *hub_terminal = m_terminals_list.first();
	
	for (Terminal *tt : m_terminals_list)
	{
		if (tt->scenePos().x() < hub_terminal->scenePos().x()) {
			hub_terminal = tt;
		} else if (tt->scenePos().x() == hub_terminal->scenePos().x()) {
			if (tt->scenePos().y() < hub_terminal->scenePos().y()) {
				hub_terminal = tt;
			}
		}
	}
	
	return  hub_terminal;
}
