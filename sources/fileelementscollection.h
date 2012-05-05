/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef FILE_ELEMENTS_COLLECTION_H
#define FILE_ELEMENTS_COLLECTION_H
#include <QtCore>
#include "elementscollection.h"
class FileElementsCategory;
/**
	Cette classe represente une collection d'elements accessible via un
	systeme de fichier. Typiquement, il s'agit de la collection QET ou
	de la collection utilisateur.
*/
class FileElementsCollection : public ElementsCollection {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	FileElementsCollection(const QString &, ElementsCollectionItem *parent = 0);
	virtual ~FileElementsCollection();
	
	private:
	FileElementsCollection(const FileElementsCollection &);
	
	// methodes
	public:
	virtual void reload();
	virtual ElementsCategory *rootCategory();
	
	virtual bool hasFilePath();
	virtual QString filePath();
	virtual void setFilePath(const QString &);
	virtual bool exists();
	virtual bool isReadable();
	virtual bool isWritable();
	virtual bool write();
	virtual bool isCacheable() const;
	
	private:
	void deleteContent();
	
	// attributs
	private:
	QString coll_path;
	FileElementsCategory *root;
	QMutex reload_mutex_;       ///< Mutex used to avoid loading a collection twice at the same time
};
#endif
