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
#include "rotateselectioncommand.h"
#include "element.h"
#include "conductortextitem.h"
#include "independenttextitem.h"
#include "dynamicelementtextitem.h"
#include "elementtextitemgroup.h"
#include "diagramimageitem.h"
#include "diagram.h"
#include "conductor.h"
#include "qet.h"

#include <QGraphicsItem>

RotateSelectionCommand::RotateSelectionCommand(Diagram *diagram, qreal angle, QUndoCommand *parent) :
QUndoCommand(parent),
m_diagram(diagram),
m_angle(angle)
{
	setText(QObject::tr("Pivoter la selection"));
	
	if(!m_diagram->isReadOnly())
	{
		for (QGraphicsItem *item : m_diagram->selectedItems())
		{
			switch (item->type())
			{
				case Element::Type:
					m_element << static_cast<Element *>(item);
					break;
				case ConductorTextItem::Type:
					m_text << static_cast<DiagramTextItem *>(item);
					break;
				case IndependentTextItem::Type:
					m_text << static_cast<DiagramTextItem *>(item);
					break;
				case DynamicElementTextItem::Type:
					if(item->parentItem() && !item->parentItem()->isSelected())
						m_text << static_cast<DiagramTextItem *>(item);
					break;
				case QGraphicsItemGroup::Type:
					if(ElementTextItemGroup *grp = dynamic_cast<ElementTextItemGroup *>(item))
						if(grp->parentElement() && !grp->parentElement()->isSelected())
							m_group << grp;
					break;
				case DiagramImageItem::Type:
					m_image << static_cast<DiagramImageItem *>(item);
					break;
				default:
					break;
			}
		}
	}
}

/**
 * @brief RotateSelectionCommand::undo
 */
void RotateSelectionCommand::undo()
{
	m_diagram->showMe();
	
	for(QPointer<Element> elmt : m_element)
		if(elmt)
			elmt.data()->setRotation(elmt.data()->rotation() - m_angle);
	for(QPointer<DiagramTextItem> text : m_text)
	{
		if(text)
		{
			if(text.data()->type() == ConductorTextItem::Type)
			{
				ConductorTextItem *cti = static_cast<ConductorTextItem *>(text.data());
				cti->forceRotateByUser(m_rotate_by_user.value(text.data()));
				if(cti->wasRotateByUser())
					cti->setRotation(cti->rotation() - m_angle);
				else
					cti->parentConductor()->calculateTextItemPosition();
			}
			else
				text.data()->setRotation(text.data()->rotation() - m_angle);
		}
	}
	for(QPointer<DiagramImageItem> image : m_image)
		if(image)
			image.data()->setRotation(image.data()->rotation() - m_angle);
	for(QPointer<ElementTextItemGroup> group : m_group)
		if(group)
			group.data()->setRotation(group.data()->rotation() - m_angle);
}

/**
 * @brief RotateSelectionCommand::redo
 */
void RotateSelectionCommand::redo()
{
	m_diagram->showMe();
	
	for(QPointer<Element> elmt : m_element)
		if(elmt)
			elmt.data()->setRotation(elmt.data()->rotation() + m_angle);
	for(QPointer<DiagramTextItem> text : m_text)
	{
		if(text)
		{
			if(text.data()->type() == ConductorTextItem::Type)
			{
				ConductorTextItem *cti = static_cast<ConductorTextItem *>(text.data());
				m_rotate_by_user.insert(text.data(), cti->wasRotateByUser());
				cti->forceRotateByUser(true);
			}
			text.data()->setRotation(text.data()->rotation() + m_angle);
		}
	}
	for(QPointer<DiagramImageItem> image : m_image)
		if(image)
			image.data()->setRotation(image.data()->rotation() + m_angle);
	for(QPointer<ElementTextItemGroup> group : m_group)
		if(group)
			group.data()->setRotation(group.data()->rotation() + m_angle);
}

/**
 * @brief RotateSelectionCommand::isValid
 * @return true if this command rotate a least one item.
 */
bool RotateSelectionCommand::isValid()
{
	if(m_element.size()) return true;
	if(m_image.size())   return true;
	if(m_group.size())   return true;
	if(m_text.size())    return true;
	
	return false;
}
