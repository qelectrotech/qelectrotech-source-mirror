/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "fileelementcollectionitem.h"
#include "xmlprojectelementcollectionitem.h"
#include "qetapp.h"
#include "xmlelementcollection.h"
#include "qetproject.h"
#include "elementcollectionhandler.h"

#include <QtConcurrent>

/**
 * @brief ElementsCollectionModel::ElementsCollectionModel
 * Constructor
 * @param parent
 */
ElementsCollectionModel::ElementsCollectionModel(QObject *parent) :
	QStandardItemModel(parent)
{
}

/**
 * @brief ElementsCollectionModel::data
 * Reimplemented from QStandardItemModel
 * @param index
 * @param role
 * @return
 */
QVariant ElementsCollectionModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DecorationRole) {
		QStandardItem *item = itemFromIndex(index);

		if (item->type() == FileElementCollectionItem::Type)
			static_cast<FileElementCollectionItem*>(item)->setUpIcon();
		else if (item->type() == XmlProjectElementCollectionItem::Type)
			static_cast<XmlProjectElementCollectionItem*>(item)->setUpIcon();
	}

	return QStandardItemModel::data(index, role);
}

/**
 * @brief ElementsCollectionModel::mimeData
 * Reimplemented from QStandardItemModel
 * @param indexes
 * @return
 */
QMimeData *ElementsCollectionModel::mimeData(const QModelIndexList &indexes) const
{
	QModelIndex index = indexes.first();
	if (index.isValid())
	{
		ElementCollectionItem *item = static_cast<ElementCollectionItem*>(itemFromIndex(index));

		QMimeData *mime_data = new QMimeData();
		mime_data->setText(item->collectionPath());

		if (item->isElement())
			mime_data->setData("application/x-qet-element-uri", item->collectionPath().toLatin1());
		else
			mime_data->setData("application/x-qet-category-uri", item->collectionPath().toLatin1());

		return mime_data;
	}
	else
		return new QMimeData();
}

/**
 * @brief ElementsCollectionModel::mimeTypes
 * Reimplemented from QStandardItemModel
 * @return
 */
QStringList ElementsCollectionModel::mimeTypes() const
{
	QStringList mime_list = QAbstractItemModel::mimeTypes();
	mime_list << "application/x-qet-element-uri" << "application/x-qet-category-uri";
	return mime_list;
}

/**
 * @brief ElementsCollectionModel::canDropMimeData
 * Reimplemented from QStandardItemModel
 * @param data
 * @param action
 * @param row
 * @param column
 * @param parent
 * @return
 */
bool ElementsCollectionModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
	if (!(QStandardItemModel::canDropMimeData(data, action, row, column, parent) && parent.isValid()))
		return false;

	QStandardItem *qsi = itemFromIndex(parent.child(row, column));
	if (!qsi)
		qsi = itemFromIndex(parent);

		//Drop in the common collection is forbiden
	if (qsi->type() == FileElementCollectionItem::Type)
		if (static_cast<FileElementCollectionItem *>(qsi)->isCommonCollection())
			return false;

	ElementCollectionItem *eci = static_cast<ElementCollectionItem *>(qsi);

	if (data->hasFormat("application/x-qet-element-uri") || data->hasFormat("application/x-qet-category-uri")) {
		return true;
			//Return false if user try to drop a item from a folder to the same folder
		ElementsLocation drop_location(data->text());
		for (int i=0 ; i<eci->rowCount() ; i++)
			if (static_cast<ElementCollectionItem *>(eci->child(i))->collectionPath() == drop_location.collectionPath())
				return false;

		return true;
	}
	else
		return false;
}

/**
 * @brief ElementsCollectionModel::dropMimeData
 * Reimplemented from QStandardItemModel
 * @param data
 * @param action
 * @param row
 * @param column
 * @param parent
 * @return
 */
