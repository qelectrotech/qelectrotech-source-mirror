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
#include "shownkinds.h"

#include <QGraphicsItem>
#include <QGraphicsScene>

namespace
{
	bool hidden[ShownKinds::KindCount] = {};
}

/**
	@return true unless @a kind was hidden with setShown().
*/
bool ShownKinds::isShown(Kind kind)
{
	return kind >= KindCount || !hidden[kind];
}

/**
	Show or hide @a kind for the items created from now on. Items already
	on a scene change when apply() is called on it.
*/
void ShownKinds::setShown(Kind kind, bool shown)
{
	if (kind < KindCount) {
		hidden[kind] = !shown;
	}
}

/**
	@return how many kinds are hidden, for the status bar.
*/
int ShownKinds::hiddenCount()
{
	int count = 0;
	for (bool h : hidden) {
		count += h ? 1 : 0;
	}
	return count;
}

/**
	Mark @a item as being of @a kind, and hide it now if that kind is
	hidden. Called by the item's constructor, while the item still wants to
	be visible.
*/
void ShownKinds::tag(QGraphicsItem *item, Kind kind)
{
	if (!item) {
		return;
	}
	item->setData(data_key, int(kind));
	if (!isShown(kind)) {
		item->setVisible(false);
		item->setData(hidden_key, true);
	}
}

/**
	Show or hide @a item as its own code wants, unless its kind is hidden:
	then it stays hidden, and comes back when the kind is shown again.
	Use this instead of QGraphicsItem::setVisible() in a tagged item.
*/
void ShownKinds::setVisible(QGraphicsItem *item, bool visible)
{
	if (!item) {
		return;
	}
	const QVariant kind = item->data(data_key);
	const bool vetoed = visible && kind.isValid() && !isShown(Kind(kind.toInt()));
	item->setVisible(visible && !vetoed);
	item->setData(hidden_key, vetoed ? QVariant(true) : QVariant());
}

/**
	@return true if @a item, or an item it hangs from, is of a hidden kind.
	For code that walks the items itself (DXF export) and must leave out
	what View > Show hides.
*/
bool ShownKinds::isHidden(const QGraphicsItem *item)
{
	for (; item; item = item->parentItem())
	{
		const QVariant value = item->data(data_key);
		if (value.isValid() && !isShown(Kind(value.toInt()))) {
			return true;
		}
	}
	return false;
}

/**
	@return true if @a item is visible, or would be but for its kind being
	hidden. For code that asks isVisible() to learn what an item's own code
	decided (which wire carries the text of a potential).
*/
bool ShownKinds::wantsVisible(const QGraphicsItem *item)
{
	return item && (item->isVisible() || item->data(hidden_key).toBool());
}

/**
	Bring the items of @a kind on @a scene in line with the current state:
	hide the ones that are visible, or show again the ones hidden for it.
*/
void ShownKinds::apply(QGraphicsScene *scene, Kind kind)
{
	if (!scene) {
		return;
	}
	const bool shown = isShown(kind);
		//items() with no argument also returns the hidden items
	const QList<QGraphicsItem *> items = scene->items();
	for (QGraphicsItem *item : items)
	{
		const QVariant value = item->data(data_key);
		if (!value.isValid() || value.toInt() != kind) {
			continue;
		}
		if (!shown) {
				//Its own flag, not isVisible(): a symbol text under a
				//hidden group still wants to be visible
			if (item->isVisibleTo(item->parentItem())) {
				item->setVisible(false);
				item->setData(hidden_key, true);
			}
		} else if (item->data(hidden_key).toBool()) {
			item->setVisible(true);
			item->setData(hidden_key, QVariant());
		}
	}
}
