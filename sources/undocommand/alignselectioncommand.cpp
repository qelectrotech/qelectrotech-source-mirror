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
#include "alignselectioncommand.h"

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../alignment.h"
#include "../diagram.h"
#include "../foliogrid.h"
#include "../diagramcontent.h"
#include "../itemgroups.h"
#include "../qetgraphicsitem/diagramimageitem.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/independenttextitem.h"
#include "../qetgraphicsitem/qetshapeitem.h"

#include <QHash>
#include <QSettings>

namespace {
	/**
		One selected item that takes part: where its edges and centre
		are, and the point that goes on the grid, on the grid its own
		drag uses (divided by @a divisor, as TextGrid::snap() does).
	*/
	struct Entry {
		QGraphicsObject *item;
		Alignment::Item geometry;
		QPointF snap_point;
		qreal divisor;
	};

	/**
		@return the items of @a dc that take part, symbols first so a
		group with a symbol in it snaps on that symbol.
	*/
	QList<Entry> entriesOf(const DiagramContent &dc, qreal text_divisor)
	{
		QList<Entry> entries;
			//A symbol's edges are its own outline, without its texts, and
			//its centre is its origin point: that is where its wires leave.
		for (Element *element : std::as_const(dc.m_elements))
			entries << Entry{element, {element->sceneBoundingRect(), element->pos()},
					 element->pos(), 1};
			//A picture's edges are the picture's, without its caption
		for (DiagramImageItem *image : std::as_const(dc.m_images))
		{
			const QRectF rect = image->mapRectToScene(image->imageRect());
			entries << Entry{image, {rect, rect.center()}, image->pos(), 1};
		}
		for (IndependentTextItem *text : std::as_const(dc.m_text_fields))
			entries << Entry{text, {text->sceneBoundingRect(), text->sceneBoundingRect().center()},
					 text->pos(), text_divisor};
			//A shape's edges are the shape as drawn. What goes on the grid is
			//the top-left corner of that box: a rectangle's corner, an
			//ellipse's box, a line's end. Its pos() says nothing a user can
			//see, and a rotated shape's is never on the grid anyway.
		for (QetShapeItem *shape : std::as_const(dc.m_shapes))
		{
			const QRectF rect = shape->sceneOutlineRect();
			entries << Entry{shape, {rect, rect.center()}, rect.topLeft(), 1};
		}
		return entries;
	}

	/**
		@return @a entries sorted into the pieces that line up: a group
		(#1070) is one piece, every other item a piece of its own. Each
		piece is the list of its indexes into @a entries.
	*/
	QList<QList<int>> unitsOf(const QList<Entry> &entries)
	{
		QList<QList<int>> units;
		QHash<QUuid, int> group_units;
		for (int i = 0 ; i < entries.size() ; ++i)
		{
			const QUuid group = ItemGroups::groupOf(entries.at(i).item);
			if (group.isNull()) {
				units << QList<int>{i};
				continue;
			}
			if (!group_units.contains(group)) {
				group_units.insert(group, units.size());
				units << QList<int>();
			}
			units[group_units.value(group)] << i;
		}
		return units;
	}
}

