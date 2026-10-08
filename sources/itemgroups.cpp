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
#include "itemgroups.h"

#include <QDomElement>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QSet>

/**
	@return the uuid of the group @a item belongs to, or a null uuid.
*/
QUuid ItemGroups::groupOf(const QGraphicsItem *item)
{
	return item ? item->data(data_key).toUuid() : QUuid();
}

/**
	Put @a item in @a group, or take it out of any group if @a group is null.
*/
void ItemGroups::setGroup(QGraphicsItem *item, const QUuid &group)
{
	if (!item) {
		return;
	}
	item->setData(data_key, group.isNull() ? QVariant() : QVariant(group));
}

/**
	Write @a item's group, if it has one, on its XML element @a xml.
	Nothing is written otherwise, so a project without groups saves exactly
	as before.
*/
void ItemGroups::write(QDomElement &xml, const QGraphicsItem *item)
{
	const QUuid group = groupOf(item);
	if (!group.isNull()) {
		xml.setAttribute(QString::fromLatin1(xml_attribute), group.toString());
	}
}

/**
	@return the group written on @a xml, or a null uuid.
*/
QUuid ItemGroups::read(const QDomElement &xml)
{
	return QUuid(xml.attribute(QString::fromLatin1(xml_attribute)));
}

/**
	@return @a item, or its nearest ancestor, that belongs to a group -- a
	click on a symbol's own text hits the text, but the symbol is the member
	-- or nullptr if none does.
*/
QGraphicsItem *ItemGroups::groupedItem(QGraphicsItem *item)
{
	for (; item; item = item->parentItem()) {
		if (!groupOf(item).isNull()) {
			return item;
		}
	}
	return nullptr;
}

/**
	@return the member a click on @a hit may pick out on its own: the grouped
	item hit, when every member of its group is selected already. A first
	click selects the whole group; a second click, on a member of the group
	it selected, is how the user asks for that member alone (discussion
	#1070). nullptr when the click is not that.
*/
QGraphicsItem *ItemGroups::memberToPick(QGraphicsItem *hit)
{
	QGraphicsItem *member = groupedItem(hit);
	if (!member || !member->isSelected() || !member->scene()) {
		return nullptr;
	}
	const QUuid group = groupOf(member);
	int members = 0;
	for (QGraphicsItem *item : member->scene()->items()) {
		if (groupOf(item) == group) {
			if (!item->isSelected()) {
				return nullptr;
			}
			++members;
		}
	}
	return members > 1 ? member : nullptr;
}

/**
	Make the selection of @a scene whole groups again after it changed.
	A group with a selected member is selected entirely, except when the
	change took members of an entirely selected group out of the selection
	while @a toggling (Ctrl+click on a member): then the whole group leaves
	the selection, as the user meant.
	@param previous : the selection before the change
	@param toggling : true when Ctrl is held
	@return true if the selection was changed
*/
bool ItemGroups::completeSelection(QGraphicsScene *scene,
								   const QList<QGraphicsItem *> &previous,
								   bool toggling)
{
	if (!scene) {
		return false;
	}

	QSet<QUuid> touched;
	const QList<QGraphicsItem *> selected = scene->selectedItems();
	for (QGraphicsItem *item : selected) {
		const QUuid group = groupOf(item);
		if (!group.isNull()) {
			touched << group;
		}
	}

		//Groups that just lost a member from the selection
	QSet<QUuid> shrunk;
	for (QGraphicsItem *item : previous) {
		if (item->scene() == scene && !item->isSelected()) {
			const QUuid group = groupOf(item);
			if (!group.isNull()) {
				shrunk << group;
			}
		}
	}

	if (touched.isEmpty()) {
		return false;
	}

		//One pass over the folio for every touched group, not one per group:
		//select all on a folio of 2000 items in 200 groups took 44 ms the
		//other way.
	bool changed = false;
	for (QGraphicsItem *item : scene->items())
	{
		const QUuid group = groupOf(item);
		if (group.isNull() || !touched.contains(group)) {
			continue;
		}
		const bool select = !(toggling && shrunk.contains(group));
		if (item->isSelected() != select) {
			item->setSelected(select);
			changed = true;
		}
	}
	return changed;
}

/**
	@return the group @a selected is exactly, whole -- every item in it
	belongs to that group and every member of the group is in it -- or a null
	uuid. Rotating such a selection turns the group as one piece rather than
	each member in place (discussion #1070). A member picked out on its own
	is not a whole group, and turns in place.
	@param selected : the selected items that can be members (the caller
	leaves out wires, which follow their symbols)
*/
QUuid ItemGroups::soleWholeGroup(const QList<QGraphicsItem *> &selected)
{
	if (selected.isEmpty() || !selected.first()->scene()) {
		return QUuid();
	}
	const QUuid group = groupOf(selected.first());
	if (group.isNull()) {
		return QUuid();
	}
	for (QGraphicsItem *item : selected) {
		if (groupOf(item) != group) {
			return QUuid();
		}
	}
	for (QGraphicsItem *item : selected.first()->scene()->items()) {
		if (groupOf(item) == group && !item->isSelected()) {
			return QUuid();
		}
	}
	return group;
}
