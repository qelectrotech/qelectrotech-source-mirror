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
#include "xmlprojectelementcollectionitem.h"
#include "qetproject.h"
#include "qeticons.h"
#include "xmlelementcollection.h"
#include "nameslist.h"
#include "qetapp.h"
#include "elementlocation.h"
#include "elementcollectionhandler.h"
#include <algorithm>

/**
 * @brief XmlProjectElementCollectionItem::XmlProjectElementCollectionItem
 * Default constructor.
 * @param project -project for this item
 * @param parent -paretn item
 */
XmlProjectElementCollectionItem::XmlProjectElementCollectionItem(QETProject *project, ElementCollectionItem *parent) :
	ElementCollectionItem(parent),
	m_project(project)
{
	m_dom_element = project->embeddedElementCollection()->root();
	populate();
}

/**
 * @brief XmlProjectElementCollectionItem::XmlProjectElementCollectionItem
 * Private constructor
 * @param project -project for this item
 * @param dom_element: the dom_element must represent this item
 * @param parent
 */
XmlProjectElementCollectionItem::XmlProjectElementCollectionItem(QETProject *project, const QDomElement &dom_element, ElementCollectionItem *parent) :
	ElementCollectionItem(parent),
	m_project(project),
	m_dom_element(dom_element)
{
	populate();
}

/**
 * @brief XmlProjectElementCollectionItem::~XmlProjectElementCollectionItem
 */
XmlProjectElementCollectionItem::~XmlProjectElementCollectionItem()
{}

/**
 * @brief XmlProjectElementCollectionItem::data
 * The data used by the view who display this item through the model
 * @param column
 * @param role
 * @return
 */
QVariant XmlProjectElementCollectionItem::data(int column, int role)
{
	if (column > 0)
		return QVariant();

    switch (role)
    {
        case Qt::DisplayRole:
            return name();
            break;
        case Qt::DecorationRole:
			if (isCollectionRoot())
				return QIcon(QET::Icons::ProjectFileGP);
			else if (isDir())
				return QET::Icons::Folder;
			else
			{
				if (m_icon.isNull())
				{
					ElementLocation loc(embeddedPath(), m_project);
					m_icon = loc.icon();
				}
				return m_icon;
			}
            break;
		case Qt::ToolTipRole:
			if (isCollectionRoot())
				return m_project->filePath();
			else
				return collectionPath();
			break;
        default:
            return QVariant();
	}
}

/**
 * @brief XmlProjectElementCollectionItem::mimeData
 * @return The mimedata of this item
 */
QMimeData *XmlProjectElementCollectionItem::mimeData()
{
	QMimeData *mime_data = new QMimeData();
	mime_data->setText(collectionPath());

	if (isElement())
		mime_data->setData("application/x-qet-element-uri", collectionPath().toLatin1());
	else
		mime_data->setData("application/x-qet-category-uri", collectionPath().toLatin1());

	return mime_data;
}

/**
 * @brief XmlProjectElementCollectionItem::canDropMimeData
 * @param data
 * @param action
 * @param row
 * @param column
 * @return True if the data can be dropped
 */
bool XmlProjectElementCollectionItem::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column) const
{
	Q_UNUSED(action); Q_UNUSED(row); Q_UNUSED(column);

	if (data->hasFormat("application/x-qet-element-uri") || data->hasFormat("application/x-qet-category-uri"))
	{
			//Return false if user try to drop a item from a folder to the same folder
		ElementLocation drop_location(data->text());
		for (int i=0 ; i<childCount() ; i++)
		{
			if (static_cast<XmlProjectElementCollectionItem *>(child(i))->collectionPath() == drop_location.collectionPath())
				return false;
		}

		return true;
	}
	else
		return false;
}

/**
 * @brief XmlProjectElementCollectionItem::dropMimeData
 * @param data
 * @param action
 * @param row
 * @param column
 * @return handle a drop of a mime data
 */
bool XmlProjectElementCollectionItem::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column)
{
	Q_UNUSED(action); Q_UNUSED(row); Q_UNUSED(column);

	XmlProjectElementCollectionItem *xpeci = this;
	if (isElement() && parent() && parent()->type() == XmlProjectElementCollectionItem::Type)
		xpeci = static_cast<XmlProjectElementCollectionItem *>(parent());

		//before do the copy, we get all collection path of child,
		//for remove it if the copied item have the same path of an existing child.
		//We can't do this after the copy, because at the copy if the xml collection have a DomElement with the same path,
		//he was removed before the new xml DomElement is inserted
		//So the existing child of this will return a null QString when call collectionPath(), because the item
		//doesn't exist anymore in the xml collection.
	QList <QString> child_path_list;
	for (int i=0 ; i<childCount() ; i++)
		child_path_list.append(static_cast<XmlProjectElementCollectionItem *>(child(i))->collectionPath());

	ElementCollectionHandler ech;

	ElementLocation source(data->text());
	ElementLocation destination(xpeci->collectionPath());
	ElementLocation location = ech.copy(source, destination);

	if (location.exist())
	{
			//If this item have a child with the same path of location, we remove the existing child before insert new child
		for (int i=0 ; i<child_path_list.size() ; i++)
			if (child_path_list.at(i) == location.projectCollectionPath())
				removeChild(i,1);

		insertNewItem(location.fileName());
		return true;
	}

	return false;
}

