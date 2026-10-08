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
#include "groupitemscommand.h"

#include "../diagram.h"
#include "../itemgroups.h"
#include "../qetgraphicsitem/diagramimageitem.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/independenttextitem.h"
#include "../qetgraphicsitem/qetshapeitem.h"

#include <QSet>

namespace {
	QList<QGraphicsObject *> selectedGroupable(Diagram *diagram)
	{
		QList<QGraphicsObject *> items;
		for (QGraphicsItem *item : diagram->selectedItems()) {
			if (GroupItemsCommand::isGroupable(item)) {
				items << item->toGraphicsObject();
			}
		}
		return items;
	}
}

/**
	@return a command grouping the selected symbols, free texts, shapes and
	pictures of @a diagram into one new group, or nullptr if fewer than two
	are selected. The selection already holds whole groups (see
	ItemGroups::completeSelection()), so grouping items that were in groups
	merges those groups into the new one.
*/
GroupItemsCommand *GroupItemsCommand::group(Diagram *diagram)
{
	if (!canGroup(diagram)) {
		return nullptr;
	}
	auto command = new GroupItemsCommand(diagram, selectedGroupable(diagram), QUuid::createUuid());
	command->setText(QObject::tr("Group %n items", "", command->m_changes.size()));
	return command;
}

/**
	@return a command taking every member of the selected groups of
	@a diagram out of its group, or nullptr if no group is selected.
*/
GroupItemsCommand *GroupItemsCommand::ungroup(Diagram *diagram)
{
	if (!canUngroup(diagram)) {
		return nullptr;
	}
	QList<QGraphicsObject *> items;
	for (QGraphicsObject *item : selectedGroupable(diagram)) {
		if (!ItemGroups::groupOf(item).isNull()) {
			items << item;
		}
	}
	auto command = new GroupItemsCommand(diagram, items, QUuid());
	command->setText(QObject::tr("Ungroup %n items", "", command->m_changes.size()));
	return command;
}

/**
	@return true for the kinds of item a group can hold: symbols, free texts,
	shapes and pictures. Never conductors, which follow their symbols, and
	never a symbol's own texts, which belong to it.
*/
bool GroupItemsCommand::isGroupable(const QGraphicsItem *item)
{
	if (!item) {
		return false;
	}
	switch (item->type())
	{
		case Element::Type:
		case IndependentTextItem::Type:
		case QetShapeItem::Type:
		case DiagramImageItem::Type:
			return true;
		default:
			return false;
	}
}

/**
	@return true if the selection of @a diagram holds at least two items
	that could be grouped, and is not already exactly one group.
*/
bool GroupItemsCommand::canGroup(Diagram *diagram)
{
	if (!diagram || diagram->isReadOnly()) {
		return false;
	}
	const QList<QGraphicsObject *> items = selectedGroupable(diagram);
	if (items.size() < 2) {
		return false;
	}
	QSet<QUuid> groups;
	for (QGraphicsObject *item : items) {
		groups << ItemGroups::groupOf(item);
	}
	return groups.size() > 1 || groups.contains(QUuid());
}

/**
	@return true if the selection of @a diagram holds a grouped item.
*/
bool GroupItemsCommand::canUngroup(Diagram *diagram)
{
	if (!diagram || diagram->isReadOnly()) {
		return false;
	}
	for (QGraphicsObject *item : selectedGroupable(diagram)) {
		if (!ItemGroups::groupOf(item).isNull()) {
			return true;
		}
	}
	return false;
}

/**
	@brief GroupItemsCommand::GroupItemsCommand
	@param diagram : diagram holding @a items
	@param items : items to put in @a group
	@param group : the new group, or a null uuid to ungroup
*/
GroupItemsCommand::GroupItemsCommand(Diagram *diagram,
									 const QList<QGraphicsObject *> &items,
									 const QUuid &group) :
	m_diagram(diagram),
	m_group(group)
{
	for (QGraphicsObject *item : items) {
		m_changes.append({item, ItemGroups::groupOf(item)});
	}
}

/**
	@brief GroupItemsCommand::undo
*/
void GroupItemsCommand::undo()
{
	apply(false);
}

/**
	@brief GroupItemsCommand::redo
*/
void GroupItemsCommand::redo()
{
	apply(true);
}

void GroupItemsCommand::apply(bool redo)
{
	if (!m_diagram) {
		return;
	}
	m_diagram->showMe();
	for (const Change &change : std::as_const(m_changes)) {
		if (change.item) {
			m_diagram->setItemGroup(change.item, redo ? m_group : change.before);
		}
	}
}
