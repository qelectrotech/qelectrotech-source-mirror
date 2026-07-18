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
#include "linkelementcommand.h"

#include "../conductorautonumerotation.h"
#include "../diagram.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminal.h"
#include "../ui/potentialselectordialog.h"

/**
	@brief LinkElementCommand::LinkElementCommand
	Constructor
	@param element_ : element where we work the link / unlink
	@param parent : parent undo
*/
LinkElementCommand::LinkElementCommand(Element *element_, QUndoCommand *parent):
	QUndoCommand(parent),
	m_element(element_),
	m_first_redo (true)
{
	m_linked_before = m_linked_after = m_element->linkedElements();
	setText(QObject::tr("Éditer les référence croisé", "edite the cross reference"));
}

/**
	@brief LinkElementCommand::mergeWith
	@param other try to merge this command with other
	@return true if merge with success else false
*/
bool LinkElementCommand::mergeWith(const QUndoCommand *other)
{
	if (id() != other->id() || other->childCount()) return false;
	LinkElementCommand const *undo = static_cast<const LinkElementCommand *> (other);
	if (m_element != undo->m_element) return false;
	m_linked_after = undo->m_linked_after;
	return true;
}

/**
	@brief LinkElementCommand::isLinkable
	@param element_a
	@param element_b
	@param already_linked
	@return true if element_a and element_b can be linked between them.
	There is few condition to be linked :
	1- element_a and element_b must be linkable type. (Ex : A is master and B is slave 'OK', A and B is master 'KO')
	2- For element type slave and report (no matter if element is 'A' or 'B'), the element must be free (not connected to an element)
	3- we can override the section 2 by set already_linked to true. In this case, if slave or report is already
	linked to the other element ('A' or 'B') return true, but if linked to another element (not 'A' or 'B') return false
*/
bool LinkElementCommand::isLinkable(Element *element_a, Element *element_b, bool already_linked)
{
	switch(element_a->linkType())
	{
		case Element::Simple: return false;

		case Element::NextReport:
		{
				//Type isn't good
			if (element_b->linkType() != Element::PreviousReport) return false;
				//two report is free
			if (element_a->isFree() && element_b->isFree()) return true;
				//Reports aren't free but are already linked between them and and already_linked is true
			if (element_a->linkedElements().contains(element_b) && already_linked) return true;

			return false;
		}

		case Element::PreviousReport:
		{
			//Type isn't good
		if (element_b->linkType() != Element::NextReport) return false;
			//two report is free
		if (element_a->isFree() && element_b->isFree()) return true;
			//Reports aren't free but are already linked between them and and already_linked is true
		if (element_a->linkedElements().contains(element_b) && already_linked) return true;

		return false;
		}

		case Element::Master:
		{
			//Type isn't good
		if (element_b->linkType() != Element::Slave) return false;
			//element_b is free
		if (element_b->isFree()) return true;
			//element_b isn't free but already linked to element_a and already_linked is true
		if (element_a->linkedElements().contains(element_b) && already_linked) return true;

		return false;
		}

		case Element::Slave:
		{
				//Type isn't good
			if (element_b->linkType() != Element::Master) return false;
				//Element_a is free
			if (element_a->isFree()) return true;
				//element_a isn't free but already linked to element_b and already_linked is true;
			if (element_b->linkedElements().contains(element_a) && already_linked) return true;

			return false;
		}

		case Element::Terminale: return false;

		default: return false;
	}
}

/**
	@brief LinkElementCommand::setLink
	Replace all linked elements of edited element
	by elements stored in element_list
	This method do several check to know if element can be linked or not.
	@param element_list
*/
void LinkElementCommand::setLink(const QList<Element *>& element_list)
{
	m_linked_after.clear();
	setUpNewLink(element_list, true);
}

/**
	@brief LinkElementCommand::setLink
	This is an overloaded function.
	@param element_
*/
void LinkElementCommand::setLink(Element *element_)
{
	QList<Element *> list;
	list << element_;
	setLink(list);
}

