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
ElementCollectionItem *ElementCollectionItem::child(int row) {
	return m_child_items.value(row);
}

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

bool ElementCollectionItem::canDropMimeData(const QMimeData *data, Qt::DropAction action, int column) const
{
	Q_UNUSED(data); Q_UNUSED(action); Q_UNUSED(column);
	return false;
}

bool ElementCollectionItem::dropMimeData(const QMimeData *data, Qt::DropAction action, int column)
{
	Q_UNUSED(data); Q_UNUSED(action); Q_UNUSED(column);
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
