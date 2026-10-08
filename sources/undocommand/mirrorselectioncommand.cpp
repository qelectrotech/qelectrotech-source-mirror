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
#include "mirrorselectioncommand.h"

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../diagram.h"
#include "../qetgraphicsitem/element.h"

/**
	@brief MirrorSelectionCommand::MirrorSelectionCommand
	Mirror the elements selected in @p diagram; the other selected items
	are left as they are. See the other constructor.
	@param diagram : the diagram whose selection is mirrored
	@param orientation : Qt::Horizontal swaps left and right,
	Qt::Vertical swaps top and bottom
	@param parent : parent undo command
*/
MirrorSelectionCommand::MirrorSelectionCommand(Diagram *diagram,
											   Qt::Orientation orientation,
											   QUndoCommand *parent) :
	MirrorSelectionCommand(selectedElements(diagram), orientation, parent)
{}

/**
	@brief MirrorSelectionCommand::MirrorSelectionCommand
	The mirrors of an element are about its own axes, before it is rotated
	(Element::setMirror()). Turned by 0 or 180 degrees, the element's axes
	are the folio's, so a horizontal mirror on the folio is the element's
	own horizontal mirror; turned by 90 or 270 degrees, they are swapped,
	so it is the element's vertical mirror. The rotation never changes.
	Each element is mirrored in place: its centre stays where it was.
	@param elements : the elements to mirror
	@param orientation : Qt::Horizontal swaps left and right on the folio,
	Qt::Vertical swaps top and bottom
	@param parent : parent undo command
*/
MirrorSelectionCommand::MirrorSelectionCommand(const QList<Element *> &elements,
											   Qt::Orientation orientation,
											   QUndoCommand *parent) :
	QUndoCommand(parent)
{
	setText(orientation == Qt::Horizontal
			? QObject::tr("Miroir horizontal")
			: QObject::tr("Miroir vertical"));

	for (Element *element : elements)
	{
		if (!element)
			continue;

		const bool turned = element->orientation() % 2;
		const bool horizontal = (orientation == Qt::Horizontal) != turned;

		bool new_horizontal = element->hasHorizontalMirror();
		bool new_vertical = element->hasVerticalMirror();
		if (horizontal)
		{
			new_horizontal = !new_horizontal;
			new QPropertyUndoCommand(element, "horizontalMirror",
									 !new_horizontal, new_horizontal, this);
		}
		else
		{
			new_vertical = !new_vertical;
			new QPropertyUndoCommand(element, "verticalMirror",
									 !new_vertical, new_vertical, this);
		}

			//The mirror is about the element's hotspot, which is often a
			//corner of the symbol: move it so that it stays where it was,
			//centre on centre, and on the grid
		QPointF centre = element->boundingRect().center();
		centre = QTransform::fromScale(new_horizontal ? -1 : 1, new_vertical ? -1 : 1)
				 .map(centre);
		centre = QTransform().rotate(element->rotation()).map(centre);

		const QPointF old_pos = element->pos();
		const QPointF new_pos = Diagram::snapToGrid(
					element->mapToScene(element->boundingRect().center()) - centre);
		if (new_pos != old_pos)
			new QPropertyUndoCommand(element, "pos", old_pos, new_pos, this);
	}
}

/**
	@brief MirrorSelectionCommand::selectedElements
	@param diagram
	@return the elements selected in @p diagram, none if it is read only
*/
QList<Element *> MirrorSelectionCommand::selectedElements(const Diagram *diagram)
{
	QList<Element *> elements;
	if (!diagram || diagram->isReadOnly())
		return elements;

	const QList<QGraphicsItem *> items = diagram->selectedItems();
	for (QGraphicsItem *item : items)
		if (Element *element = qgraphicsitem_cast<Element *>(item))
			elements << element;
	return elements;
}

/**
	@brief MirrorSelectionCommand::isValid
	@return true if at least one element is mirrored by this command
*/
bool MirrorSelectionCommand::isValid() const
{
	return childCount() > 0;
}
