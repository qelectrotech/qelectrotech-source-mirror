/*
	Copyright 2006-2019 The QElectroTech Team
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
#include "diagram.h"
#include "qgraphicsitem.h"
#include "terminal.h"
#include "conductor.h"
#include "potentialselectordialog.h"
#include "diagramcommands.h"
#include "conductorautonumerotation.h"
#include "element.h"

#include <QPolygonF>

/**
 * @brief ConductorCreator::ConductorCreator
 * Create an electrical potential between all terminals of @terminals_list.
 * the terminals of the list must be in the same diagram.
 * @param terminals_list
 */
ConductorCreator::ConductorCreator(Diagram *d, QList<Terminal *> terminals_list) :
	m_terminals_list(terminals_list)
{
	if (m_terminals_list.size() <= 1) {
		return;
	}
	m_properties = m_terminals_list.first()->diagram()->defaultConductorProperties;
	
	setUpPropertieToUse();
	Terminal *hub_terminal = hubTerminal();
	
	d->undoStack().beginMacro(QObject::tr("Création de conducteurs"));
	
	QList<Conductor *> c_list;
	for (Terminal *t : m_terminals_list)
	{
		if (t == hub_terminal) {
			continue;
		}
		
		Conductor *cond = new Conductor(hub_terminal, t);
		cond->setProperties(m_properties);
		cond->setSequenceNum(m_sequential_number);
		d->undoStack().push(new AddItemCommand<Conductor *>(cond, d));
		
		c_list.append(cond);
	}
	d->undoStack().endMacro();
	
	for(Conductor *c : c_list) {
		c->refreshText();
	}
}

/**
 * @brief ConductorCreator::create
 * Create an electrical potential between the terminals of the diagram d, contained in the polygon
 * @param d
 * @param polygon : polygon in diagram coordinate
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
 * @brief ConductorCreator::propertieToUse
 * @return the conductor properties to use for the new conductors.
 */
void ConductorCreator::setUpPropertieToUse()
{
	QList<Conductor *> potentials = existingPotential();
	
		//There is an existing potential
		//we get one of them
	if (!potentials.isEmpty())
	{
		if (potentials.size() >= 2)
		{
			QList <ConductorProperties> cp_list;
			for(Conductor *c : potentials) {
				cp_list.append(c->properties());
			}
			
			m_properties = PotentialSelectorDialog::chosenProperties(cp_list);
			for (Conductor *c : potentials) {
				if (c->properties() == m_properties) {
					m_sequential_number = c->sequenceNum();
				}
			}
		}
		else if (potentials.size() == 1)
		{
			m_properties = potentials.first()->properties();
			m_sequential_number = potentials.first()->sequenceNum();
		}
		return;
	}
	
		//get a new properties
	ConductorAutoNumerotation::newProperties(m_terminals_list.first()->diagram(), m_properties, m_sequential_number);
}

/**
 * @brief ConductorCreator::existingPotential
 * Return the list of existing potential of
 * the terminal list
 * @return 
 */
QList<Conductor *> ConductorCreator::existingPotential()
{
	QList<Conductor *> c_list;
	QList<Terminal *> t_exclude;
	
	for (Terminal *t : m_terminals_list)
	{
		if (t_exclude.contains(t)) {
			continue;
		}
		
		if (!t->conductors().isEmpty())
		{
			c_list.append(t->conductors().first());
			
				//We must to check m_terminals_list contain a terminal
				//in the same potential of c, and if true, exclude this terminal from the search.
			for (Conductor *c : t->conductors().first()->relatedPotentialConductors(false))
			{
				if (m_terminals_list.contains(c->terminal1)) {
					t_exclude.append(c->terminal1);
				} else if (m_terminals_list.contains(c->terminal2)) {
					t_exclude.append(c->terminal2);
				}
			}
		}
		else if (t->parentElement()->linkType() & Element::AllReport && !t->parentElement()->isFree())
		{
			Element *linked_report = t->parentElement()->linkedElements().first();
			if (!linked_report->conductors().isEmpty()) {
				c_list.append(linked_report->conductors().first());
			}
		}
	}
	
	return c_list;
}

/**
 * @brief ConductorCreator::hubTerminal
 * @return 
 */
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