/**
	@brief LinkElementCommand::unlink
	Unlink all elements of element_list from the edited element.
	@param element_list
*/
void LinkElementCommand::unlink(QList<Element *> element_list)
{
	foreach(Element *elmt, element_list)
		m_linked_after.removeAll(elmt);
}

/**
	@brief LinkElementCommand::unlinkAll
	Unlink all element of the edited element
*/
void LinkElementCommand::unlinkAll()
{
	m_linked_after.clear();
}

/**
	@brief LinkElementCommand::undo
	Undo this command
*/
void LinkElementCommand::undo()
{
	if(m_element->diagram()) m_element->diagram()->showMe();

	//Clear group index for slave elements when undoing
	if (m_element->linkType() == Element::Slave)
	{
		int group_idx = m_group_index;
		if (m_group_indices.contains(m_element))
			group_idx = m_group_indices.value(m_element);

		if (group_idx >= 0)
		{
			// Reset master labels on slave terminals
			QList<Terminal *> slave_terms = m_element->terminals();
			for (Terminal *t : slave_terms)
			{
				t->setUseMasterLabel(false);
				t->setMasterLabelIndex(0);
			}

			foreach(Element *elmt, m_element->linkedElements())
			{
				if (elmt->linkType() == Element::Master)
				{
					elmt->setGroupIndexForElement(m_element, -1);
					break;
				}
			}
		}
	}
	else if (m_element->linkType() == Element::Master)
	{
		for (auto it = m_group_indices.constBegin(); it != m_group_indices.constEnd(); ++it)
		{
			Element *slave = it.key();
			if (m_element->linkedElements().contains(slave))
			{
				// Reset master labels on slave terminals
				QList<Terminal *> slave_terms = slave->terminals();
				for (Terminal *t : slave_terms)
				{
					t->setUseMasterLabel(false);
					t->setMasterLabelIndex(0);
				}

				m_element->setGroupIndexForElement(slave, -1);
			}
		}
	}

	makeLink(m_linked_before);
	QUndoCommand::undo();
}

/**
	@brief LinkElementCommand::redo
	Redo this command
*/
void LinkElementCommand::redo()
{
	if(m_element->diagram()) m_element->diagram()->showMe();
	makeLink(m_linked_after);

		//If the action is to link two reports together, we check if the conductors
		//of the new potential have the same text, function, and protocol.
		//if not, a dialog ask what do to.
	if (m_first_redo && (m_element->linkType() & Element::AllReport) \
		&& m_element->conductors().size() \
		&& m_linked_after.size() && m_linked_after.first()->conductors().size())
	{
			//fill list of potential
		QSet <Conductor *> c_list = m_element->conductors().first()->relatedPotentialConductors();
		c_list << m_element->conductors().first();
			//fill list of text
		QStringList str_txt;
		QStringList str_funct;
		QStringList str_tens;
		for (const Conductor *c : c_list)
		{
			str_txt   << c->properties().text;
			str_funct << c->properties().m_function;
			str_tens  << c->properties().m_tension_protocol;
			str_tens  << c->properties().m_wire_color;
			str_tens  << c->properties().m_wire_section;
		}

			//check text list, isn't same in potential, ask user what to do
		if (!QET::eachStrIsEqual(str_txt) || !QET::eachStrIsEqual(str_funct) || !QET::eachStrIsEqual(str_tens))
		{
			PotentialSelectorDialog psd(m_element, this);
			psd.exec();
		}
		m_first_redo = false;
	}
	QUndoCommand::redo();
}

/**
	@brief LinkElementCommand::setUpNewLink
	Update the content of m_link_after with the content of element_list.
	Each linkable element (know via the static method isLinkable)
	is added to m_linked_after
	already_link is used for the static method isLinkable.
	@param element_list
	@param already_link
*/
void LinkElementCommand::setUpNewLink(
	const QList<Element *> &element_list, bool already_link)
{
	//m_element is a master we can connect several element to it
	//if m_element isn't master (may be a report or slave) we can connect only one element
	if (m_element->linkType() == Element::Master || element_list.size() == 1)
	{
		foreach(Element *elmt, element_list)
			if (isLinkable(m_element, elmt, already_link))
				m_linked_after << elmt;
	}
	else
	{
		qDebug() << "LinkElementCommand::setUpNewLink : try to link several elements to a report element or slave element,"
					" only the first element of the list will be taken to be linked";
		foreach(Element *elmt, element_list)
			if (isLinkable(m_element, elmt, already_link))
			{
				m_linked_after << elmt;
				return;
			}
	}
}

