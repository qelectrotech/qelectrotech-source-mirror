/*
        Copyright 2006-2015 The QElectroTech Team
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
#include <QMimeData>

/**
 * @brief ElementCollectionItem::ElementCollectionItem
 * Constructor
 * @param parent : the parent item of this item
 */
ElementCollectionItem::ElementCollectionItem(ElementCollectionItem *parent) :
	m_parent_item (parent)
{}

/**
 * @brief ElementCollectionItem::~ElementCollectionItem
 * Destructor
 */
ElementCollectionItem::~ElementCollectionItem() {
    qDeleteAll(m_child_items);
}

/**
 * @brief ElementCollectionItem::appendChild
 * Add @item to the child of this item
 * @param item
 */
void ElementCollectionItem::appendChild(ElementCollectionItem *item) {
	m_child_items << item;
}

/**
 * @brief ElementCollectionItem::removeChild
 * Remove and delete count childs starting at position row
 * @param row
 * @return true if childs was successfully removed
 */
bool ElementCollectionItem::removeChild(int row, int count)
{
	if (!(0 <= row+count  && row+count <= m_child_items.size())) return false;

	for (int i=0 ; i<count ; i++)
	{
		ElementCollectionItem *eci = m_child_items.takeAt(row);
		delete eci;
	}

	return true;
}

/**
 * @brief ElementCollectionItem::insertChild
 * Insert item at position row in the child item list
 * @param row
 * @param item
 * @return true if item was inserted, if item is already a chil of this item, return false
 */
bool ElementCollectionItem::insertChild(int row, ElementCollectionItem *item)
{
	if (m_child_items.contains(item)) return false;

	m_child_items.insert(row, item);
	return true;
}

/**
 * @brief ElementCollectionItem::child
 * @param row
 * @return The child at @row of this item.
 * If there isn't child at @row, return default ElementCollectionItem
 */
ElementCollectionItem *ElementCollectionItem::child(int row) const {
	return m_child_items.value(row);
}

/**
 * @brief ElementCollectionItem::childWithCollectionName
 * Return the child with the collection name @name, else return nullptr
 * @param name
 * @return
 */
ElementCollectionItem *ElementCollectionItem::childWithCollectionName(QString name) const
{
	foreach (ElementCollectionItem *eci, m_child_items)
		if (eci->collectionName() == name) return eci;

	return nullptr;
}

/**
 * @brief ElementCollectionItem::lastItemForPath
 * Return the last existing item in this ElementCollectionItem hierarchy according to the given path.
 * Next_item is the first non existing item in this hierarchy according to the given path.
 * @param path : The path to find last item. The path must be in form : path/otherPath/.../.../myElement.elmt.
 * @param newt_item : The first item that not exist in this hierarchy
 * @return : The last item that exist in this hierarchy, or nullptr can't find (an error was occurred, or path already exist)
 */
ElementCollectionItem *ElementCollectionItem::lastItemForPath(const QString &path, QString &newt_item)
{
	QStringList str_list = path.split("/");
	if (str_list.isEmpty()) return nullptr;

	ElementCollectionItem *return_eci = this;
	foreach (QString str, str_list)
	{
		ElementCollectionItem *eci = return_eci->childWithCollectionName(str);
		if (!eci)
		{
			newt_item = str;
			return return_eci;
		}
		else
			return_eci = eci;
	}

	return nullptr;
}

/**
 * @brief ElementCollectionItem::rowForInsertItem
 * Return the row for insert a new child item to this item with name @collection_name.
 * If row can't be found (collection_name is null, or already exist) return -1;
 * @param collection_name
 * @return
 */
int ElementCollectionItem::rowForInsertItem(const QString &collection_name)
{
	if (collection_name.isEmpty()) return -1;

	QList <ElementCollectionItem *> child;
		//The item to insert is an element we search from element child
	if (collection_name.endsWith(".elmt"))
	{
		child = elementsChild();
			//There isn't element, we insert at last position
		if (child.isEmpty())
			return childCount();
	}
		//The item is a directory, we search from directory child
	else
	{
		child = directoriesChild();
		//There isn't directory, we insert at first position
		if(child.isEmpty())
			return 0;
	}

	foreach (ElementCollectionItem *eci, child)
		if (eci->collectionName() > collection_name)
			return indexOfChild(eci);

	return (indexOfChild(child.last())+1);
}

/**
 * @brief ElementCollectionItem::insertNewItem
 * By defualt do nothing, implement this method in subclass
 * to handle the insertion of a new item with name collection_name
 */
void ElementCollectionItem::insertNewItem(const QString &collection_name) {Q_UNUSED (collection_name);}

