/*
		Copyright 2006-2026 The QElectroTech Team
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

#include "../qetapp.h"
#include "../qeticons.h"
#include "elementslocation.h"

#include <QDir>
#include <QMetaObject>
#include <QStandardItemModel>

/**
	@brief FileElementCollectionItem::FileElementCollectionItem
	Constructor
*/
FileElementCollectionItem::FileElementCollectionItem()
{}

/**
	@brief FileElementCollectionItem::setRootPath
	Set path has root path for this file item.
	Use this function only to set the beginning of a file collection.
	@param path
	@param set_data
	@param hide_element
	@return true if path exist.
*/
bool FileElementCollectionItem::setRootPath(const QString& path,
						bool set_data,
						bool hide_element)
{
	QDir dir(path);
	if (dir.exists())
	{
		m_path = path;
		populate(set_data, hide_element);
		return true;
	}

	return false;
}

/**
	@brief FileElementCollectionItem::fileSystemPath
	@return the file system path of this item
*/
QString FileElementCollectionItem::fileSystemPath() const
{
	if (isCollectionRoot())
		return m_path;

	FileElementCollectionItem *feci =
			static_cast<FileElementCollectionItem *> (parent());
	if (feci)
		return feci->fileSystemPath() % "/" % m_path;
	else
		return QString();//Null string
}

/**
	@brief FileElementCollectionItem::dirPath
	@return the dir path of this item (if this item is a dir return the path,
	if item is an element return the path of the parent directory)
*/
QString FileElementCollectionItem::dirPath() const
{
	if (isDir())
		return fileSystemPath();
	else if (parent() && parent()->type() == FileElementCollectionItem::Type)
		return static_cast<FileElementCollectionItem*>(parent())->fileSystemPath();
	else
		return QString();//Null string
}

/**
	@brief FileElementCollectionItem::isDir
	@return true if this item represent a directory
*/
bool FileElementCollectionItem::isDir() const
{
	if (m_path.endsWith(".elmt") || m_path.endsWith(".qetmak"))
		return false;
	else
		return true;

}

/**
	@brief FileElementCollectionItem::isElement
	@return true if this item represent an element
*/
bool FileElementCollectionItem::isElement() const
{
	return (!isDir());
}

/**
 * @brief FileElementCollectionItem::computeDisplayName
 * Compute the display name without calling setText() — safe to call from
 * any thread.  localName() and setUpData() both delegate to this.
 */
QString FileElementCollectionItem::computeDisplayName() const
{
	if (isCollectionRoot()) {
		QString macrosPath = QETApp::userMacrosDir();
		if (macrosPath.endsWith('/')) macrosPath.chop(1);

		if (m_path == QETApp::commonElementsDirN())
			return QObject::tr("Collection QET");
		if (m_path == QETApp::companyElementsDirN())
			return QObject::tr("Collection Company");
		if (m_path == QETApp::customElementsDirN())
			return QObject::tr("Collection utilisateur");
		if (m_path == macrosPath)
			return QObject::tr("Makros");
		return QObject::tr("Collection inconnue");
	}
	if (isDir()) {
		const QString str = fileSystemPath() % "/qet_directory";
		pugi::xml_document docu;
		if (docu.load_file(str.toStdString().c_str())) {
			if (QString(docu.document_element().name()) == "qet-directory") {
				NamesList nl;
				nl.fromXml(docu.document_element());
				return nl.name();
			}
		}
		return {};
	}
	// Element
	ElementsLocation loc(collectionPath());
	QString name = loc.name();
	if (name.endsWith(".qetmak"))
		name.remove(".qetmak");
	return name;
}

/**
 * @brief FileElementCollectionItem::localName
 * @return the located name of this item
 */
QString FileElementCollectionItem::localName()
{
	if (!text().isNull())
		return text();
	setText(computeDisplayName());
	return text();
}

