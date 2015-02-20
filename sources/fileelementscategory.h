/*
	Copyright 2006-2015 The QElectroTech Team
	This file is part of QElectroTech.
	
	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	
	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef FILE_ELEMENTS_CATEGORY_H
#define FILE_ELEMENTS_CATEGORY_H
#include <QtCore>
#include "elementscategory.h"
class FileElementsCollection;
class FileElementDefinition;
/**
	This class represents an elements category stored on a filesystem.
*/
class FileElementsCategory : public ElementsCategory {
	Q_OBJECT
	
	// constructors, destructor
	public:
	FileElementsCategory(const QString & = QString(), FileElementsCategory * = 0, FileElementsCollection * = 0);
	virtual ~FileElementsCategory();
	
	private:
	FileElementsCategory(const FileElementsCategory &);
	
	// methods
	public:
	virtual QString pathName() const;
	virtual QString virtualPath();
	
	virtual QString filePath();
	virtual bool hasFilePath();
	virtual void setFilePath(const QString &);
	
	virtual QList<ElementsCategory *> categories();
	virtual ElementsCategory *category(const QString &);
	virtual ElementsCategory *createCategory(const QString &);
	
	virtual QList<ElementDefinition *> elements();
	virtual ElementDefinition *element(const QString &);
	virtual ElementDefinition *createElement(const QString &);
	
	virtual bool exists();
	virtual bool isReadable();
	virtual bool isWritable();
	
	virtual void reload();
	virtual bool remove();
	virtual bool removeContent();
	virtual bool write();
	
	private:
	bool rmdir(const QString &) const;
	void loadNames();
	void deleteContent();
	
	// attributes
	protected:
	/// Parent collection, stored on filesystem too
	FileElementsCollection *file_parent_collection_;
	/// Paremt Collection, stored on filesystem too
	FileElementsCategory   *file_parent_category_;
	/// Child sub-categories
	QHash<QString, FileElementsCategory  *> categories_;
	/// Child elements
	QHash<QString, FileElementDefinition *> elements_;
	/// Directory matching this category on filesystem
	QDir cat_dir;
};
#endif
