/*
	Copyright 2006-2011 Xavier Guerrin
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
#include "qet.h"
#include "elementslocation.h"
class ElementsCollection;
class ElementsCategory;
class ElementDefinition;
/**
	Cette classe fournit une liste graphique des categories d'elements de
	l'utilisateur.
*/
class ElementsCategoriesList : public QTreeWidget {
	Q_OBJECT
	
	// Constructeurs, destructeur
	public:
	ElementsCategoriesList(bool = false, uint = QET::All, QWidget * = 0);
	virtual ~ElementsCategoriesList();
	
	private:
	ElementsCategoriesList(const ElementsCategoriesList &);
	
	// methodes
	public:
	QString selectedCategoryName() const;
	ElementsLocation selectedLocation() const;
	bool selectLocation(const ElementsLocation &);
	
	private:
	QTreeWidgetItem *addCollection(QTreeWidgetItem *, ElementsCollection *, const QString & = QString(), const QIcon & = QIcon());
	QTreeWidgetItem *addCategory  (QTreeWidgetItem *, ElementsCategory   *, const QString & = QString(), const QIcon & = QIcon());
	QTreeWidgetItem *addElement   (QTreeWidgetItem *, ElementDefinition  *, const QString & = QString(), const QIcon & = QIcon());
	QString categoryName(QDir &);
	
	public slots:
	void reload();
	
	private slots:
	void selectionChanged(QTreeWidgetItem *, QTreeWidgetItem *);
	
	signals:
	void locationChanged(const ElementsLocation &);
	
	// attributs
	private:
	bool display_elements_;
	int selectables_;
	bool first_load;
	QHash<QTreeWidgetItem *, ElementsLocation> locations_;
};
#endif