/**
	@brief FileElementCollectionItem::localName
	Surcharged method, unlike the default method,
	avoid to create an elementLocation and so, gain time.
	@param location
	@return
*/
QString FileElementCollectionItem::localName(const ElementsLocation &location)
{
	if (!text().isNull())
		return text();

	if (isDir()) {
		setText(computeDisplayName());
	} else {
		QString display_name = location.name();
		if (display_name.endsWith(".qetmak"))
			display_name.remove(".qetmak");
		setText(display_name);
	}

	return text();
}

/**
	@brief FileElementCollectionItem::name
	@return The collection name of this item
*/
QString FileElementCollectionItem::name() const
{
	if (isCollectionRoot())
		return QString();
	else
		return m_path;
}

/**
	@brief FileElementCollectionItem::collectionPath
	@return The path of this item relative to the collection.
*/
QString FileElementCollectionItem::collectionPath() const
{
	if (isCollectionRoot()) {
		QString macrosPath = QETApp::userMacrosDir();
		if (macrosPath.endsWith("/")) macrosPath.remove(macrosPath.length() - 1, 1);

		if (m_path == QETApp::commonElementsDirN())
			return "common://";
		else if (m_path == QETApp::companyElementsDirN())
			return "company://";
		else if (m_path == macrosPath)
			return "macros://"; //
			else
				return "custom://";
	}
	else if (parent() && parent()->type()
		== FileElementCollectionItem::Type) {
		ElementCollectionItem *eci =
		static_cast<ElementCollectionItem*>(parent());
	if (eci->isCollectionRoot())
		return eci->collectionPath() + m_path;
		else
			return eci->collectionPath() % "/" % m_path;
		}
		else
			return QString();
}

/**
	@brief FileElementCollectionItem::isCollectionRoot
	@return true if this item represent the root of collection
*/
bool FileElementCollectionItem::isCollectionRoot() const
{
	QString macrosPath = QETApp::userMacrosDir();
	if (macrosPath.endsWith("/")) macrosPath.remove(macrosPath.length() - 1, 1);

	if (m_path == QETApp::commonElementsDirN()
		|| m_path == QETApp::companyElementsDirN()
		|| m_path == QETApp::customElementsDirN()
		|| m_path == macrosPath)
	return true;
	else
		return false;
}

/**
	@brief FileElementCollectionItem::isCommonCollection
	@return True if this item represent the common collection
*/
bool FileElementCollectionItem::isCommonCollection() const
{
	return fileSystemPath().startsWith(QETApp::commonElementsDirN());
}

/**
	@brief FileElementCollectionItem::isCompanyCollection
	@return True if this item represent the company collection
*/
bool FileElementCollectionItem::isCompanyCollection() const
{
	return fileSystemPath().startsWith(QETApp::companyElementsDirN());
}

/**
	@brief FileElementCollectionItem::isCustomCollection
	@return True if this item represent the custom collection
*/
bool FileElementCollectionItem::isCustomCollection() const
{
	return fileSystemPath().startsWith(QETApp::customElementsDirN());
}

/**
	@brief FileElementCollectionItem::addChildAtPath
	Ask to this item item to add a child
	with collection name collection_name
	@param collection_name
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
 *   @brief FileElementCollectionItem::setUpData
 *   SetUp the data of this item.
 *
 *   This method may be called from a QtConcurrent worker thread.  All
 *   expensive I/O is done first on the calling thread; the QStandardItem
 *   mutations (setText/setFlags/setData/setToolTip) are then dispatched
 *   to the main (GUI) thread via a BlockingQueuedConnection so they never
 *   race with the model's internal state or connected views.
 */
