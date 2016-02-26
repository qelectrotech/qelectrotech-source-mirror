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
#include "fileelementcollectionitem.h"
#include "qetapp.h"
#include "elementslocation.h"
#include "nameslist.h"
#include "qeticons.h"
#include "elementcollectionhandler.h"

/**
 * @brief FileElementCollectionItem::FileElementCollectionItem
 * Default constructor
 * @param parent : parent item of this item
 */
FileElementCollectionItem::FileElementCollectionItem(ElementCollectionItem *parent) :
    ElementCollectionItem(parent)
{}

/**
 * @brief FileElementCollectionItem::~FileElementCollectionItem
 * Destructor
 */
FileElementCollectionItem::~FileElementCollectionItem()
{}

/**
 * @brief FileElementCollectionItem::setRootPath
 * Set path has root path for this file item.
 * Use this function only to set the beginning of a file collection.
 * @param path
 * @return true if path exist.
 */
bool FileElementCollectionItem::setRootPath(QString path)
{
    QDir dir(path);

    if (dir.exists())
	{
		m_path = path;
		populate();
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
        //Parent must be a file element collection item
	if (!m_parent_item || m_parent_item->type() != FileElementCollectionItem::Type)
		return m_path;

    FileElementCollectionItem *parent = static_cast<FileElementCollectionItem*>(m_parent_item);

        //Get the path of the parent.
//	if (parent->isCollectionRoot())
//		return parent->fileSystemPath() + m_path;
//	else
//		return parent->fileSystemPath() + "/" + m_path;
	return parent->fileSystemPath() + "/" + m_path;
}

/**
 * @brief FileElementCollectionItem::dirPath
 * @return the dir path of this item
 */
QString FileElementCollectionItem::dirPath() const
{
	if (isDir())
		return fileSystemPath();

		//Parent must be a file element collection item
	if (m_parent_item->type() != FileElementCollectionItem::Type) return QString();

	FileElementCollectionItem *parent = static_cast<FileElementCollectionItem*>(m_parent_item);
		//Get the path of the parent.
	return parent->fileSystemPath();
}

/**
 * @brief FileElementCollectionItem::collectionPath
 * @return The path of this item relative to the collection.
 */
QString FileElementCollectionItem::collectionPath() const
{
		//Parent must be a file element collection item
		//else this item is the root of collection path.
	if (!m_parent_item || m_parent_item->type() != FileElementCollectionItem::Type)
	{
		if (m_path == QETApp::commonElementsDirN())
			return "common://";
		else
			return "custom://";
	}
	else if (m_parent_item->type() == FileElementCollectionItem::Type)
	{
		FileElementCollectionItem *feci = static_cast<FileElementCollectionItem*>(m_parent_item);
		if (feci->isCollectionRoot())
			return feci->collectionPath() + m_path;
		else
			return feci->collectionPath() + "/" + m_path;
	}
	else
		return QString();
}

/**
 * @brief FileElementCollectionItem::collectionName
 * @return The collection name of this item
 */
QString FileElementCollectionItem::collectionName() const
{
	if (isCollectionRoot()) return QString();
	else return m_path;
}

/**
 * @brief FileElementCollectionItem::data
 * @param column
 * @param role
 * @return the item data at column and role
 */
QVariant FileElementCollectionItem::data(int column, int role)
{
		//element collection have only one column
	if (column > 0)
		return QVariant();

	switch (role)
	{
		case Qt::DisplayRole: {
			return name();
		}
			break;
		case Qt::DecorationRole:
		{
				//This item have no parent or parent isn't a file element, so it is the root of a collection
			if (!m_parent_item || m_parent_item->type() != FileElementCollectionItem::Type)
			{
				if (m_path == QETApp::commonElementsDirN())
					return QIcon(":/ico/16x16/qet.png");
				else
					return QIcon(":/ico/16x16/go-home.png");
			}

			if (isDir())
				return QET::Icons::Folder;
			else if (isElement())
			{
				if (m_icon.isNull())
				{
					ElementLocation loc(collectionPath());
					m_icon = loc.icon();
				}
				return m_icon;
			}
		}
		case Qt::ToolTipRole:
			return collectionPath();
			break;
		default:
			return ElementCollectionItem::data(column, role);
			break;
	}
}

/**
 * @brief FileElementCollectionItem::mimeData
 * @return the mime data of this item
 */
QMimeData *FileElementCollectionItem::mimeData()
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
 * @brief FileElementCollectionItem::canDropMimeData
 * @param data
 * @param action
 * @param column
 * @return True if the data can be dropped
 */
bool FileElementCollectionItem::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column) const
{
	Q_UNUSED(action); Q_UNUSED(row); Q_UNUSED(column);
	if (isCommonCollection()) return false;

	if (data->hasFormat("application/x-qet-element-uri") || data->hasFormat("application/x-qet-category-uri"))
	{
			//Return false if user try to drop a item from a folder to the same folder
		ElementLocation drop_location(data->text());
		for (int i=0 ; i<childCount() ; i++)
		{
			if (static_cast<FileElementCollectionItem *>(child(i))->collectionPath() == drop_location.collectionPath())
				return false;
		}

		return true;
	}
	else
		return false;
}

/**
 * @brief FileElementCollectionItem::dropMimeData
 * @param data
 * @param action
 * @param column
 * @return Handle the drop of a data
 */
