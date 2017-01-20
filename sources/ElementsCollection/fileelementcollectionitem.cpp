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

#include "fileelementcollectionitem.h"
#include "elementslocation.h"
#include "qetapp.h"
#include "qeticons.h"

#include <QDir>

/**
 * @brief FileElementCollectionItem::FileElementCollectionItem
 * Constructor
 */
FileElementCollectionItem::FileElementCollectionItem()
{}

/**
 * @brief FileElementCollectionItem::setRootPath
 * Set path has root path for this file item.
 * Use this function only to set the beginning of a file collection.
 * @param path
 * @return true if path exist.
 */
bool FileElementCollectionItem::setRootPath(QString path, bool set_data, bool hide_element)
{
	QDir dir(path);
	if (dir.exists()) {
		m_path = path;
		populate(set_data, hide_element);
		return true;
	}

	return false;
}

/**
 * @brief FileElementCollectionItem::fileSystemPath
 * @return the file system path of this item
 */
QString FileElementCollectionItem::fileSystemPath() const
{
	if (isCollectionRoot())
		return m_path;

	FileElementCollectionItem *feci = static_cast<FileElementCollectionItem *> (parent());
	if (feci)
		return feci->fileSystemPath() + "/" + m_path;
	else
		return QString();
}

/**
 * @brief FileElementCollectionItem::dirPath
 * @return the dir path of this item (if this item is a dir return the path,
 * if item is an element return the path of the parent directory)
 */
QString FileElementCollectionItem::dirPath() const
{
	if (isDir())
		return fileSystemPath();
	else if (parent() && parent()->type() == FileElementCollectionItem::Type)
		return static_cast<FileElementCollectionItem*>(parent())->fileSystemPath();
	else
		return QString();
}

/**
 * @brief FileElementCollectionItem::isDir
 * @return true if this item represent a directory
 */
bool FileElementCollectionItem::isDir() const
{
	if (m_path.endsWith(".elmt"))
		return false;
	else
		return true;

}

/**
 * @brief FileElementCollectionItem::isElement
 * @return true if this item represent an element
 */
bool FileElementCollectionItem::isElement() const
{
	return (!isDir());
}

/**
 * @brief FileElementCollectionItem::localName
 * @return the located name of this item
 */
QString FileElementCollectionItem::localName()
{
	if (!text().isNull())
		return text();

	else if (isDir()) {
		if (isCollectionRoot()) {
			if (m_path == QETApp::commonElementsDirN())
				setText(QObject::tr("Collection QET"));
			else if (m_path == QETApp::customElementsDirN())
				setText(QObject::tr("Collection utilisateur"));
			else
				setText(QObject::tr("Collection inconnue"));
		}
		else {
				//Open the qet_directory file, to get the traductions name of this dir
			QFile dir_conf(fileSystemPath() + "/qet_directory");

			if (dir_conf.exists() && dir_conf.open(QIODevice::ReadOnly | QIODevice::Text)) {

					//Get the content of the file
				QDomDocument document;
				if (document.setContent(&dir_conf)) {
					QDomElement root = document.documentElement();
					if (root.tagName() == "qet-directory") {
						NamesList nl;
						nl.fromXml(root);
						setText(nl.name());
					}
				}
			}
		}
	}
	else if (isElement()) {
		ElementsLocation loc(collectionPath());
		setText(loc.name());
	}

	return text();
}

/**
 * @brief FileElementCollectionItem::name
 * @return The collection name of this item
 */
QString FileElementCollectionItem::name() const
{
	if (isCollectionRoot())
		return QString();
	else
		return m_path;
}

/**
 * @brief FileElementCollectionItem::collectionPath
 * @return The path of this item relative to the collection.
 */
