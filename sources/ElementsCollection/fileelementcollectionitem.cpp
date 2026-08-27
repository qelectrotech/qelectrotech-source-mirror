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

#include <QApplication>
#include <QDir>
#include <QPainter>
#include <QPixmap>
#include <QStyle>

namespace {
	/**
		@return the folder icon overlaid with a small warning badge in the
		bottom-right corner. Used for a directory whose qet_directory could
		not be read (@see FileElementCollectionItem::m_qet_directory_unreadable),
		so the problem is visible in the tree itself and not only on hover
		via the tooltip. Built once: same folder icon, same badge, every time.
	*/
	const QIcon &unreadableFolderIcon()
	{
		static const QIcon icon = []() {
			QPixmap pixmap = QET::Icons::Folder.pixmap(16, 16);
			const QPixmap badge = QApplication::style()
				->standardIcon(QStyle::SP_MessageBoxWarning)
				.pixmap(9, 9);

			QPainter painter(&pixmap);
			painter.drawPixmap(pixmap.width() - badge.width(),
					    pixmap.height() - badge.height(),
					    badge);
			painter.end();

			return QIcon(pixmap);
		}();
		return icon;
	}
}

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
 * @brief FileElementCollectionItem::localName
 * @return the located name of this item
 */
QString FileElementCollectionItem::localName()
{
	if (!text().isNull())
		return text();

	else if (isDir()) {
		if (isCollectionRoot()) {
			QString macrosPath = QETApp::userMacrosDir();
			if (macrosPath.endsWith("/")) macrosPath.remove(macrosPath.length() - 1, 1);

			if (m_path == QETApp::commonElementsDirN())
				setText(QObject::tr("Collection QET"));
			else if (m_path == QETApp::companyElementsDirN())
				setText(QObject::tr("Collection Company"));
			else if (m_path == QETApp::customElementsDirN())
				setText(QObject::tr("Collection utilisateur"));
			else if (m_path == macrosPath)
				setText(QObject::tr("Makros"));
			else
				setText(QObject::tr("Collection inconnue"));
		}
		else
		{
			// Fall back to the raw directory name (m_path) whenever the
			// translated name can't be obtained -- qet_directory missing,
			// unreadable (e.g. a Windows path-encoding issue with special
			// characters, see bugtracker #332), malformed, or present but
			// without a usable name entry -- rather than leaving the item
			// blank.
			QString display_name;
			bool readable = false;
			QString str(fileSystemPath() % "/qet_directory");
			pugi::xml_document docu;
			if (docu.load_file(str.toStdWString().c_str()))
			{
				if (QString(docu.document_element().name())
					== "qet-directory")
				{
					readable = true;
					NamesList nl;
					nl.fromXml(docu.document_element());
						// Deliberately no fallback argument: a non-empty one
						// is returned *before* NamesList::name() reaches its
						// "first available translation" step, so passing
						// m_path here would replace a perfectly good name in
						// some other language with the raw directory name.
						// The fallback belongs after the chain, not inside it.
					display_name = nl.name();
				}
			}
			setText(display_name.isEmpty() ? m_path : display_name);

				// Only a file-level failure counts: a readable qet-directory
				// with no entry for the current language is not an error,
				// NamesList::name() resolves that on its own. Recorded here
				// and reported by setUpData(), which sets the tooltip after
				// this runs.
			m_qet_directory_unreadable = !readable;
		}
	}
	else if (isElement()) {
		ElementsLocation loc(collectionPath());
		QString display_name = loc.name();
		if (display_name.endsWith(".qetmak")) {
			display_name.remove(".qetmak");
		}
		setText(display_name);
	}

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

	else if (isDir()) {
		localName();
	}
	else if (isElement()) {
		QString display_name = location.name();
		if (display_name.endsWith(".qetmak")) {
			display_name.remove(".qetmak");
		}
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
	const QString dir = QETApp::customElementsDirN();
	const QString path = fileSystemPath();
	return path == dir || path.startsWith(dir + QLatin1Char('/'));
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
 *   SetUp the data of this item
 */
void FileElementCollectionItem::setUpData()
{
	if (isDir())
	{
		localName();
		setFlags(Qt::ItemIsSelectable
		| Qt::ItemIsDragEnabled
		| Qt::ItemIsDropEnabled
		| Qt::ItemIsEnabled);
	}
	else
	{
		setFlags(Qt::ItemIsSelectable
		| Qt::ItemIsDragEnabled
		| Qt::ItemIsEnabled);

		if (m_path.endsWith(".qetmak")) {
			setData(localName());
		} else {
			// Parse standard element information for search
			ElementsLocation loc(collectionPath());
			DiagramContext context = loc.elementInformations();
			QStringList search_list;
			for (QString& key : context.keys())
			{ search_list.append(context.value(key).toString()); }
			search_list.append(localName(loc));
			setData(search_list.join(" "));

			// Tooltip: show what a truncated tree label can't - the full
			// localized name and the descriptive element information.
			// Reuses the location/context already parsed for the search
			// index above; the collection path stays as the last line
			// (it used to be the whole tooltip).
			QStringList tip;
			tip << localName(loc);
			for (const auto &key : { QStringLiteral("description"),
									 QStringLiteral("manufacturer"),
									 QStringLiteral("manufacturer_reference") })
			{
				const QString value = context.value(key).toString();
				if (!value.isEmpty())
					tip << value;
			}
			tip << collectionPath();
			tip.removeDuplicates();
			setToolTip(tip.join(QLatin1Char('\n')));
			return;
		}
	}

		// Falling back to the raw directory name keeps the folder usable, but
		// on its own it hides the fact that a file is broken: the user sees a
		// plausible name and never learns there is anything to repair. Say so
		// above the collection path, which stays as the last line the way the
		// element tooltip above builds it.
	QStringList tip;
	if (isDir() && m_qet_directory_unreadable)
	{
		tip << QObject::tr("Le fichier « %1 » est absent ou illisible : "
				   "le nom traduit de ce dossier n'a pas pu être lu, "
				   "son nom de dossier est affiché à la place.")
		       .arg(fileSystemPath() % "/qet_directory");
	}
	tip << collectionPath();
	setToolTip(tip.join(QLatin1Char('\n')));
}

/**
 *   @brief FileElementCollectionItem::setUpIcon
 *   SetUp the icon of this item.
 *   Because icon use several memory,
 *   we use this method for setup icon instead setUpData.
 */
void FileElementCollectionItem::setUpIcon()
{
		// Must return unconditionally once setUpIcon has run: setIcon() calls
		// setData(), which emits dataChanged() regardless of whether the new
		// icon differs from the old one (QIcon has no meaningful equality).
		// QTreeView responds to dataChanged() by recomputing the row's size
		// hint, which re-enters data() for this same index -- so without this
		// guard, any repeated setIcon() here recurses until the stack
		// overflows. Confirmed by crash report on PR #633.
		//
		// We use a dedicated bool instead of icon().isNull() because
		// some items intentionally keep a null icon (e.g. .qetmak),
		// which would bypass a null-based guard and still recurse.
		//
		// This item's m_qet_directory_unreadable is already final by the time
		// this can run at all: ElementsCollectionModel only attaches itself
		// to the tree view (making data() reachable) from loadingFinished(),
		// which fires after the QtConcurrent::map over every item -- this one
		// included -- has completed. So there is no race to work around here.
	if (m_icon_initialized)
		return;
	m_icon_initialized = true;

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
			setIcon(m_qet_directory_unreadable ? unreadableFolderIcon()
							    : QET::Icons::Folder);
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