bool ElementsCollectionModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	Q_UNUSED(action);

	QStandardItem *qsi = itemFromIndex(parent.child(row, column));
	if (!qsi)
		qsi = itemFromIndex(parent);

	if (qsi->type() == FileElementCollectionItem::Type) {
		FileElementCollectionItem *feci = static_cast<FileElementCollectionItem *>(qsi);

		if (feci->isCommonCollection())
			return false;

		if (feci->isElement() && feci->parent() && feci->parent()->type() == FileElementCollectionItem::Type)
			feci = static_cast<FileElementCollectionItem *>(feci->parent());

		ElementCollectionHandler ech;

		ElementsLocation source(data->text());
		ElementsLocation destination(feci->fileSystemPath());
		ElementsLocation location = ech.copy(source, destination);

		if (location.exist())
		{
				//If feci have a child with the same path of location,
				//we remove the existing child befor insert new child
			for (int i=0 ; i<feci->rowCount() ; i++) {
				if (static_cast<FileElementCollectionItem *>(feci->child(i))->collectionPath() == location.collectionPath())
					feci->removeRow(i);
			}
			feci->addChildAtPath(location.fileName());
			return true;
		}

		return false;
	}
	else if (qsi->type() == XmlProjectElementCollectionItem::Type) {
		XmlProjectElementCollectionItem *xpeci = static_cast<XmlProjectElementCollectionItem*>(qsi);

		if (xpeci->isElement() && xpeci->parent() && xpeci->parent()->type() == XmlProjectElementCollectionItem::Type)
			xpeci = static_cast<XmlProjectElementCollectionItem *>(xpeci->parent());

			//before do the copy, we get all collection path of xpeci child,
			//for remove it if the copied item have the same path of an existing child.
			//We can't do this after the copy, because at the copy if the xml collection have a DomElement with the same path,
			//he was removed before the new xml DomElement is inserted
			//So the existing child of this will return a null QString when call collectionPath(), because the item
			//doesn't exist anymore in the xml collection.
		QList <QString> child_path_list;
		for (int i=0 ; i<xpeci->rowCount() ; i++)
			child_path_list.append(static_cast<XmlProjectElementCollectionItem *>(xpeci->child(i, 0))->collectionPath());

		ElementCollectionHandler ech;

		ElementsLocation source(data->text());
		ElementsLocation destination(xpeci->collectionPath());
		ElementsLocation location = ech.copy(source, destination);

		return location.exist();
	}

	return false;
}

/**
 * @brief ElementsCollectionModel::loadCollections
 * Load the several collections in this model.
 * Prefer use this method instead of addCommonCollection, addCustomCollection and addProject,
 * because it use multithreading to speed up the loading.
 * This method emit loadingMaxValue(int) for know the maximum progress value
 * This method emit loadingProgressValue(int) for know the current progress value
 * @param common_collection : true for load the common collection
 * @param custom_collection : true for load the custom collection
 * @param projects : list of projects to load
 */
void ElementsCollectionModel::loadCollections(bool common_collection, bool custom_collection, QList<QETProject *> projects)
{
	if (common_collection)
		addCommonCollection(false);
	if (custom_collection)
		addCustomCollection(false);

	foreach (QETProject *project, projects)
		addProject(project, false);

	QList <ElementCollectionItem *> list = items();
	QFuture<void> futur = QtConcurrent::map(list, setUpData);
	emit loadingMaxValue(futur.progressMaximum());
	while (futur.isRunning()) {
		emit loadingProgressValue(futur.progressValue());
	}
}

/**
 * @brief ElementsCollectionModel::addCommonCollection
 * Add the common elements collection to this model
 */
void ElementsCollectionModel::addCommonCollection(bool set_data)
{
	FileElementCollectionItem *feci = new FileElementCollectionItem();
	if (feci->setRootPath(QETApp::commonElementsDirN(), set_data, m_hide_element)) {
		invisibleRootItem()->appendRow(feci);
		if (set_data)
			feci->setUpData();
	}
	else
		delete feci;
}

/**
 * @brief ElementsCollectionModel::addCustomCollection
 * Add the custom elements collection to this model
 */
void ElementsCollectionModel::addCustomCollection(bool set_data)
{
	FileElementCollectionItem *feci = new FileElementCollectionItem();
	if (feci->setRootPath(QETApp::customElementsDirN(), set_data, m_hide_element)) {
		invisibleRootItem()->appendRow(feci);
		if (set_data)
			feci->setUpData();
	}
	else
		delete feci;
}

/**
 * @brief ElementsCollectionModel::addLocation
 * Add the element or directory to this model.
 * If the location is already managed by this model, do nothing.
 * @param location
 */