QString FileElementCollectionItem::collectionPath() const
{
	if (isCollectionRoot()) {
		if (m_path == QETApp::commonElementsDirN())
			return "common://";
		else
			return "custom://";
	}
	else if (parent() && parent()->type() == FileElementCollectionItem::Type) {
		ElementCollectionItem *eci = static_cast<ElementCollectionItem*>(parent());
		if (eci->isCollectionRoot())
			return eci->collectionPath() + m_path;
		else
			return eci->collectionPath() + "/" + m_path;
	}
	else
		return QString();
}

/**
 * @brief FileElementCollectionItem::isCollectionRoot
 * @return true if this item represent the root of collection
 */
bool FileElementCollectionItem::isCollectionRoot() const
{
	if (m_path == QETApp::commonElementsDirN() || m_path == QETApp::customElementsDirN())
		return true;
	else
		return false;
}

/**
 * @brief FileElementCollectionItem::isCommonCollection
 * @return True if this item represent the common collection
 */
bool FileElementCollectionItem::isCommonCollection() const
{
	return fileSystemPath().startsWith(QETApp::commonElementsDirN());
}

/**
 * @brief FileElementCollectionItem::isCustomCollection
 * @return True if this item represent the custom collection
 */
bool FileElementCollectionItem::isCustomCollection() const
{
	return fileSystemPath().startsWith(QETApp::customElementsDirN());
}

/**
 * @brief FileElementCollectionItem::addChildAtPath
 * Ask to this item item to add a child with collection name @collection_name
 * @param collection_name
 */
void FileElementCollectionItem::addChildAtPath(const QString &collection_name)
{
	if (collection_name.isEmpty())
		return;

	FileElementCollectionItem *feci = new FileElementCollectionItem();
	insertRow(rowForInsertItem(collection_name), feci);
	feci->setPathName(collection_name);
	feci->setUpData();
}

/**
 * @brief FileElementCollectionItem::setUpData
 * SetUp the data of this item
 */
void FileElementCollectionItem::setUpData()
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
 * @brief FileElementCollectionItem::setUpIcon
 * SetUp the icon of this item.
 * Because icon use several memory, we use this method for setup icon instead setUpData.
 */
void FileElementCollectionItem::setUpIcon()
{
	if (!icon().isNull())
		return;

	if (isCollectionRoot()) {
		if (m_path == QETApp::commonElementsDirN())
			setIcon(QIcon(":/ico/16x16/qet.png"));
		else
			setIcon(QIcon(":/ico/16x16/go-home.png"));
	}
	else {
		if (isDir())
			setIcon(QET::Icons::Folder);
		else {
			ElementsLocation loc(collectionPath());
			setIcon(loc.icon());
		}
	}
}

/**
 * @brief FileElementCollectionItem::setPathName
 * Set the name of this item in the file system path.
 * This item must have a parent, because they should be a child item of another.
 * For create a new file collection see setRootPath.
 * @param path_name
 */
void FileElementCollectionItem::setPathName(QString path_name, bool set_data, bool hide_element)
{
	m_path = path_name;

		//This isn't an element, we create the childs
	if (!path_name.endsWith(".elmt"))
		populate(set_data, hide_element);
}

/**
 * @brief FileElementCollectionItem::populate
 * Create the childs of this item
 * @param set_data : if true, call setUpData for every child of this item
 */
void FileElementCollectionItem::populate(bool set_data, bool hide_element)
{
	QDir dir (fileSystemPath());

		//Get all directory in this directory.
	foreach(QString str, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
	{
		FileElementCollectionItem *feci = new FileElementCollectionItem();
		appendRow(feci);
		feci->setPathName(str, set_data, hide_element);
		if (set_data)
			feci->setUpData();
	}

	if (hide_element)
		return;

		//Get all elmt file in this directory
	dir.setNameFilters(QStringList() << "*.elmt");
	foreach(QString str, dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name))
	{
		FileElementCollectionItem *feci = new FileElementCollectionItem();
		appendRow(feci);
		feci->setPathName(str, set_data);
		if (set_data)
			feci->setUpData();
	}
}