void FileElementCollectionItem::setUpData()
{
	// ── Computation phase (any thread) ────────────────────────────────────
	const Qt::ItemFlags flags = isDir()
		? (Qt::ItemIsSelectable | Qt::ItemIsDragEnabled
		   | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled)
		: (Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);

	const QString display_name = computeDisplayName();
	const QString tooltip      = collectionPath();
	QString search_data;

	if (!isDir()) {
		if (m_path.endsWith(".qetmak")) {
			search_data = display_name;
		} else {
			ElementsLocation loc(tooltip);
			DiagramContext context = loc.elementInformations();
			QStringList sl;
			for (const QString &key : context.keys())
				sl.append(context.value(key).toString());
			sl.append(display_name);
			search_data = sl.join(' ');
		}
	}

	// ── Apply phase (main/GUI thread only) ────────────────────────────────
	// setText/setFlags/setData/setToolTip on a QStandardItem that is in a
	// model trigger dataChanged signals — not safe to emit from a background
	// thread.  Route through the model (a QObject) with BlockingQueuedConnection
	// so the future watcher marks each item done only after the update lands.
	auto apply = [this, flags, display_name, search_data, tooltip]() {
		setText(display_name);
		setFlags(flags);
		setData(search_data);
		setToolTip(tooltip);
	};

	if (QStandardItemModel *m = model()) {
		if (QThread::currentThread() == m->thread())
			apply();
		else
			QMetaObject::invokeMethod(m, apply, Qt::BlockingQueuedConnection);
	} else {
		apply();
	}
}

/**
 *   @brief FileElementCollectionItem::setUpIcon
 *   SetUp the icon of this item.
 *   Because icon use several memory,
 *   we use this method for setup icon instead setUpData.
 */
void FileElementCollectionItem::setUpIcon()
{
	if (!icon().isNull())
		return;

	if (isCollectionRoot()) {
		QString macrosPath = QETApp::userMacrosDir();
		if (macrosPath.endsWith("/")) macrosPath.remove(macrosPath.length() - 1, 1);

		if (m_path == QETApp::commonElementsDirN())
			setIcon(QIcon(":/ico/16x16/qet.png"));
		else if (m_path == QETApp::companyElementsDirN())
			setIcon(QIcon(":/ico/16x16/go-company.png"));
		else if (m_path == macrosPath)
			setIcon(QIcon(":/ico/16x16/go-home.png"));
		else
			setIcon(QIcon(":/ico/16x16/go-home.png"));
	}
	else
	{
		if (isDir()) {
			setIcon(QET::Icons::Folder);
		} else {
			if (m_path.endsWith(".qetmak")) {
				setIcon(QIcon());
			} else {
				ElementsLocation loc(collectionPath());
				setIcon(loc.icon());
			}
		}
	}
}

/**
	@brief FileElementCollectionItem::setPathName
	Set the name of this item in the file system path.
	This item must have a parent,
	because they should be a child item of another.
	For create a new file collection see setRootPath.
	@param path_name
	@param set_data
	@param hide_element
*/
void FileElementCollectionItem::setPathName(const QString& path_name,
											bool set_data,
											bool hide_element)
{
	m_path = path_name;

	//This isn't an element or template, we create the childs
	if (!path_name.endsWith(".elmt") && !path_name.endsWith(".qetmak"))
		populate(set_data, hide_element);
}

/**
	@brief FileElementCollectionItem::populate
	Create the childs of this item
	@param set_data : if true, call setUpData for every child of this item
	@param hide_element
*/
void FileElementCollectionItem::populate(bool set_data, bool hide_element)
{
	QDir dir (fileSystemPath());

		//Get all directory in this directory.
	for (auto& str :
		 dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
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
	dir.setNameFilters(QStringList() << "*.elmt" << "*.qetmak");
	for (auto& str :
		dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name))
	{
		FileElementCollectionItem *feci = new FileElementCollectionItem();
		appendRow(feci);
		feci->setPathName(str, set_data);
		if (set_data)
			feci->setUpData();
	}
}

/**
 * @brief FileElementCollectionItem::isMacrosCollection
 * @return True if this item represent the macros collection
 */
bool FileElementCollectionItem::isMacrosCollection() const
{
	QString macrosPath = QETApp::userMacrosDir();
	if (macrosPath.endsWith("/")) macrosPath.remove(macrosPath.length() - 1, 1);

	return fileSystemPath().startsWith(macrosPath);
}
