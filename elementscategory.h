/*
	Copyright 2006-2007 Xavier Guerrin
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
#ifndef ELEMENTS_CATEGORY_H
#define ELEMENTS_CATEGORY_H
#include <QtCore>
#include "nameslist.h"
/**
	Cette classe represente une categorie d'elements.
	Une categorie d'elements est en fait un dossier avec un fichier
	qet_directory contenant ses caracteristiques (pour le moment : ses noms).
*/
class ElementsCategory : public QDir {
	// constructeurs, destructeur
	public:
	ElementsCategory(const QString & = QString());
	virtual ~ElementsCategory();
	
	private:
	ElementsCategory(const ElementsCategory &);
	
	// attributs
	private:
	NamesList category_names;
	
	// methodes
	public:
	QString name() const;
	NamesList categoryNames() const;
	void clearNames();
	void addName(const QString &, const QString &);
	bool write() const;
	bool remove() const;
	bool isWritable() const;
	//bool move(const QString &new_parent);
	
	private:
	bool rmdir(const QString &) const;
	void loadNames();
};
#endif
