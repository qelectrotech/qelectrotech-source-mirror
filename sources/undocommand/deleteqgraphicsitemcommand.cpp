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
	
	for(DynamicElementTextItem *deti : m_removed_contents.m_element_texts)
	{
		if(deti->parentGroup())
			m_grp_texts_hash.insert(deti, deti->parentGroup());
		else
			m_elmt_text_hash.insert(deti, deti->parentElement());
	}
	
	setText(QString(QObject::tr("supprimer %1", "undo caption - %1 is a sentence listing the removed content")).arg(m_removed_contents.sentence(DiagramContent::All)));
	m_diagram->qgiManager().manage(m_removed_contents.items(DiagramContent::All));
}

DeleteQGraphicsItemCommand::~DeleteQGraphicsItemCommand() {
	m_diagram->qgiManager().release(m_removed_contents.items(DiagramContent::All));
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
}