bool FileElementCollectionItem::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column)
{
	Q_UNUSED(action); Q_UNUSED(row); Q_UNUSED(column);
	if (isCommonCollection()) return false;

	FileElementCollectionItem *feci = this;
	if (isElement() && parent() && parent()->type() == FileElementCollectionItem::Type)
		feci = static_cast<FileElementCollectionItem *>(parent());

	ElementCollectionHandler ech;

	ElementLocation source(data->text());
	ElementLocation destination(feci->fileSystemPath());
	ElementLocation location = ech.copy(source, destination);

	if (location.exist())
	{
			//If this item have a child with the same path of location,
			//we remove the existing child befor insert new child
		for (int i=0 ; i<childCount() ; i++)
			if (static_cast<FileElementCollectionItem *>(child(i))->collectionPath() == location.collectionPath())
				removeChild(i, 1);

		insertNewItem(location.fileName());
		return true;
	}

	return false;
}

/**
 * @brief FileElementCollectionItem::flags
 * @return the flags of this item
 */
Qt::ItemFlags FileElementCollectionItem::flags()
{
	if (isDir())
		return (Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
	else
		return (Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
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
bool FileElementCollectionItem::isElement() const {
	return (!isDir());
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
 * @return True if this item is part of the common element collection item
 */
bool FileElementCollectionItem::isCommonCollection() const {
	return fileSystemPath().startsWith(QETApp::commonElementsDirN());
}

/**
 * @brief FileElementCollectionItem::isValid
 * @return
 */
bool FileElementCollectionItem::isValid() const
{
	return true;
	if (m_path.isEmpty())
		return false;
	else
		return true;
}

/**
 * @brief FileElementCollectionItem::name
 * @return the located name of this item
 */
QString FileElementCollectionItem::name()
{
	if (!m_name.isNull()) return m_name;

	else if (isDir())
	{
		if (isCollectionRoot())
		{
			if (m_path == QETApp::commonElementsDirN())
				m_name = QObject::tr("Collection QET");
			else if (m_path == QETApp::customElementsDirN())
				m_name = QObject::tr("Collection utilisateur");
			else
				m_name = QObject::tr("Collection inconnue");
		}
		else
		{
				//Open the qet_directory file, to get the traductions name of this dir
			QFile dir_conf(fileSystemPath() + "/qet_directory");
			if (!dir_conf.exists())
				m_name = QString("");

			if (!dir_conf.open(QIODevice::ReadOnly | QIODevice::Text))
				m_name = QString("");

				//Get the content of the file
			QDomDocument document;
			if (!document.setContent(&dir_conf))
				m_name = QString("");

			QDomElement root = document.documentElement();
			if (root.tagName() != "qet-directory")
				m_name = QString("");

				//Return the name for the current langage.
			NamesList nl;
			nl.fromXml(root);
			m_name = nl.name();
		}
	}
	else if (isElement())
	{		
		ElementLocation loc(collectionPath());
		m_name = loc.name();
	}
	return m_name;
}

/**
 * @brief FileElementCollectionItem::canRemoveContent
 * Reimplemented from ElementCollectionItem
 * @return
 */
bool FileElementCollectionItem::canRemoveContent()
{
	if (isCommonCollection()) return false;
	else if (isDir() && isCollectionRoot()) return false;
	else return true;
}

/**
 * @brief FileElementCollectionItem::removeContent
 * Reimplemented from ElementCollectionItem
 * @return
 */
bool FileElementCollectionItem::removeContent()
{
	if (!canRemoveContent()) return false;

	if (isElement())
	{
		QFile file(fileSystemPath());
		return file.remove();
	}
	else if (isDir() && !isCollectionRoot())
	{
		QDir dir (fileSystemPath());
		return dir.removeRecursively();
	}
	return false;
}

void FileElementCollectionItem::insertNewItem(const QString &collection_name)
{
	if (collection_name.isEmpty()) return;

	FileElementCollectionItem *feci = new FileElementCollectionItem(this);
	feci->setPathName(collection_name);
	insertChild(rowForInsertItem(collection_name), feci);
}

/**
 * @brief FileElementCollectionItem::setPathName
 * Set the name of this item in the file system path.
 * This item must have a parent, because they should be a child item of another.
 * For create a new file collection see setRootPath.
 * @param path_name
 */
void FileElementCollectionItem::setPathName(QString path_name)
{
	if (!m_parent_item) return;

	m_path = path_name;

		//This isn't an element, we create the childs
	if (!path_name.endsWith(".elmt"))
		populate();
}

/**
 * @brief FileElementCollectionItem::populate
 * Item populate itself with childs found in the system path.
 */
void FileElementCollectionItem::populate()
{
	QDir dir (fileSystemPath());

		//Get all directory in this directory.
	foreach(QString str, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
	{
		FileElementCollectionItem *feci = new FileElementCollectionItem(this);
		feci->setPathName(str);
		appendChild(feci);
	}

		//Get all elmt file in this directory
	dir.setNameFilters(QStringList() << "*.elmt");
	foreach(QString str, dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name))
	{
		FileElementCollectionItem *feci = new FileElementCollectionItem(this);
		feci->setPathName(str);
		appendChild(feci);
	}
}
