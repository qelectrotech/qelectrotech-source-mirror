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
#include "rotateselectioncommand.h"

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../diagram.h"
#include "../qet.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/conductortextitem.h"
#include "../qetgraphicsitem/diagramimageitem.h"
#include "../qetgraphicsitem/dynamicelementtextitem.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/elementtextitemgroup.h"
#include "../qetgraphicsitem/independenttextitem.h"

#include <QGraphicsItem>
#include <QtMath>

RotateSelectionCommand::RotateSelectionCommand(Diagram *diagram, qreal angle, QUndoCommand *parent, bool rotate_as_group) :
QUndoCommand(parent),
m_diagram(diagram)
{
	setText(rotate_as_group ? QObject::tr("Pivoter le groupe") : QObject::tr("Pivoter la selection"));

	if(!m_diagram->isReadOnly())
	{
			//Shared pivot for group rotation: the bounding-box center of
			//everything selected, computed once up front from the
			//selection as a whole (not just the items that end up being
			//individually repositioned below).
		QPointF pivot;
		if (rotate_as_group)
		{
			QRectF bounding_rect;
			for (QGraphicsItem *item : m_diagram->selectedItems())
				bounding_rect |= item->sceneBoundingRect();
			pivot = bounding_rect.center();
		}

		for (QGraphicsItem *item : m_diagram->selectedItems())
		{
			switch (item->type())
			{
				case Element::Type:
					m_undo << new QPropertyUndoCommand(item->toGraphicsObject(), "rotation", QVariant(item->rotation()), QVariant(item->rotation()+angle), this);
					if (rotate_as_group)
						addGroupPositionUndo(item, pivot, angle);
					break;
				case ConductorTextItem::Type:
				{
					m_cond_text << static_cast<ConductorTextItem *>(item);
					m_undo << new QPropertyUndoCommand(item->toGraphicsObject(), "rotation", QVariant(item->rotation()), QVariant(item->rotation()+angle), this);
				}
					break;
				case IndependentTextItem::Type:
					m_undo << new QPropertyUndoCommand(item->toGraphicsObject(), "rotation", QVariant(item->rotation()), QVariant(item->rotation()+angle), this);
					if (rotate_as_group)
						addGroupPositionUndo(item, pivot, angle);
					break;
				case DynamicElementTextItem::Type:
				{
						//No pos() undo here even in group mode: this item is
						//only rotated in place when its parent Element isn't
						//also selected (guard below), and its pos() is
						//parent-local, not scene coordinates -- when the
						//parent Element *is* selected and gets its own pos()
						//rotated around the shared pivot above, this child
						//text item is carried along for free by Qt's normal
						//parent/child transform propagation.
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
					if (rotate_as_group)
						addGroupPositionUndo(item, pivot, angle);
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
	@brief RotateSelectionCommand::addGroupPositionUndo
	Queue a "pos" QPropertyUndoCommand that rotates @a item's position
	around @a pivot by @a angle degrees (Qt's clockwise-positive
	convention, matching QGraphicsItem::setRotation() so a group
	rotation turns the same direction as each item's own spin).
	Only meaningful for items whose pos() is in scene coordinates
	(Element, IndependentTextItem, DiagramImageItem) -- never call this
	for a child item positioned relative to its own parent.
	@param item : item to reposition, its own rotation undo already queued
	@param pivot : shared pivot point, in scene coordinates
	@param angle : rotation angle in degrees
*/
void RotateSelectionCommand::addGroupPositionUndo(QGraphicsItem *item, const QPointF &pivot, qreal angle)
{
	const QPointF old_pos = item->pos();
	const qreal radians = qDegreesToRadians(angle);
	const QPointF delta = old_pos - pivot;
	const QPointF new_pos(
		pivot.x() + delta.x() * qCos(radians) - delta.y() * qSin(radians),
		pivot.y() + delta.x() * qSin(radians) + delta.y() * qCos(radians)
	);
	m_undo << new QPropertyUndoCommand(item->toGraphicsObject(), "pos", QVariant(old_pos), QVariant(new_pos), this);
}

/**
	@brief RotateSelectionCommand::undo
*/
void RotateSelectionCommand::undo()
{
	m_diagram->showMe();
	QUndoCommand::undo();
	
	for(const QPointer<ConductorTextItem>& cti : m_cond_text)
	{
		cti->forceRotateByUser(m_rotate_by_user.value(cti.data()));
		if(!cti->wasRotatedByUser())
			cti->parentConductor()->calculateTextItemPosition();
	}
}

/**
	@brief RotateSelectionCommand::redo
*/
void RotateSelectionCommand::redo()
{
	m_diagram->showMe();
	QUndoCommand::redo();
	
		for(const QPointer<ConductorTextItem>& cti : m_cond_text)
		{
			m_rotate_by_user.insert(cti, cti->wasRotatedByUser());
			cti->forceRotateByUser(true);
		}
}

/**
	@brief RotateSelectionCommand::isValid
	@return true if this command rotate a least one item.
*/
bool RotateSelectionCommand::isValid()
{
	if(childCount())
		return true;
	else
		return false;
}
