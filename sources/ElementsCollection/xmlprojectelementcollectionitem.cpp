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
#include "qetproject.h"
#include "xmlelementcollection.h"
#include "qeticons.h"
#include "xmlprojectelementcollectionitem.h"

/**
 * @brief XmlProjectElementCollectionItem::XmlProjectElementCollectionItem
 * Constructor
 */
XmlProjectElementCollectionItem::XmlProjectElementCollectionItem()
{}

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
 * @brief XmlProjectElementCollectionItem::localName
 * @return the located name of this item
 */
QString XmlProjectElementCollectionItem::localName()
{
	if (!text().isNull())
		return text();

	if (isCollectionRoot()) {
		if (m_project->title().isEmpty())
			setText(QObject::tr("Projet sans titre"));
		else
			setText(m_project->title());
	}
	else {
		ElementsLocation location (embeddedPath(), m_project);
		setText(location.name());
	}

	return text();
}

/**
 * @brief XmlProjectElementCollectionItem::name
 * @return The collection name of this item
 */
QString XmlProjectElementCollectionItem::name() const
{
	return m_dom_element.attribute("name");
}

/**
 * @brief XmlProjectElementCollectionItem::collectionPath
 * @return The path of this item relative to the collection.
 */
QString XmlProjectElementCollectionItem::collectionPath() const
{
	ElementsLocation loc (embeddedPath(), m_project);
	QString p = loc.projectCollectionPath();
	if (p.isEmpty())
		p = QObject::tr("Collection");
	return p;
}

/**
 * @brief XmlProjectElementCollectionItem::embeddedPath
 * @return The embedde path of this item
 * The path is in form : embed://dir/subdir/myElement.elmt
 */
QString XmlProjectElementCollectionItem::embeddedPath() const
{
	if (isCollectionRoot())
		return "embed://";
	else if (parent()){
		XmlProjectElementCollectionItem *xpeci = static_cast<XmlProjectElementCollectionItem *>(parent());

		if (xpeci->isCollectionRoot())
			return xpeci->embeddedPath() + name();
		else
			return xpeci->embeddedPath() + "/" + name();
	}
	else
		return QString();
}

/**
 * @brief XmlProjectElementCollectionItem::isCollectionRoot
 * @return true if this item represent the root of collection
 */
bool XmlProjectElementCollectionItem::isCollectionRoot() const
{
	if (!parent())
		return true;
	else if (parent()->type() != XmlProjectElementCollectionItem::Type)
		return true;
	else
		return false;
}

/**
 * @brief XmlProjectElementCollectionItem::addChildAtPath
 * Ask to this item item to add a new child with collection name @collection_name
 * (the child must exist in the xml element collection)
 * @param collection_name : name of the child item to add.
 */
void XmlProjectElementCollectionItem::addChildAtPath(const QString &collection_name)
{
	if (collection_name.isEmpty())
		return;

	QString str (collection_name.endsWith(".elmt")? "element" : "category");
	QDomElement child_element = m_dom_element.firstChildElement(str);

	while (!child_element.isNull()) {
		if (child_element.attribute("name") == collection_name) {
			XmlProjectElementCollectionItem *xpeci = new XmlProjectElementCollectionItem ();
			insertRow(rowForInsertItem(collection_name), xpeci);
			xpeci->setXmlElement(child_element, m_project);
			xpeci->setUpData();
			return;
		}
		else
			child_element = child_element.nextSiblingElement(str);
	}
}

/**
 * @brief XmlProjectElementCollectionItem::project
 * @return the paretn project of the managed collection
 */
QETProject *XmlProjectElementCollectionItem::project() const
{
	return m_project;
}

/**
 * @brief XmlProjectElementCollectionItem::setProject
 * Set the project for this item.
 * Use this method for set this item the root of the collection
 * @param project : project to manage the collection
 * @param set_data : if true, call setUpData for every child of this item
 */
void XmlProjectElementCollectionItem::setProject(QETProject *project, bool set_data, bool hide_element)
{
	if (m_project)
		return;

	m_project = project;
	m_dom_element = project->embeddedElementCollection()->root();
	populate(set_data, hide_element);
}

/**
 * @brief XmlProjectElementCollectionItem::setUpData
 * SetUp the data of this item
 */
void XmlProjectElementCollectionItem::setUpData()
{
		//Setup the displayed name
	localName();

	if (isDir())
		setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
	else
		setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);

	setToolTip(collectionPath());
}

/**
 * @brief XmlProjectElementCollectionItem::setUpIcon
 * SetUp the icon of this item.
 * Because icon use several memory, we use this method for setup icon instead setUpData.
 */
void XmlProjectElementCollectionItem::setUpIcon()
{
	if (!icon().isNull())
		return;

	if (isCollectionRoot())
		setIcon(QET::Icons::ProjectFileGP);
	else if (isDir())
		setIcon(QET::Icons::Folder);
	else {
		ElementsLocation loc(embeddedPath(), m_project);
		setIcon(loc.icon());
	}
}

/**
 * @brief XmlProjectElementCollectionItem::populate
 * Create the childs of this item
 * @param set_data : if true, call setUpData for every child of this item
 */
void XmlProjectElementCollectionItem::populate(bool set_data, bool hide_element)
{
	QList <QDomElement> dom_category = m_project->embeddedElementCollection()->directories(m_dom_element);
	std::sort(dom_category.begin(), dom_category.end(), [](QDomElement a, QDomElement b){return (a.attribute("name") < b.attribute("name"));});

	foreach (QDomElement element, dom_category)
	{
		XmlProjectElementCollectionItem *xpeci = new XmlProjectElementCollectionItem();
		appendRow(xpeci);
		xpeci->setXmlElement(element, m_project, set_data, hide_element);
		if (set_data)
			xpeci->setUpData();
	}

	if (hide_element)
		return;

	QList <QDomElement> dom_elements = m_project->embeddedElementCollection()->elements(m_dom_element);
	std::sort(dom_elements.begin(), dom_elements.end(), [](QDomElement a, QDomElement b){return (a.attribute("name") < b.attribute("name"));});

	foreach (QDomElement element, dom_elements)
	{
		XmlProjectElementCollectionItem *xpeci = new XmlProjectElementCollectionItem();
		appendRow(xpeci);
		xpeci->setXmlElement(element, m_project, set_data);
		if (set_data)
			xpeci->setUpData();
	}
}

/**
 * @brief XmlProjectElementCollectionItem::setXmlElement
 * Set the managed content of this item
 * @param element : the dom element (directory or element), to be managed by this item
 * @param project : the parent project of managed collection
 * @param set_data : if true, call setUpData for every child of this item
 */
void XmlProjectElementCollectionItem::setXmlElement(QDomElement element, QETProject *project, bool set_data, bool hide_element)
{
	m_dom_element = element;
	m_project = project;
	populate(set_data, hide_element);
}
