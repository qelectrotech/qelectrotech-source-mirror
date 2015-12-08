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
#include "elementscollectionmodel.h"
#include "elementcollectionitem.h"
#include "qetapp.h"
#include "fileelementcollectionitem.h"

/**
 * @brief ElementsCollectionModel::ElementsCollectionModel
 * Defaut constructor
 * @param parent : parent QObject
 */
ElementsCollectionModel::ElementsCollectionModel(QObject *parent) :
	QAbstractItemModel (parent)
{
	m_root_item = new ElementCollectionItem();
}

ElementsCollectionModel::~ElementsCollectionModel()
{
	delete m_root_item;
}

/**
 * @brief ElementsCollectionModel::index
 * Create a index for child of parent at row @row and column @column.
 * If there isn't child return default QModelIndex
 * @param row : the wanted row
 * @param column : the wanted column
 * @param parent : the parent index
 * @return the wanted index or a unvalid index.
 */
QModelIndex ElementsCollectionModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	ElementCollectionItem *parent_item = nullptr;

	if (!parent.isValid())
		parent_item = m_root_item;
	else
		parent_item = static_cast<ElementCollectionItem*>(parent.internalPointer());

	ElementCollectionItem *child_item = parent_item->child(row);
	if (child_item->isValid())
		return createIndex(row, column, child_item);
	else
		return QModelIndex();
}

/**
 * @brief ElementsCollectionModel::parent
 * @param child :
 * @return the parent index of child if have parent.
 * If child haven't got parent or parent is the root_item, return default index
 */
QModelIndex ElementsCollectionModel::parent(const QModelIndex &child) const
{
	if (!child.isValid())
		return QModelIndex();

	ElementCollectionItem *child_item = static_cast<ElementCollectionItem*> (child.internalPointer());
	ElementCollectionItem *parent_item = child_item->parent();

	if (parent_item == m_root_item)
		return QModelIndex();

	return createIndex(parent_item->row(), 0, parent_item);
}

/**
 * @brief ElementsCollectionModel::rowCount
 * @param parent
 * @return the number of row for @parent.
 * If @parent is unvalid, return the number of row of the root_item
 */
int ElementsCollectionModel::rowCount(const QModelIndex &parent) const
{
	ElementCollectionItem *parent_item = nullptr;

	if (!parent.isValid())
		parent_item = m_root_item;
	else
		parent_item = static_cast<ElementCollectionItem*> (parent.internalPointer());

	return parent_item->childCount();
}

/**
 * @brief ElementsCollectionModel::columnCount
 * @param parent
 * @return the number of column for @parent.
 * If @parent is unvalid, return the number of column of the root_item
 */
int ElementsCollectionModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return static_cast<ElementCollectionItem*>(parent.internalPointer())->columnCount();
	else
		return m_root_item->columnCount();
}

/**
 * @brief ElementsCollectionModel::data
 * @param index
 * @param role
 * @return the data of index for the given role or a default QVariant if no data.
 */
QVariant ElementsCollectionModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	ElementCollectionItem *item = static_cast<ElementCollectionItem*>(index.internalPointer());
	return item->data(index.column(), role);
}

/**
 * @brief ElementsCollectionModel::mimeData
 * @param indexes
 * @return the mime data of the items at @indexes
 */
QMimeData *ElementsCollectionModel::mimeData(const QModelIndexList &indexes) const
{
	QModelIndex index = indexes.first();
	if (index.isValid())
	{
		ElementCollectionItem *item = static_cast<ElementCollectionItem*>(index.internalPointer());
		return item->mimeData();
	}
	else
		return new QMimeData();
}

/**
 * @brief ElementsCollectionModel::flags
 * @param index
 * @return the flags of the item at @index
 */
Qt::ItemFlags ElementsCollectionModel::flags(const QModelIndex &index) const
{
	if (index.isValid())
	{
		ElementCollectionItem *eci = static_cast<ElementCollectionItem*>(index.internalPointer());
		return eci->flags();
	}
	else
		return Qt::NoItemFlags;
}

bool ElementsCollectionModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
	if (!parent.isValid())
		return false;

	if (!hasIndex(row, column, parent)) return false;

	QModelIndex item = index(row, column, parent);

	if (item.isValid())
		return static_cast<ElementCollectionItem*>(item.internalPointer())->canDropMimeData(data, action, column);
	else
		return false;

}

bool ElementsCollectionModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	if (!parent.isValid()) return false;

	if (!hasIndex(row, column, parent)) return false;

	QModelIndex item = index(row, column, parent);

	if (item.isValid())
		return static_cast<ElementCollectionItem*>(item.internalPointer())->dropMimeData(data, action, column);
	else
		return false;
}

QStringList ElementsCollectionModel::mimeTypes() const
{
	QStringList mime_list = QAbstractItemModel::mimeTypes();
	mime_list << "application/x-qet-element-uri";
	return mime_list;
}

/**
 * @brief ElementsCollectionModel::addCommonCollection
 * Add the common elements collection to this model
 */
void ElementsCollectionModel::addCommonCollection()
{
	FileElementCollectionItem *feci = new FileElementCollectionItem(m_root_item);
	if (feci->setRootPath(QETApp::commonElementsDir()))
		m_root_item->appendChild(feci);
	else
		delete feci;
}

/**
 * @brief ElementsCollectionModel::addCustomCollection
 * Add the custom elements collection to this model
 */
void ElementsCollectionModel::addCustomCollection()
{
	FileElementCollectionItem *feci = new FileElementCollectionItem(m_root_item);
	if (feci->setRootPath(QETApp::customElementsDir()))
		m_root_item->appendChild(feci);
	else
		delete feci;
}
