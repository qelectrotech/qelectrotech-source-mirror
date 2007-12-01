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
#ifndef ELEMENTS_CATEGORIES_LIST_H
#define ELEMENTS_CATEGORIES_LIST_H
#include <QtGui>
/**
	Cette classe fournit une liste graphique des categories d'elements de
	l'utilisateur.
*/
class ElementsCategoriesList : public QTreeWidget {
	Q_OBJECT
	
	// Constructeurs, destructeur
	public:
	ElementsCategoriesList(QWidget * = 0);
	virtual ~ElementsCategoriesList();
	
	private:
	ElementsCategoriesList(const ElementsCategoriesList &);
	
	// methodes
	public:
	QString selectedCategoryPath();
	QString selectedCategoryName();
	
	private:
	void addDir(QTreeWidgetItem *, QString, QString = QString());
	QString categoryName(QDir &);
	
	public slots:
	void reload();
};
#endif
