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
#include "addelementtextcommand.h"
#include "element.h"
#include "dynamicelementtextitem.h"
#include "elementtextitemgroup.h"

#include <QGraphicsScene>


/************************
 * AddElementTextCommand*
 * **********************/

AddElementTextCommand::AddElementTextCommand(Element *element, DynamicElementTextItem *deti, QUndoCommand *parent):
	QUndoCommand(parent),
	m_element(element),
	m_text(deti)
{
	setText(QObject::tr("Ajouter un texte d'élément"));	
}

AddElementTextCommand::~AddElementTextCommand()
{
	if(m_text->parentGroup())
		return;
	
	if(!m_element->dynamicTextItems().contains(m_text))
		delete m_text;
}

void AddElementTextCommand::undo()
{
	m_element->removeDynamicTextItem(m_text);
	if(m_text->scene())
		m_text->scene()->removeItem(m_text);
}

void AddElementTextCommand::redo()
{
	m_text->setParentItem(m_element);
	m_element->addDynamicTextItem(m_text);
}


/***********************
 * AddTextsGroupCommand*
 * *********************/
/**
 * @brief AddTextsGroupCommand::AddTextsGroupCommand
 * @param element : the element to add a new group
 * @param groupe_name : the name of the group
 * @param parent : parent undo
 */
