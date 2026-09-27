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
#include "../diagramcontent.h"
#include "../itemgroups.h"
#include "../qetgraphicsitem/diagramimageitem.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/independenttextitem.h"
#include "../qetgraphicsitem/qetshapeitem.h"

#include <QHash>
#include <QSettings>

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
	const int x_grid = settings.value(QStringLiteral("diagrameditor/Xgrid"), Diagram::xGrid).toInt();
	const int y_grid = settings.value(QStringLiteral("diagrameditor/Ygrid"), Diagram::yGrid).toInt();
	const qreal text_divisor = settings.value(TextGrid::settings_key, 1).toReal();

		//Each kind goes where dragging it would have left it: symbols and
		//pictures on the folio grid, free texts on the text grid.
		//Shapes are left out: they are made of several points and no single
		//one of them is the obvious one to snap.
	struct Entry {
		QGraphicsObject *item;
		Alignment::Item geometry;
		qreal divisor;
	};
	QList<Entry> entries;
		//A symbol's edges are its own outline, without its texts, and its
		//centre is its origin point: that is where its wires leave.
	for (Element *element : std::as_const(dc.m_elements))
		entries << Entry{element, {element->sceneBoundingRect(), element->pos()}, 1};
		//A picture's edges are the picture's, without its caption
	for (DiagramImageItem *image : std::as_const(dc.m_images))
	{
		const QRectF rect = image->mapRectToScene(image->imageRect());
		entries << Entry{image, {rect, rect.center()}, 1};
	}
	for (IndependentTextItem *text : std::as_const(dc.m_text_fields))
		entries << Entry{text, {text->sceneBoundingRect(), text->sceneBoundingRect().center()}, text_divisor};
	m_item_count = entries.size();

	auto move = [this](QGraphicsObject *item, const QPointF &offset)
	{
		if (!offset.isNull())
			new QPropertyUndoCommand(item, "pos", item->pos(), item->pos() + offset, this);
	};

	if (mode == SnapToGrid)
	{
		for (const Entry &entry : std::as_const(entries))
			move(entry.item, Alignment::gridOffset(entry.item->pos(), x_grid, y_grid, entry.divisor));
		setText(QObject::tr("Aligner %n objet(s) sur la grille", "", childCount()));
		return;
	}

		//A group (#1070) lines up as one piece: its edges are its members'
		//together, its centre the middle of that box, and all its members
		//move by the same amount, so the group keeps its shape. Shapes in a
		//group come along; shapes outside one are left out, as above.
	struct Unit {
		QList<QGraphicsObject *> members;
		Alignment::Item geometry;
		QGraphicsObject *snap_item = nullptr; ///< lands on its grid
		qreal divisor = 1;
	};
	QList<Unit> units;
	QHash<QUuid, int> group_units;
	auto unitFor = [&](QGraphicsObject *item) -> Unit &
	{
		const QUuid group = ItemGroups::groupOf(item);
		if (group.isNull()) {
			units << Unit();
			return units.last();
		}
		if (!group_units.contains(group)) {
			group_units.insert(group, units.size());
			units << Unit();
		}
		return units[group_units.value(group)];
	};
	for (const Entry &entry : std::as_const(entries))
	{
		Unit &unit = unitFor(entry.item);
		unit.members << entry.item;
		unit.geometry.edges = unit.geometry.edges.isNull()
				? entry.geometry.edges
				: unit.geometry.edges.united(entry.geometry.edges);
		unit.geometry.ref = entry.geometry.ref;
			//Symbols come first in entries, so a group with one snaps on it
		if (!unit.snap_item) {
			unit.snap_item = entry.item;
			unit.divisor = entry.divisor;
		}
	}
	for (QetShapeItem *shape : std::as_const(dc.m_shapes))
	{
		if (ItemGroups::groupOf(shape).isNull())
			continue;
		Unit &unit = unitFor(shape);
		unit.members << shape;
		unit.geometry.edges = unit.geometry.edges.isNull()
				? shape->sceneBoundingRect()
				: unit.geometry.edges.united(shape->sceneBoundingRect());
		if (!unit.snap_item)
			unit.snap_item = shape;
	}
	for (Unit &unit : units) {
		if (unit.members.size() > 1)
			unit.geometry.ref = unit.geometry.edges.center();
	}
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
	for (const Unit &unit : std::as_const(units))
		geometry << unit.geometry;
	const QList<QPointF> offsets = Alignment::alignOffsets(geometry, edge);

	for (int i = 0 ; i < units.size() ; ++i)
	{
		const Unit &unit = units.at(i);
		const QPointF offset = Alignment::snappedOffset(unit.snap_item->pos(), offsets.at(i), edge,
								x_grid, y_grid, unit.divisor);
		for (QGraphicsObject *member : unit.members)
			move(member, offset);
	}
	setText(QObject::tr("Aligner %n objet(s)", "", childCount()));
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
	@return the number of things that took part: symbols, pictures and
	free texts whose position is not locked, a group counting as one.
*/
int AlignSelectionCommand::itemCount() const
{
	return m_item_count;
}