/**
 * @brief XmlProjectElementCollectionItem::flags
 * @return The flags of this item
 */
Qt::ItemFlags XmlProjectElementCollectionItem::flags()
{
	if (isDir())
		return (Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
	else
		return (Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
}

/**
 * @brief XmlProjectElementCollectionItem::isCollectionRoot
 * @return True if this item represent the root collection of a project
 */
bool XmlProjectElementCollectionItem::isCollectionRoot() const
{
    if (!m_parent_item) return true;
    else if (m_parent_item->type() != XmlProjectElementCollectionItem::Type) return true;
    else return false;
}

/**
 * @brief XmlProjectElementCollectionItem::name
 * @return The name of this item, name is notably use for Qt::DisplayRole data
 */
QString XmlProjectElementCollectionItem::name()
{
    if (!m_name.isNull()) return m_name;

    if (isCollectionRoot())
    {
		if (m_project->title().isEmpty())
            return QString("Projet sans titre");
        else
			return m_project->title();
    }
	else
	{
		ElementLocation location (embeddedPath(), m_project);
		m_name = location.name();
		return m_name;
	}
}

/**
 * @brief XmlProjectElementCollectionItem::isValid
 * @return Always true
 */
bool XmlProjectElementCollectionItem::isValid() const {
	return true;
}

/**
 * @brief XmlProjectElementCollectionItem::project
 * @return The project for this collection item
 */
QETProject *XmlProjectElementCollectionItem::project() const {
	return m_project;
}

/**
 * @brief XmlProjectElementCollectionItem::isDir
 * @return true if this item represent a directory
 */
bool XmlProjectElementCollectionItem::isDir() const
{
	if (m_dom_element.tagName() == "category") return true;
	else return false;
}

/**
 * @brief XmlProjectElementCollectionItem::isElement
 * @return true if this item represent an element
 */
bool XmlProjectElementCollectionItem::isElement() const
{
	if (m_dom_element.tagName() == "element") return true;
	else return false;
}

/**
 * @brief XmlProjectElementCollectionItem::collectionPath
 * @return The collection path of this item.
 * The path is in form : project0+embed://dir/subdir/myElement.elmt
 */
QString XmlProjectElementCollectionItem::collectionPath() const
{
	ElementLocation loc (embeddedPath(), m_project);
	return loc.projectCollectionPath();
}

/**
 * @brief XmlProjectElementCollectionItem::embeddedPath
 * @return The embedde path of this item
 * The path is in form : embed://dir/subdir/myElement.elmt
 */
QString XmlProjectElementCollectionItem::embeddedPath() const
{
	if (isCollectionRoot())
	{
		return "embed://";
	}
	else
	{
		XmlProjectElementCollectionItem *parent = static_cast<XmlProjectElementCollectionItem *>(m_parent_item);

		if (parent->isCollectionRoot())
			return parent->embeddedPath() + collectionName();
		else
			return parent->embeddedPath() + "/" + collectionName();
	}
}

/**
 * @brief XmlProjectElementCollectionItem::collectionName
 * @return The collection name of this item
 */
QString XmlProjectElementCollectionItem::collectionName() const {
	return m_dom_element.attribute("name");
}

/**
 * @brief XmlProjectElementCollectionItem::insertNewItem
 * When this XmlProjectElementCollectionItem is already created, we must to use this method for insert a new item.
 * Befor use this, see rowForInsertItem and lastItemForPath
 * @param collection_name : the collection name to search in the child of QDomElement.
 */
void XmlProjectElementCollectionItem::insertNewItem(const QString &collection_name)
{
	if (collection_name.isEmpty()) return;

	QDomNodeList node_list;
	if (collection_name.endsWith(".elmt"))
		node_list = m_dom_element.elementsByTagName("element");
	else
		node_list = m_dom_element.elementsByTagName("category");

	QDomElement child_element;
	for(int i=0 ; i<node_list.count() ; i++)
	{
		QDomElement dom_elmt = node_list.at(i).toElement();
		if (dom_elmt.attribute("name") == collection_name)
		{
			child_element = dom_elmt;
			i = node_list.count();
		}
	}

	XmlProjectElementCollectionItem *xpeci = new XmlProjectElementCollectionItem(m_project, child_element, this);
	insertChild(rowForInsertItem(collection_name), xpeci);
}

/**
 * @brief XmlProjectElementCollectionItem::populate
 * Populate this item
 */
void XmlProjectElementCollectionItem::populate()
{
	QList <QDomElement> dom_category = m_project->embeddedElementCollection()->directories(m_dom_element);
	std::sort(dom_category.begin(), dom_category.end(), [](QDomElement a, QDomElement b){return (a.attribute("name") < b.attribute("name"));});

	foreach (QDomElement element, dom_category)
	{
		XmlProjectElementCollectionItem *xpeci = new XmlProjectElementCollectionItem(m_project, element, this);
		this->appendChild(xpeci);
	}

	QList <QDomElement> dom_elements = m_project->embeddedElementCollection()->elements(m_dom_element);
	std::sort(dom_elements.begin(), dom_elements.end(), [](QDomElement a, QDomElement b){return (a.attribute("name") < b.attribute("name"));});

	foreach (QDomElement element, dom_elements)
	{
		XmlProjectElementCollectionItem *xpeci = new XmlProjectElementCollectionItem(m_project, element, this);
		this->appendChild(xpeci);
	}
}
