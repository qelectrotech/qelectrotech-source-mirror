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
#ifndef FILE_ELEMENTS_CATEGORY_H
#define FILE_ELEMENTS_CATEGORY_H
#include <QtCore>
#include "elementscategory.h"
class FileElementsCollection;
class FileElementDefinition;
/**
	Cette classe represente une categorie d'elements accessible via un systeme
	de fichiers.
*/
class FileElementsCategory : public ElementsCategory {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	FileElementsCategory(const QString & = QString(), FileElementsCategory * = 0, FileElementsCollection * = 0);
	virtual ~FileElementsCategory();
	
	private:
	FileElementsCategory(const FileElementsCategory &);
	
	// methodes
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
	
	// attributs
	protected:
	/// Collection parente, de type fichier
	FileElementsCollection *file_parent_collection_;
	/// Categorie parente, de type fichier
	FileElementsCategory   *file_parent_category_;
	/// Sous-categories contenues dans cette categorie
	QHash<QString, FileElementsCategory  *> categories_;
	/// Elements contenus dans cette categorie
	QHash<QString, FileElementDefinition *> elements_;
	/// Dossier representant cette categorie sur le systeme de fichiers
	QDir cat_dir;
};
#endif