/**
 * @brief ElementCollectionItem::childCount
 * @return the number of childs of this item
 */
int ElementCollectionItem::childCount() const {
    return m_child_items.size();
}

/**
 * @brief ElementCollectionItem::columnCount
 * @return the number of columns (always 1)
 */
int ElementCollectionItem::columnCount() const {
    return 1;
}

/**
 * @brief ElementCollectionItem::data
 * @param column
 * @param role
 * @return the data at @column and @role.
 */
QVariant ElementCollectionItem::data(int column, int role) {
	Q_UNUSED(column);
	Q_UNUSED(role);
	return QVariant();
}

/**
 * @brief ElementCollectionItem::mimeData
 * @return The mime data of this item
 */
QMimeData *ElementCollectionItem::mimeData() {
	return new QMimeData();
}

bool ElementCollectionItem::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column) const
{
	Q_UNUSED(data); Q_UNUSED(action); Q_UNUSED(row); Q_UNUSED(column);
	return false;
}

bool ElementCollectionItem::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column)
{
	Q_UNUSED(data); Q_UNUSED(action); Q_UNUSED(row); Q_UNUSED(column);
	return false;
}

/**
 * @brief ElementCollectionItem::flags
 * @return the flag of this item
 */
Qt::ItemFlags ElementCollectionItem::flags() {
	return (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
}

/**
 * @brief ElementCollectionItem::parent
 * @return parent item of this item.
 * return nullptr if item haven't got parent
 */
ElementCollectionItem *ElementCollectionItem::parent() {
    return m_parent_item;
}

/**
 * @brief ElementCollectionItem::row
 * @return the index of this item, from his parent.
 * If item haven't got parent return 0
 */
int ElementCollectionItem::row() const
{
	if (m_parent_item)
		return m_parent_item->m_child_items.indexOf(const_cast<ElementCollectionItem *>(this));
	else
		return 0;
}

/**
 * @brief ElementCollectionItem::name
 * @return the located name of this item
 */
QString ElementCollectionItem::name() {
	return m_name;
}

/**
 * @brief ElementCollectionItem::collectionName
 * @return The collection name of this item
 */
QString ElementCollectionItem::collectionName() const {
	return QString();
}

/**
 * @brief ElementCollectionItem::isDir
 * @return true if this item represent a directory
 */
bool ElementCollectionItem::isDir() const {
	return false;
}

/**
 * @brief ElementCollectionItem::isElement
 * @return true if this item represent a directory
 */
bool ElementCollectionItem::isElement() const {
	return false;
}

/**
 * @brief ElementCollectionItem::isValid
 * @return true if this item refer to an dir or an element.
 */
bool ElementCollectionItem::isValid() const {
	return false;
}

/**
 * @brief ElementCollectionItem::items
 * @return all child and subchild subsubchild... contained by this item
 * This item isn't stored in the list
 */
QList<ElementCollectionItem *> ElementCollectionItem::items() const
{
	QList<ElementCollectionItem *> list;
	list.append(m_child_items);
	foreach(ElementCollectionItem *eci, m_child_items)
		list.append(eci->items());
	return list;
}

/**
 * @brief ElementCollectionItem::elementsChild
 * @return All elements child of this item
 */
QList<ElementCollectionItem *> ElementCollectionItem::elementsChild() const
{
	QList<ElementCollectionItem *> list;
	foreach (ElementCollectionItem *eci, m_child_items)
		if (eci->isElement())
			list.append(eci);

	return list;
}

/**
 * @brief ElementCollectionItem::directoriesChild
 * @return All directories child of this item
 */
QList<ElementCollectionItem *> ElementCollectionItem::directoriesChild() const
{
	QList<ElementCollectionItem *> list;
	foreach (ElementCollectionItem *eci, m_child_items)
		if (eci->isDir())
			list.append(eci);

	return list;
}

/**
 * @brief ElementCollectionItem::indexOfChild
 * @param child
 * @return the index of child or -1 if not found
 */
int ElementCollectionItem::indexOfChild(ElementCollectionItem *child) const {
	return m_child_items.indexOf(child);
}

/**
 * @brief ElementCollectionItem::canRemoveContent
 * @return true if this item can remove the content that he represent
 * By default return false.
 */
bool ElementCollectionItem::canRemoveContent() {
	return false;
}

/**
 * @brief ElementCollectionItem::removeContent
 * Remove the content that he represent this item (a directory or an element).
 * This method do nothing and return false. Inherit it, to handle removing
 * @return true if the content was successfully removed
 */
bool ElementCollectionItem::removeContent() {
	return false;
}
