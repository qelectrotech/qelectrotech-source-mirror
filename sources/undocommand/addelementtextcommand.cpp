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
#include <QGraphicsScene>

AddElementTextCommand::AddElementTextCommand(Element *element, DynamicElementTextItem *deti, QUndoCommand *parent):
	QUndoCommand(parent),
	m_element(element),
	m_text(deti)
{
	setText(QObject::tr("Ajouter un texte d'élément"));	
}

AddElementTextCommand::~AddElementTextCommand()
{
	if(!m_element->dynamicTextItems().contains(m_text))
		delete m_text;
}

void AddElementTextCommand::undo()
{
	m_element->removeDynamicTextItem(m_text);
	m_text->setParentItem(nullptr);
	if(m_text->scene())
		m_text->scene()->removeItem(m_text);
}

void AddElementTextCommand::redo()
{
	m_text->setParentItem(m_element);
	m_element->addDynamicTextItem(m_text);
}

//RemoveElementTextCommand::RemoveElementTextCommand(DynamicElementTextItem *deti, QUndoCommand *parent) :
//	QUndoCommand(parent),
//	m_text(deti)
//{
//	setText(QObject::tr("Supprimer un texte d'élément"));
//	m_element = deti->ParentElement();
//}

//RemoveElementTextCommand::~RemoveElementTextCommand()
//{
//	if(m_element && !m_element->dynamicTextItems().contains(m_text))
//		delete m_text;
//}

//void RemoveElementTextCommand::undo()
//{
//	if(m_element)
//	{
//		m_text->setParentItem(m_element);
//		m_element->addDynamicTextItem(m_text);
//	}
//}

//void RemoveElementTextCommand::redo()
//{
//	if(m_element && m_text->scene())
//	{
		
//		m_element->removeDynamicTextItem(m_text);
//		m_text->setParentItem(nullptr);
//		m_text->scene()->removeItem(m_text);
//	}
//}