void ElementsCollectionModel::addLocation(ElementsLocation location)
{
	QModelIndex index = indexFromLocation(location);
	if (index.isValid())
		return;

	ElementCollectionItem *last_item = nullptr;
	QString collection_name;

	if (location.isProject()) {
		QETProject *project = location.project();

		if (project) {
			XmlProjectElementCollectionItem *xpeci = m_project_hash.value(project);

			last_item = xpeci->lastItemForPath(location.collectionPath(false), collection_name);
		}
	}
	else if (location.isCustomCollection()) {
		QList <ElementCollectionItem *> child_list;

		for (int i=0 ; i<rowCount() ; i++)
			child_list.append(static_cast<ElementCollectionItem *>(item(i)));

		foreach(ElementCollectionItem *eci, child_list) {

			if (eci->type() == FileElementCollectionItem::Type) {
				FileElementCollectionItem *feci = static_cast<FileElementCollectionItem *>(eci);

				if (feci->isCustomCollection()) {
					last_item = feci->lastItemForPath(location.collectionPath(false), collection_name);
					if(last_item)
						break;
				}
			}
		}
	}

	if (last_item)
		last_item->addChildAtPath(collection_name);
}

/**
 * @brief ElementsCollectionModel::addProject
 * Add project to this model
 * @param project : project to add.
 * @param set_data : if true, setUpData is called for every ElementCollectionItem of project
 */
void ElementsCollectionModel::addProject(QETProject *project, bool set_data)
{
	if (m_project_list.contains(project))
		return;

	m_project_list.append(project);
	int row = m_project_list.indexOf(project);
	XmlProjectElementCollectionItem *xpeci = new XmlProjectElementCollectionItem();
	m_project_hash.insert(project, xpeci);

	xpeci->setProject(project, set_data);
	insertRow(row, xpeci);
	if (set_data)
		xpeci->setUpData();
	connect(project->embeddedElementCollection(), &XmlElementCollection::elementAdded, this, &ElementsCollectionModel::elementIntegratedToCollection);
	connect(project->embeddedElementCollection(), &XmlElementCollection::elementChanged, this, &ElementsCollectionModel::updateItem);
	connect(project->embeddedElementCollection(), &XmlElementCollection::elementRemoved, this, &ElementsCollectionModel::itemRemovedFromCollection);
	connect(project->embeddedElementCollection(), &XmlElementCollection::directoryRemoved, this, &ElementsCollectionModel::itemRemovedFromCollection);
}

/**
 * @brief ElementsCollectionModel::removeProject
 * Remove project from this model
 * @param project
 */
void ElementsCollectionModel::removeProject(QETProject *project)
{
	if (!m_project_list.contains(project))
		return;

	int row = m_project_list.indexOf(project);
	if (removeRows(row, 1, QModelIndex())) {
		m_project_list.removeOne(project);
		m_project_hash.remove(project);
		disconnect(project->embeddedElementCollection(), &XmlElementCollection::elementAdded, this, &ElementsCollectionModel::elementIntegratedToCollection);
		disconnect(project->embeddedElementCollection(), &XmlElementCollection::elementChanged, this, &ElementsCollectionModel::updateItem);
		disconnect(project->embeddedElementCollection(), &XmlElementCollection::elementRemoved, this, &ElementsCollectionModel::itemRemovedFromCollection);
		disconnect(project->embeddedElementCollection(), &XmlElementCollection::directoryRemoved, this, &ElementsCollectionModel::itemRemovedFromCollection);
	}
}

/**
 * @brief ElementsCollectionModel::project
 * @return every project added to this model
 */
QList<QETProject *> ElementsCollectionModel::project() const
{
	return m_project_list;
}

/**
 * @brief ElementsCollectionModel::highlightUnusedElement
 * Highlight every unused element of managed project.
 * @See QETProject::unusedElements()
 */
void ElementsCollectionModel::highlightUnusedElement()
{
	QList <ElementsLocation> unused;

	foreach (QETProject *project, m_project_list)
		unused.append(project->unusedElements());

	QBrush brush;
	brush.setStyle(Qt::Dense4Pattern);
	brush.setColor(Qt::red);

	foreach (ElementsLocation location, unused) {
		QModelIndex index = indexFromLocation(location);
		if (index.isValid()) {
			QStandardItem *qsi = itemFromIndex(index);
			if (qsi)
				qsi->setBackground(brush);
		}
	}
}

