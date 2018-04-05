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
#include "QPropertyUndoCommand/qpropertyundocommand.h"

#include <QGraphicsItem>

RotateSelectionCommand::RotateSelectionCommand(Diagram *diagram, qreal angle, QUndoCommand *parent) :
QUndoCommand(parent),
m_diagram(diagram)
{
	setText(QObject::tr("Pivoter la selection"));
	
	if(!m_diagram->isReadOnly())
	{
		for (QGraphicsItem *item : m_diagram->selectedItems())
		{
			switch (item->type())
			{
				case Element::Type:
					m_undo << new QPropertyUndoCommand(item->toGraphicsObject(), "rotation", QVariant(item->rotation()), QVariant(item->rotation()+angle), this);
					break;
				case ConductorTextItem::Type:
				{
					m_cond_text << static_cast<ConductorTextItem *>(item);
					m_undo << new QPropertyUndoCommand(item->toGraphicsObject(), "rotation", QVariant(item->rotation()), QVariant(item->rotation()+angle), this);
				}
					break;
				case IndependentTextItem::Type:
					m_undo << new QPropertyUndoCommand(item->toGraphicsObject(), "rotation", QVariant(item->rotation()), QVariant(item->rotation()+angle), this);
					break;
				case DynamicElementTextItem::Type:
				{
					if(item->parentItem() && !item->parentItem()->isSelected())
						m_undo << new QPropertyUndoCommand(item->toGraphicsObject(), "rotation", QVariant(item->rotation()), QVariant(item->rotation()+angle), this);
				}
					break;
				case QGraphicsItemGroup::Type:
				{
					if(ElementTextItemGroup *grp = dynamic_cast<ElementTextItemGroup *>(item))
						if(grp->parentElement() && !grp->parentElement()->isSelected())
							m_undo << new QPropertyUndoCommand(grp, "rotation", QVariant(item->rotation()), QVariant(item->rotation()+angle), this);
				}
					break;
				case DiagramImageItem::Type:
					m_undo << new QPropertyUndoCommand(item->toGraphicsObject(), "rotation", QVariant(item->rotation()), QVariant(item->rotation()+angle), this);
					break;
				default:
					break;
			}
		}
		
		for (QPropertyUndoCommand *undo : m_undo)
			undo->setAnimated(true, false);
	}
}

/**
 * @brief RotateSelectionCommand::undo
 */
void RotateSelectionCommand::undo()
{
	m_diagram->showMe();
	QUndoCommand::undo();
	
	for(QPointer<ConductorTextItem> cti : m_cond_text)
	{
		cti->forceRotateByUser(m_rotate_by_user.value(cti.data()));
		if(!cti->wasRotateByUser())
			cti->parentConductor()->calculateTextItemPosition();
	}
}

/**
 * @brief RotateSelectionCommand::redo
 */
void RotateSelectionCommand::redo()
{
	m_diagram->showMe();
	QUndoCommand::redo();
	
		for(QPointer<ConductorTextItem> cti : m_cond_text)
		{
			m_rotate_by_user.insert(cti, cti->wasRotateByUser());
			cti->forceRotateByUser(true);
		}
}

/**
 * @brief RotateSelectionCommand::isValid
 * @return true if this command rotate a least one item.
 */
bool RotateSelectionCommand::isValid()
{
	if(childCount())
		return true;
	else
		return false;
}