/**
	@brief LinkElementCommand::makeLink
	Make the link between m_element and element_list;
	This method unlinks elements if needed.
	@param element_list
*/
void LinkElementCommand::makeLink(const QList<Element *> &element_list)
{
	//List is empty, that means m_element must be free, so we unlink all elements
	if (element_list.isEmpty())
	{
		m_element->unlinkAllElements();
		return;
	}

		//We link all element from element_list
	foreach(Element *elmt, element_list)
		m_element->linkToElement(elmt);

	//Set group index for slave-master links
	if (m_element->linkType() == Element::Slave)
	{
		int group_idx = m_group_index;

		//Check if we have a per-slave group index
		if (m_group_indices.contains(m_element))
			group_idx = m_group_indices.value(m_element);

		if (group_idx >= 0)
		{
			foreach(Element *elmt, element_list)
			{
				if (elmt->linkType() == Element::Master)
				{
					elmt->setGroupIndexForElement(m_element, group_idx);

					// Set master labels on slave terminals
					const auto &groups = elmt->elementData().m_slave_contact_groups;
					if (group_idx < groups.size())
					{
						const QStringList &labels = groups.at(group_idx).labels;
						QList<Terminal *> slave_terms = m_element->terminals();
						// Sort terminals by name (T1, T2, T3...) to match label order
						std::sort(slave_terms.begin(), slave_terms.end(),
							[](Terminal *a, Terminal *b) {
								return a->name() < b->name();
							});
						for (int i = 0; i < slave_terms.size(); ++i)
						{
							if (i < labels.size())
							{
								slave_terms.at(i)->setUseMasterLabel(true);
								slave_terms.at(i)->setMasterLabelIndex(i);
							}
						}
					}
					break;
				}
			}
		}
	}
	else if (m_element->linkType() == Element::Master)
	{
		//For master linking to slaves, set group indices for each slave
		for (auto it = m_group_indices.constBegin(); it != m_group_indices.constEnd(); ++it)
		{
			Element *slave = it.key();
			int group_idx = it.value();
			if (group_idx >= 0 && element_list.contains(slave))
			{
				m_element->setGroupIndexForElement(slave, group_idx);

				// Set master labels on slave terminals
				const auto &groups = m_element->elementData().m_slave_contact_groups;
				if (group_idx < groups.size())
				{
					const QStringList &labels = groups.at(group_idx).labels;
					QList<Terminal *> slave_terms = slave->terminals();
					// Sort terminals by name (T1, T2, T3...) to match label order
					std::sort(slave_terms.begin(), slave_terms.end(),
						[](Terminal *a, Terminal *b) {
							return a->name() < b->name();
						});
					for (int i = 0; i < slave_terms.size(); ++i)
					{
						if (i < labels.size())
						{
							slave_terms.at(i)->setUseMasterLabel(true);
							slave_terms.at(i)->setMasterLabelIndex(i);
						}
					}
				}
			}
		}
	}

	/* At this point there may be unwanted linked elements to m_element.
	 * We must unlink it.
	 * Elements from element_list are wanted so we compare element_list
	 *  to current linked element of m_element
	 */
	QList<Element *> to_unlink = m_element->linkedElements();
	foreach(Element *elmt, element_list)
		to_unlink.removeAll(elmt);

	//All elements stored in to_unlink is unwanted we unlink it from m_element
	if (!to_unlink.isEmpty())
	{
		foreach(Element *elmt, to_unlink)
		{
			m_element->unlinkElement(elmt);
		}
	}
}