/**
 * @brief ElementsCollectionModel::items
 * @return every ElementCollectionItem owned by this model
 */
QList <ElementCollectionItem *> ElementsCollectionModel::items() const
{
	QList <ElementCollectionItem *> list;

	for (int i=0 ; i<rowCount() ; i++) {
		ElementCollectionItem *eci = static_cast<ElementCollectionItem *>(item(i));
		list.append(eci);
		list.append(eci->items());
	}

	return list;
}

/**
 * @brief ElementsCollectionModel::hideElement
 * Hide element in this model, only directory is managed
 */
void ElementsCollectionModel::hideElement()
{
	m_hide_element = true;
	foreach(ElementCollectionItem *eci, items()) {
		if (eci->isElement()) {
			removeRow(eci->row(), indexFromItem(eci).parent());
		}
	}
}

/**
 * @brief ElementsCollectionModel::indexFromLocation
 * Return the index who represent @location.
 * Index can be non valid
 * @param location
 * @return
 */
QModelIndex ElementsCollectionModel::indexFromLocation(const ElementsLocation &location)
{
	QList <ElementCollectionItem *> child_list;

	for (int i=0 ; i<rowCount() ; i++)
		child_list.append(static_cast<ElementCollectionItem *>(item(i)));

		foreach(ElementCollectionItem *eci, child_list) {

			ElementCollectionItem *match_eci = nullptr;

			if (eci->type() == FileElementCollectionItem::Type) {
				if (FileElementCollectionItem *feci = static_cast<FileElementCollectionItem *>(eci)) {
					if ( (location.isCommonCollection() && feci->isCommonCollection()) ||
						 (location.isCustomCollection() && !feci->isCommonCollection()) ) {
						match_eci = feci->itemAtPath(location.collectionPath(false));
					}
				}
			}
			else if (eci->type() == XmlProjectElementCollectionItem::Type) {
				if (XmlProjectElementCollectionItem *xpeci = static_cast<XmlProjectElementCollectionItem *>(eci)) {
					match_eci = xpeci->itemAtPath(location.collectionPath(false));
				}
			}

			if (match_eci)
				return indexFromItem(match_eci);
		}

		return QModelIndex();
}

/**
 * @brief ElementsCollectionModel::elementIntegratedToCollection
 * When an element is added to embedded collection of a project,
 * this method create and display the new element
 * @param path -The path of the new element in the embedded collection of a project
 */
void ElementsCollectionModel::elementIntegratedToCollection(QString path)
{
	QObject *object = sender();
	XmlElementCollection *collection = static_cast<XmlElementCollection *> (object);
	if (!collection)
		return;

	QETProject *project = nullptr;

		//Get the owner project of the collection
	foreach (QETProject *prj, m_project_list) {
		if (prj->embeddedElementCollection() == collection) {
			project = prj;
		}
	}

	if (project) {
		XmlProjectElementCollectionItem *xpeci = m_project_hash.value(project);

		QString collection_name;
		ElementCollectionItem *eci = xpeci->lastItemForPath(path, collection_name);
		if (!eci)
			return;

		eci->addChildAtPath(collection_name);
	}
}

/**
 * @brief ElementsCollectionModel::itemRemovedFromCollection
 * This method must be called by a signal, to get a sender.
 * @param path
 */
void ElementsCollectionModel::itemRemovedFromCollection(QString path)
{
	QObject *object = sender();
	XmlElementCollection *collection = static_cast<XmlElementCollection *> (object);
	if (!collection)
		return;

	QETProject *project = nullptr;

		//Get the owner project of the collection
	foreach (QETProject *prj, m_project_list) {
		if (prj->embeddedElementCollection() == collection) {
			project = prj;
		}
	}

	if (project) {
		QModelIndex index = indexFromLocation(ElementsLocation(path, project));
		if (index.isValid())
			removeRow(index.row(), index.parent());
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
	if (!collection)
		return;

	QETProject *project = nullptr;

		//Get the owner project of the collection
	foreach (QETProject *prj, m_project_list) {
		if (prj->embeddedElementCollection() == collection) {
			project = prj;
		}
	}

	if (project) {
		ElementCollectionItem *eci = m_project_hash.value(project)->itemAtPath(path);
		if (!eci)
			return;

		eci->clearData();
		eci->setUpData();
	}
}