/**
	@brief AlignSelectionCommand::AlignSelectionCommand
	Works out the movement of every selected item for @a mode. Nothing
	moves until the command is pushed.
	Elements redraw their conductors themselves when their position
	changes (see Element's constructor), so a plain "pos" property undo
	per item is enough.
	@param diagram : diagram whose selection is aligned
	@param mode : what to align the selection to
	@param parent : parent undo command
*/
AlignSelectionCommand::AlignSelectionCommand(Diagram *diagram, Mode mode, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_diagram(diagram)
{
	DiagramContent dc(diagram);
	m_locked_count = dc.removeNonMovableItems();

	QSettings settings;
	const int x_grid = FolioGrid::step(settings.value(FolioGrid::x_key), Diagram::xGrid);
	const int y_grid = FolioGrid::step(settings.value(FolioGrid::y_key), Diagram::yGrid);
	const qreal text_divisor = settings.value(TextGrid::settings_key, 1).toReal();

		//Each kind goes where dragging it would have left it: symbols,
		//pictures and shapes on the folio grid, free texts on the text grid.
	const QList<Entry> entries = entriesOf(dc, text_divisor);
	m_item_count = entries.size();

	auto move = [this](QGraphicsObject *item, const QPointF &offset)
	{
		if (!offset.isNull())
			new QPropertyUndoCommand(item, "pos", item->pos(), item->pos() + offset, this);
	};

	if (mode == SnapToGrid)
	{
		for (const Entry &entry : std::as_const(entries))
			move(entry.item, Alignment::gridOffset(entry.snap_point, x_grid, y_grid, entry.divisor));
		setText(QObject::tr("Align %n objects to the grid", "", childCount()));
		return;
	}

		//A group (#1070) lines up as one piece: its edges are its members'
		//together, its centre the middle of that box, and all its members
		//move by the same amount, so the group keeps its shape.
	const QList<QList<int>> units = unitsOf(entries);
	m_item_count = units.size();

		//Lining up a single item on itself would only snap it
	if (units.size() < 2)
		return;

	Alignment::Edge edge = Alignment::Left;
	switch (mode) {
		case AlignLeft:    edge = Alignment::Left;    break;
		case AlignHCenter: edge = Alignment::HCenter; break;
		case AlignRight:   edge = Alignment::Right;   break;
		case AlignTop:     edge = Alignment::Top;     break;
		case AlignVCenter: edge = Alignment::VCenter; break;
		case AlignBottom:  edge = Alignment::Bottom;  break;
		case SnapToGrid:   break;
	}

	QList<Alignment::Item> geometry;
	for (const QList<int> &unit : units)
	{
		QList<Alignment::Item> parts;
		for (int i : unit)
			parts << entries.at(i).geometry;
		geometry << Alignment::combined(parts);
	}
	const QList<QPointF> offsets = Alignment::alignOffsets(geometry, edge);

	for (int u = 0 ; u < units.size() ; ++u)
	{
			//The piece lands on the grid of its first member
		const Entry &first = entries.at(units.at(u).first());
		const QPointF offset = Alignment::snappedOffset(first.snap_point, offsets.at(u), edge,
								x_grid, y_grid, first.divisor);
		for (int i : units.at(u))
			move(entries.at(i).item, offset);
	}
	setText(QObject::tr("Align %n items", "", childCount()));
}

/**
	@brief AlignSelectionCommand::unitCount
	@return the number of pieces the selection in @a dc lines up as:
	every symbol, picture, free text and shape, a group counting as one.
	Locked items are counted too, so the command can say why they did
	not move.
*/
int AlignSelectionCommand::unitCount(const DiagramContent &dc)
{
	return unitsOf(entriesOf(dc, 1)).size();
}

/**
	@brief AlignSelectionCommand::undo
*/
void AlignSelectionCommand::undo()
{
	if (m_diagram)
		m_diagram->showMe();
	QUndoCommand::undo();
}

/**
	@brief AlignSelectionCommand::redo
*/
void AlignSelectionCommand::redo()
{
	if (m_diagram)
		m_diagram->showMe();
	QUndoCommand::redo();
}

/**
	@brief AlignSelectionCommand::isValid
	@return true if this command moves at least one item.
*/
bool AlignSelectionCommand::isValid() const
{
	return childCount() > 0;
}

/**
	@return the number of items this command moves.
*/
int AlignSelectionCommand::movedCount() const
{
	return childCount();
}

/**
	@return the number of selected items left in place because their
	position is locked.
*/
int AlignSelectionCommand::lockedCount() const
{
	return m_locked_count;
}

/**
	@return the number of things that took part: symbols, pictures, free
	texts and shapes whose position is not locked, a group counting as
	one.
*/
int AlignSelectionCommand::itemCount() const
{
	return m_item_count;
}