AddTextsGroupCommand::AddTextsGroupCommand(Element *element, QString groupe_name, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_element(element),
	m_name(groupe_name)
{
	setText(QObject::tr("Ajouter un groupe de textes d'élément"));
}

/**
 * @brief AddTextsGroupCommand::~AddTextsGroupCommand
 * Destructor
 */
AddTextsGroupCommand::~AddTextsGroupCommand()
{}

void AddTextsGroupCommand::undo()
{
	if(m_element && m_group)
		m_element.data()->removeTextGroup(m_group);
}

void AddTextsGroupCommand::redo()
{
	if(m_element)
	{
		if(m_first_undo)
		{
			m_group = m_element.data()->addTextGroup(m_name);
			m_first_undo = false;
		}
		else if(m_group)
			m_element.data()->addTextGroup(m_group.data());
	}
}


/**************************
 * RemoveTextsGroupCommand*
 * ************************/
/**
 * @brief RemoveTextsGroupCommand::RemoveTextsGroupCommand
 * @param element : The element where we remove a group
 * @param group : the group to remove
 * @param parent : the parent undo command
 */
RemoveTextsGroupCommand::RemoveTextsGroupCommand(Element *element, ElementTextItemGroup *group, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_element(element),
	m_group(group)
{
	setText(QObject::tr("Supprimer un groupe de textes d'élément"));
}

RemoveTextsGroupCommand::~RemoveTextsGroupCommand()
{}
	
void RemoveTextsGroupCommand::undo()
{
	if(m_element && m_group)
		m_element.data()->addTextGroup(m_group.data());
}

void RemoveTextsGroupCommand::redo()
{
	if(m_element && m_group)
		m_element.data()->removeTextGroup(m_group.data());
}


/************************
 * AddTextToGroupCommand*
 * **********************/
/**
 * @brief AddTextToGroupCommand::AddTextToGroupCommand
 * @param text
 * @param group
 * @param parent
 */
AddTextToGroupCommand::AddTextToGroupCommand(DynamicElementTextItem *text, ElementTextItemGroup *group, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_text(text),
	m_group(group),
	m_element(group->parentElement())
{
	setText(QObject::tr("Insérer un texte d'élément dans un groupe de textes"));
}

/**
 * @brief AddTextToGroupCommand::~AddTextToGroupCommand
 * Destructor
 */
AddTextToGroupCommand::~AddTextToGroupCommand()
{
	if(m_group && m_text && m_element)
	{
		if(!m_group.data()->texts().contains(m_text.data()) &&
		   !m_element.data()->dynamicTextItems().contains(m_text.data()))
			delete m_text.data();
	}
}

void AddTextToGroupCommand::undo()
{
	if(m_element && m_group && m_text)
		m_element.data()->removeTextFromGroup(m_text, m_group);
}

void AddTextToGroupCommand::redo()
{
	if(m_element && m_group && m_text)
		m_element.data()->addTextToGroup(m_text, m_group);
}

/*****************************
 * RemoveTextFromGroupCommand*
 * ***************************/
/**
 * @brief RemoveTextFromGroupCommand::RemoveTextFromGroupCommand
 * @param text : text to add to @group
 * @param group
 * @param parent : parent undo command
 */
RemoveTextFromGroupCommand::RemoveTextFromGroupCommand(DynamicElementTextItem *text, ElementTextItemGroup *group, QUndoCommand *parent):
	QUndoCommand(parent),
	m_text(text),
	m_group(group),
	m_element(group->parentElement())
{
	setText(QObject::tr("Enlever un texte d'élément d'un groupe de textes"));
}

/**
 * @brief RemoveTextFromGroupCommand::~RemoveTextFromGroupCommand
 * Destructor
 */
RemoveTextFromGroupCommand::~RemoveTextFromGroupCommand()
{
	if(m_group && m_text && m_element)
	{
		if(!m_group.data()->texts().contains(m_text.data()) &&
		   !m_element.data()->dynamicTextItems().contains(m_text.data()))
			delete m_text.data();
	}
}

void RemoveTextFromGroupCommand::undo()
{
	if(m_element && m_group && m_text)
		m_element.data()->addTextToGroup(m_text, m_group);
}

void RemoveTextFromGroupCommand::redo()
{
	if(m_element && m_group && m_text)
		m_element.data()->removeTextFromGroup(m_text, m_group);
}


/*****************************
 * AlignmentTextsGroupCommand*
 * ***************************/
/**
 * @brief AlignmentTextsGroupCommand::AlignmentTextsGroupCommand
 * @param group : Group to change the alignment
 * @param new_alignment : the new alignment of the group
 * @param parent : the parent QUndoCommand of this undo
 */
AlignmentTextsGroupCommand::AlignmentTextsGroupCommand(ElementTextItemGroup *group, Qt::Alignment new_alignment, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_group(group),
	m_previous_alignment(group->alignment()),
	m_new_alignment(new_alignment)
{
	setText(QObject::tr("Modifier l'alignement d'un groupe de textes"));
	
		//Text haven't got alignment
	if(m_previous_alignment != Qt::AlignLeft ||
	   m_previous_alignment != Qt::AlignVCenter ||
	   m_previous_alignment != Qt::AlignRight)
	{
		for(DynamicElementTextItem *deti : group->texts())
			m_texts_pos.insert(deti, deti->pos());
	}
}

/**
 * @brief AlignmentTextsGroupCommand::~AlignmentTextsGroupCommand
 * Destructor
 */
AlignmentTextsGroupCommand::~AlignmentTextsGroupCommand()
{}

/**
 * @brief AlignmentTextsGroupCommand::mergeWith
 * Try to merge this command with other command
 * @param other
 * @return true if was merged, else false
 */
bool AlignmentTextsGroupCommand::mergeWith(const QUndoCommand *other)
{
	if (id() != other->id() || other->childCount())
		return false;
	
	AlignmentTextsGroupCommand const *undo = static_cast<const AlignmentTextsGroupCommand *>(other);
	if (m_group != undo->m_group)
		return false;
	
	m_new_alignment= undo->m_new_alignment;
	return true;
}

/**
 * @brief AlignmentTextsGroupCommand::undo
 */
void AlignmentTextsGroupCommand::undo()
{
	if(m_group)
	{
		m_group.data()->setAlignment(m_previous_alignment);
			//The alignment befor this command was free, then we must
			//to restor the pos of each texts
		if(!m_texts_pos.isEmpty())
		{
			for(DynamicElementTextItem *deti : m_group.data()->texts())
			{
				if(m_texts_pos.keys().contains(deti))
					deti->setPos(m_texts_pos.value(deti));
			}
		}
	}
}

/**
 * @brief AlignmentTextsGroupCommand::redo
 */
void AlignmentTextsGroupCommand::redo()
{
	if(m_group)
		m_group.data()->setAlignment(m_new_alignment);
}
