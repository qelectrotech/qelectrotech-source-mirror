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
#include "xmlprojectelementcollectionitem.h"
#include "qetproject.h"
#include "xmlelementcollection.h"

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
 * @brief ElementsCollectionModel::removeRows
 * Reimplemented from QAbstractItemModel
 * @param row
 * @param count
 * @param parent
 * @return true if rows was successfully removed
 */
bool ElementsCollectionModel::removeRows(int row, int count, const QModelIndex &parent)
{
	ElementCollectionItem *eci = nullptr;
	if (!parent.isValid())
		eci = m_root_item;
	else
		eci = static_cast<ElementCollectionItem *>(parent.internalPointer());

	if (!(1 <= row+count && row+count <= eci->childCount())) return false;

	beginRemoveRows(parent, row, (row + count -1));
	bool r = eci->removeChild(row, count);
	endRemoveRows();

	return r;
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
	if (!(QAbstractItemModel::canDropMimeData(data, action, row, column, parent) && parent.isValid())) return false;

	ElementCollectionItem *eci = static_cast <ElementCollectionItem *>(parent.internalPointer());
	if (!eci) return false;

	return eci->canDropMimeData(data, action, row, column);
}

bool ElementsCollectionModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	if (!parent.isValid()) return false;

	ElementCollectionItem *eci =  static_cast<ElementCollectionItem*> (parent.internalPointer());
	if (!eci || eci->isElement()) return false;

	m_parent_at_drop = parent;

	connect(eci, &ElementCollectionItem::beginInsertRows, this, &ElementsCollectionModel::bir);
	connect(eci, &ElementCollectionItem::endInsertRows,   this, &ElementsCollectionModel::endInsertRows);
	connect(eci, &ElementCollectionItem::beginRemoveRows, this, &ElementsCollectionModel::brr);
	connect(eci, &ElementCollectionItem::endRemoveRows,   this, &ElementsCollectionModel::endRemoveRows);

	bool rb = eci->dropMimeData(data, action, row, column);

	disconnect(eci, &ElementCollectionItem::beginInsertRows, this, &ElementsCollectionModel::bir);
	disconnect(eci, &ElementCollectionItem::endInsertRows,   this, &ElementsCollectionModel::endInsertRows);
	disconnect(eci, &ElementCollectionItem::beginRemoveRows, this, &ElementsCollectionModel::brr);
	disconnect(eci, &ElementCollectionItem::endRemoveRows,   this, &ElementsCollectionModel::endRemoveRows);

	m_parent_at_drop = QModelIndex();

	return rb;
}

QStringList ElementsCollectionModel::mimeTypes() const
{
	QStringList mime_list = QAbstractItemModel::mimeTypes();
	mime_list << "application/x-qet-element-uri" << "application/x-qet-category-uri";
	return mime_list;
}

/**
 * @brief ElementsCollectionModel::items
 * @return All items handled by this model. The root item isn't stored in the list
 */
QList<ElementCollectionItem *> ElementsCollectionModel::items() const
{
	QList <ElementCollectionItem *> list;
	list.append(m_root_item->items());
	return list;
}

/**
 * @brief ElementsCollectionModel::addCommonCollection
 * Add the common elements collection to this model
 */
void ElementsCollectionModel::addCommonCollection()
{
	FileElementCollectionItem *feci = new FileElementCollectionItem(m_root_item);
	if (feci->setRootPath(QETApp::commonElementsDirN()))
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
	if (feci->setRootPath(QETApp::customElementsDirN()))
		m_root_item->appendChild(feci);
	else
		delete feci;
}

/**
 * @brief ElementsCollectionModel::addProject
 * Add @project to the displayed collection
 * @param project
 * @return true if project was successfully added. If project is already
 * handled, return false.
 */
bool ElementsCollectionModel::addProject(QETProject *project)
{
	if (m_project_list.contains(project)) return false;

	m_project_list.append(project);
	int row = m_project_list.indexOf(project);
	beginInsertRows(QModelIndex(), row, row);
	XmlProjectElementCollectionItem *xpeci = new XmlProjectElementCollectionItem(project, m_root_item);
	bool r = m_root_item->insertChild(row, xpeci);
	endInsertRows();
	connect(project->embeddedElementCollection(), &XmlElementCollection::elementAdded, this, &ElementsCollectionModel::elementIntegratedToCollection);
	connect(project->embeddedElementCollection(), &XmlElementCollection::elementChanged, this, &ElementsCollectionModel::updateItem);

	return r;
}

