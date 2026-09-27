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
#ifndef ITEMGROUPS_H
#define ITEMGROUPS_H

#include <QList>
#include <QUuid>

class QDomElement;
class QGraphicsItem;
class QGraphicsScene;

/**
	Groups of items on a folio (discussion #1070): symbols, free texts,
	shapes and pictures that select, move, copy and delete together.

	A group is not an object in the scene. Each member keeps its place and
	carries the group's uuid, and selecting one member selects them all;
	moving, copying and deleting then need nothing new, because they act on
	the selection. Re-parenting the members under a QGraphicsItemGroup would
	make every position group-relative, which every piece of code reading a
	symbol's position would have to learn about.

	This part knows nothing of Diagram, so it can be tested on a plain scene.
*/
namespace ItemGroups
{
		/// QGraphicsItem::data() key holding an item's group uuid.
	inline constexpr int data_key = 0x475250; // "GRP"

		/// XML attribute holding it, on <element>, <input>, <shape> and <image>.
	inline constexpr char xml_attribute[] = "group";

	QUuid groupOf(const QGraphicsItem *item);
	void setGroup(QGraphicsItem *item, const QUuid &group);

	void write(QDomElement &xml, const QGraphicsItem *item);
	QUuid read(const QDomElement &xml);

	bool completeSelection(QGraphicsScene *scene,
						   const QList<QGraphicsItem *> &previous,
						   bool toggling);
}

#endif // ITEMGROUPS_H
