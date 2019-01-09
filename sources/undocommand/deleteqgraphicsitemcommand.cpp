/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "deleteqgraphicsitemcommand.h"
#include "dynamicelementtextitem.h"
#include "diagram.h"
#include "element.h"
#include "conductor.h"
#include "conductortextitem.h"
#include "elementtextitemgroup.h"
#include "addelementtextcommand.h"
#include "terminal.h"
#include "diagramcommands.h"

/**
 * @brief DeleteQGraphicsItemCommand::DeleteQGraphicsItemCommand
 * @param diagram : deigram where this undo work
 * @param content : content to remove
 * @param parent : parent undo
 */
DeleteQGraphicsItemCommand::DeleteQGraphicsItemCommand(Diagram *diagram, const DiagramContent &content, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_removed_contents(content),
	m_diagram(diagram)
{
		//If parent element of a dynamic element text item is also in @m_removed_content,
		//we remove it, because when the element will be removed from the scene every child's will also be removed.
	const QSet<DynamicElementTextItem *> elmt_set = m_removed_contents.m_element_texts;
	for(DynamicElementTextItem *deti : elmt_set)
	{
		if (m_removed_contents.m_elements.contains(deti->parentElement()))
			m_removed_contents.m_element_texts.remove(deti);
			
	}
	
		//When remove a deti we must to know his parent item, for re-add deti as child of the parent
		//when undo this command
	for(DynamicElementTextItem *deti : m_removed_contents.m_element_texts)
	{
		if(deti->parentGroup())
			m_grp_texts_hash.insert(deti, deti->parentGroup());
		else
			m_elmt_text_hash.insert(deti, deti->parentElement());
	}
	
		//If parent element of ElementTextItemGroup is also in @m_removed_content,
		//we remove it, because when the element will be removed from the scene every child's will also be removed.
	const QSet<ElementTextItemGroup *> group_set = m_removed_contents.m_texts_groups;
	for(ElementTextItemGroup *group : group_set)
	{
		if(m_removed_contents.m_elements.contains(group->parentElement()))
			m_removed_contents.m_texts_groups.remove(group);
	}
	
		//The deletion of the groups is not managed by this undo, but by a RemoveTextsGroupCommand
	for(ElementTextItemGroup *group : m_removed_contents.m_texts_groups) {
		new RemoveTextsGroupCommand(group->parentElement(), group, this);
	}
	
	m_removed_contents.m_texts_groups.clear();
	setPotentialsOfRemovedElements();
	
	setText(QString(QObject::tr("supprimer %1", "undo caption - %1 is a sentence listing the removed content")).arg(m_removed_contents.sentence(DiagramContent::All)));
	m_diagram->qgiManager().manage(m_removed_contents.items(DiagramContent::All));
}

DeleteQGraphicsItemCommand::~DeleteQGraphicsItemCommand() {
	m_diagram->qgiManager().release(m_removed_contents.items(DiagramContent::All));
}

/**
 * @brief DeleteQGraphicsItemCommand::setPotentialsOfRemovedElements
 * This function creates new conductors (if needed) for conserve the electrical potentials
 * present at the terminals of each removed elements.
 */
void DeleteQGraphicsItemCommand::setPotentialsOfRemovedElements()
{
	for (Element *elmt : m_removed_contents.m_elements)
	{
			//a list of terminals who have at least two conductors docked in.
		QList<Terminal *> terminals_list;
		for (Terminal *t : elmt->terminals()) {
			if (t->conductors().size() >= 2) {
				terminals_list.append(t);
			}
		}
		if (terminals_list.isEmpty()) {
			continue;
		}
		
		for (Terminal *t : terminals_list)
		{
				//All new created conductors will be docked to hub_terminal
			Terminal *hub_terminal = nullptr;
			QList<Terminal *> terminals_to_connect_list;
			
			for (Conductor *c : t->conductors())
			{
				Terminal *other_terminal = c->terminal1 == t ? c->terminal2 : c->terminal1;
				
				if (m_removed_contents.items(DiagramContent::Elements).contains(other_terminal->parentElement()))
				{
					other_terminal = terminalInSamePotential(other_terminal, c);
					if (other_terminal == nullptr) {
						continue;
					}
				}
				
				terminals_to_connect_list.append(other_terminal);
				if (hub_terminal == nullptr) {
					hub_terminal = other_terminal;
				}
					//hub_terminal must be the terminal the more at top left of the diagram.
				else if (other_terminal->scenePos().x() < hub_terminal->scenePos().x()) {
					hub_terminal = other_terminal;
				}
				else if (other_terminal->scenePos().x() == hub_terminal->scenePos().x()) {
					if (other_terminal->scenePos().y() < hub_terminal->scenePos().y()) {
						hub_terminal = other_terminal;
					}
				}
			}
			
			terminals_to_connect_list.removeAll(hub_terminal);
			if (hub_terminal == nullptr || terminals_to_connect_list.isEmpty()) {
				continue;
			}
			
			ConductorProperties properties = hub_terminal->conductors().first()->properties();
			for (Terminal *t : terminals_to_connect_list)
			{
					//If a conductor was already created between these two terminals
					//in this undo command, from another removed element, we do nothing
				bool exist_ = false;
				for (QPair<Terminal *, Terminal *> pair : m_connected_terminals)
				{
					if  (pair.first == hub_terminal && pair.second == t) {
						exist_ = true;
						continue;
					} else if (pair.first == t && pair.second == hub_terminal) {
						exist_ = true;
						continue;
					}
				}
				
				if (exist_ == false)
				{
					m_connected_terminals.append(qMakePair<Terminal *, Terminal *>(hub_terminal, t));
					Conductor *new_cond = new Conductor(hub_terminal, t);
					new_cond->setProperties(properties);
					new AddItemCommand<Conductor*>(new_cond, t->diagram(), QPointF(), this);
				}
			}
		}
	}
}

/**
 * @brief DeleteQGraphicsItemCommand::terminalInSamePotential
 * Return a terminal at the same potential of @terminal, by traveling through the conductors connected to @terminal
 * only if the owner element of the terminal is not delete by this undo command.
 * Return nullptr if a terminal can't be found.
 * @param terminal - terminal from search
 * @param conductor_to_exclude - a conductor to exlcude from search.
 * @return 
 */
Terminal *DeleteQGraphicsItemCommand::terminalInSamePotential(Terminal *terminal, Conductor *conductor_to_exclude)
{
	QList<Conductor *> conductor_list = terminal->conductors();
	conductor_list.removeAll(conductor_to_exclude);
	for(Conductor *c : conductor_list)
	{
		Terminal *other_terminal = c->terminal1 == terminal ? c->terminal2 : c->terminal1;
		if(!m_removed_contents.items(DiagramContent::Elements).contains(other_terminal->parentElement())) {
			return other_terminal;
		}
	}
		//No one of direct conductor of terminal are docked to an element which is not removed
	for(Conductor *c : conductor_list)
	{
		Terminal *other_terminal = c->terminal1 == terminal ? c->terminal2 : c->terminal1;
		Terminal *terminal_to_return = terminalInSamePotential(other_terminal, c);
		if (terminal_to_return != nullptr) {
			return  terminal_to_return;
		}
	}
	
	return nullptr;
}

/**
 * @brief DeleteQGraphicsItemCommand::undo
 * Undo this command
 */
void DeleteQGraphicsItemCommand::undo()
{
	m_diagram->showMe();

	for(QGraphicsItem *item : m_removed_contents.items())
		m_diagram->addItem(item);

		//We relink element after every element was added to diagram
	for(Element *e : m_removed_contents.m_elements)
		for(Element *elmt : m_link_hash[e])
				e->linkToElement(elmt);
	
	for(DynamicElementTextItem *deti : m_removed_contents.m_element_texts)
	{
		if(m_elmt_text_hash.keys().contains(deti))
			m_elmt_text_hash.value(deti)->addDynamicTextItem(deti);
		else if (m_grp_texts_hash.keys().contains(deti))
		{
			Element *elmt = m_grp_texts_hash.value(deti)->parentElement();
			elmt->addDynamicTextItem(deti);
			elmt->addTextToGroup(deti, m_grp_texts_hash.value(deti));
		}
	}
	
	QUndoCommand::undo();
}

/**
 * @brief DeleteQGraphicsItemCommand::redo
 * Redo the delete command
 */
void DeleteQGraphicsItemCommand::redo()
{
	m_diagram -> showMe();

	for(Conductor *c : m_removed_contents.conductors(DiagramContent::AnyConductor))
	{
			//If option one text per folio is enable, and the text item of
			//current conductor is visible (that mean the conductor have the single displayed text)
			//We call adjustTextItemPosition to other conductor at the same potential to keep
			//a visible text on this potential.
		if (m_diagram -> defaultConductorProperties.m_one_text_per_folio && c -> textItem() -> isVisible())
		{
			QList <Conductor *> conductor_list;
			conductor_list << c -> relatedPotentialConductors(false).toList();
			if (conductor_list.count())
				conductor_list.first() -> calculateTextItemPosition();
		}
	}
	
	for(Element *e : m_removed_contents.m_elements)
	{
			//Get linked element, for relink it at undo
		if (!e->linkedElements().isEmpty())
			m_link_hash.insert(e, e->linkedElements());
	}
	
	for(DynamicElementTextItem *deti : m_removed_contents.m_element_texts)
	{
		if(deti->parentGroup() && deti->parentGroup()->parentElement())
			deti->parentGroup()->parentElement()->removeTextFromGroup(deti, deti->parentGroup());

		deti->parentElement()->removeDynamicTextItem(deti);
		deti->setParentItem(nullptr);
	}

	
	for(QGraphicsItem *item : m_removed_contents.items())
		m_diagram->removeItem(item);
	
	QUndoCommand::redo();
}