/**
 * @brief ElementsCollectionModel::removeProject
 * Remove @project from this model
 * @param project
 * @return true if the project was successfully removed, false if not (or project doesn't managed)
 */
bool ElementsCollectionModel::removeProject(QETProject *project)
{
	if (!m_project_list.contains(project)) return false;

	int row = m_project_list.indexOf(project);
	if (removeRows(row, 1, QModelIndex())) {
		m_project_list.removeOne(project);
		disconnect(project->embeddedElementCollection(), &XmlElementCollection::elementAdded, this, &ElementsCollectionModel::elementIntegratedToCollection);
		connect(project->embeddedElementCollection(), &XmlElementCollection::elementChanged, this, &ElementsCollectionModel::updateItem);
		return true;
	}
	else
		return false;
}

/**
 * @brief ElementsCollectionModel::project
 * @return A list of project handled by this model
 */
QList<QETProject *> ElementsCollectionModel::project() const {
	return m_project_list;
}

/**
 * @brief ElementsCollectionModel::itemForProject
 * @param project
 * @return the root item of project @project, or nullptr if not found.
 */
XmlProjectElementCollectionItem *ElementsCollectionModel::itemForProject(QETProject *project)
{
	if (!m_project_list.contains(project)) return nullptr;
	QModelIndex index_ = index(m_project_list.indexOf(project), 0);
	if (!index_.isValid()) return nullptr;

	XmlProjectElementCollectionItem *xpeci = static_cast<XmlProjectElementCollectionItem *>(index_.internalPointer());
	if (xpeci)
		return xpeci;
	else
		return nullptr;
}

/**
 * @brief ElementsCollectionModel::elementAddedToEmbeddedCollection
 * When an element is added to embedded collection of a project,
 * this method create and display the new element
 * @param path -The path of the new element in the embedded collection of a project
 */
void ElementsCollectionModel::elementIntegratedToCollection (QString path)
{
	QObject *object = sender();
	XmlElementCollection *collection = static_cast<XmlElementCollection *> (object);
	if (!collection) return;

	QETProject *project = nullptr;

		//Get the owner project of the collection
	foreach (QETProject *prj, m_project_list) {
		if (prj->embeddedElementCollection() == collection) {
			project = prj;
		}
	}

	if (project) {
		XmlProjectElementCollectionItem *xpeci = itemForProject(project);
		if (!xpeci) return;

		QString collection_name;
		ElementCollectionItem *eci = xpeci->lastItemForPath(path, collection_name);
		if (!eci) return;

		int new_row = eci->rowForInsertItem(collection_name);
		if (new_row <= -1) return;
		QModelIndex parent_index = createIndex(eci->row(), 0, eci);
		beginInsertRows(parent_index, new_row, new_row);
		eci->insertNewItem(collection_name);
		endInsertRows();
	}
}

/**
 * @brief ElementsCollectionModel::updateItem
 * Update the item at path
 * @param path
 */
void ElementsCollectionModel::updateItem(QString path)
{
	QObject *object = sender();
	XmlElementCollection *collection = static_cast<XmlElementCollection *> (object);
	if (!collection) return;

	QETProject *project = nullptr;

		//Get the owner project of the collection
	foreach (QETProject *prj, m_project_list) {
		if (prj->embeddedElementCollection() == collection) {
			project = prj;
		}
	}

	if (project) {
		XmlProjectElementCollectionItem *xpeci = itemForProject(project);
		if (!xpeci) {
			return;
		}

		ElementCollectionItem *eci = xpeci->itemAtPath(path);
		if (!eci) {
			return;
		}

		eci->clearData();
	}
}

void ElementsCollectionModel::bir(ElementCollectionItem *eci, int first, int last)
{
	Q_UNUSED(eci);
	if (!m_parent_at_drop.isValid()) return;
	beginInsertRows(m_parent_at_drop, first, last);
}

void ElementsCollectionModel::brr(ElementCollectionItem *eci, int first, int last)
{
	Q_UNUSED(eci);
	if (!m_parent_at_drop.isValid()) return;
	beginRemoveRows(m_parent_at_drop, first, last);
}
