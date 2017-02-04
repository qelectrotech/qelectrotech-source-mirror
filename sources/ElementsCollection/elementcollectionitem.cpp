/*
	Copyright 2006-2017 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/

#include "elementcollectionitem.h"

/**
 * @brief ElementCollectionItem::ElementCollectionItem
 * Constructor
 */
ElementCollectionItem::ElementCollectionItem()
{}

/**
 * @brief ElementCollectionItem::clearData
 * Reset the data
 */
void ElementCollectionItem::clearData()
{
	setText(QString());
	setToolTip(QString());
	setIcon(QIcon());
}

/**
 * @brief ElementCollectionItem::lastItemForPath
 * Return the last existing item in this ElementCollectionItem hierarchy according to the given path.
 * Next_item is the first non existing item in this hierarchy according to the given path.
 * @param path : The path to find last item. The path must be in form : path/otherPath/.../.../myElement.elmt.
 * @param no_found_path : The first item that not exist in this hierarchy
 * @return : The last item that exist in this hierarchy, or nullptr can't find (an error was occurred, or path already exist)
 */
ElementCollectionItem *ElementCollectionItem::lastItemForPath(const QString &path, QString &no_found_path)
{
	QStringList str_list = path.split("/");
	if (str_list.isEmpty()) return nullptr;

	ElementCollectionItem *return_eci = this;
	for (QString str: str_list)
	{
		ElementCollectionItem *eci = return_eci->childWithCollectionName(str);
		if (!eci)
		{
			no_found_path = str;
			return return_eci;
		}
		else
			return_eci = eci;
	}

	return nullptr;
}

/**
 * @brief ElementCollectionItem::childWithCollectionName
 * Return the child with the collection name @name, else return nullptr
 * @param name
 * @return
 */
ElementCollectionItem *ElementCollectionItem::childWithCollectionName(QString name) const
{
	rowCount();
	for (QStandardItem *qsi: directChilds()) {
		ElementCollectionItem *eci = static_cast<ElementCollectionItem *>(qsi);
		if (eci->name() == name)
			return eci;
	}

	return nullptr;
}

/**
 * @brief ElementCollectionItem::directChilds
 * Return the direct child of this item
 * @return
 */
QList<QStandardItem *> ElementCollectionItem::directChilds() const
{
	QList <QStandardItem *> item_list;

	for (int i=0 ; i<rowCount() ; i++)
		item_list.append(child(i));

	return item_list;
}

/**
 * @brief ElementCollectionItem::rowForInsertItem
 * Return the row for insert a new child item to this item with name @collection_name.
 * If row can't be found (collection_name is null, or already exist) return -1;
 * @param collection_name
 * @return
 */
int ElementCollectionItem::rowForInsertItem(const QString &name)
{
	if (name.isEmpty())
		return -1;

	QList <ElementCollectionItem *> child;
		//The item to insert is an element we search from element child
	if (name.endsWith(".elmt"))
	{
		child = elementsDirectChild();
			//There isn't element, we insert at last position
		if (child.isEmpty())
			return rowCount();
	}
		//The item is a directory, we search from directory child
	else
	{
		child = directoriesDirectChild();
		//There isn't directory, we insert at first position
		if(child.isEmpty())
			return 0;
	}

	for (ElementCollectionItem *eci: child)
		if (eci->name() > name)
			return model()->indexFromItem(eci).row();

	return (model()->indexFromItem(child.last()).row() + 1);
}

/**
 * @brief ElementCollectionItem::itemAtPath
 * @param path
 * @return the item at path or nullptr if doesn't exist
 */
ElementCollectionItem *ElementCollectionItem::itemAtPath(const QString &path)
{
	QStringList str_list = path.split("/");
	if (str_list.isEmpty())
		return nullptr;

	ElementCollectionItem *match_eci = this;
	for (QString str: str_list) {
		ElementCollectionItem *eci = match_eci->childWithCollectionName(str);
		if (!eci)
			return nullptr;
		else
			match_eci = eci;
	}

	return match_eci;
}

/**
 * @brief ElementCollectionItem::elementsDirectChild
 * @return The direct element child of this item
 */
QList<ElementCollectionItem *> ElementCollectionItem::elementsDirectChild() const
{
	QList <ElementCollectionItem *> element_child;

	for (QStandardItem *qsi: directChilds()) {
		ElementCollectionItem *eci = static_cast<ElementCollectionItem *>(qsi);
		if (eci->isElement())
			element_child.append(eci);
	}

	return element_child;
}

/**
 * @brief ElementCollectionItem::directoriesDirectChild
 * @return the direct directory child of this item
 */
QList<ElementCollectionItem *> ElementCollectionItem::directoriesDirectChild() const
{
	QList <ElementCollectionItem *> dir_child;

	for (QStandardItem *qsi: directChilds()) {
		ElementCollectionItem *eci = static_cast<ElementCollectionItem *>(qsi);
		if (eci->isDir())
			dir_child.append(eci);
	}

	return dir_child;
}

/**
 * @brief ElementCollectionItem::elementsChild
 * @return Every elements child (direct and indirect) of this item
 */
QList<ElementCollectionItem *> ElementCollectionItem::elementsChild() const
{
	QList <ElementCollectionItem *> list = elementsDirectChild();

	for (ElementCollectionItem *eci: directoriesChild())
		list.append(eci->elementsDirectChild());

	return list;
}

/**
 * @brief ElementCollectionItem::directoriesChild
 * @return Every directories child (direct and indirect) of this item
 */
QList<ElementCollectionItem *> ElementCollectionItem::directoriesChild() const
{
	QList<ElementCollectionItem *> list = directoriesDirectChild();
	QList<ElementCollectionItem *> child_list;
	for (ElementCollectionItem *eci: list) {
		child_list.append(eci->directoriesChild());
	}

	list.append(child_list);
	return list;
}

/**
 * @brief ElementCollectionItem::items
 * @return every childs of this item (direct and indirect childs)
 */
QList<ElementCollectionItem *> ElementCollectionItem::items() const
{
	QList <ElementCollectionItem *> list;

	for (int i=0 ; i<rowCount() ; i++) {
		ElementCollectionItem *eci = static_cast<ElementCollectionItem *>(child(i));
		list.append(eci);
		list.append(eci->items());
	}

	return list;
}

void setUpData(ElementCollectionItem *eci)
{
	eci->setUpData();
}
