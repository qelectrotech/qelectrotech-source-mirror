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
#ifndef ELEMENTS_CATEGORIES_LIST_H
#define ELEMENTS_CATEGORIES_LIST_H
#include <QtGui>
#include "qet.h"
#include "elementslocation.h"
#include "genericpanel.h"
class ElementsCollection;
class ElementsCategory;
class ElementDefinition;
/**
	This class provides a visual listing of available elements categories.
*/
class ElementsCategoriesList : public GenericPanel {
	Q_OBJECT
	
	// Constructors, destructor
	public:
	ElementsCategoriesList(bool = false, uint = QET::All, QWidget * = 0);
	virtual ~ElementsCategoriesList();
	
	private:
	ElementsCategoriesList(const ElementsCategoriesList &);
	
	// methods
	public:
	ElementsLocation selectedLocation() const;
	bool selectLocation(const ElementsLocation &);
	
	private:
	QString categoryName(QDir &);
	QTreeWidgetItem *makeItem(QET::ItemType, QTreeWidgetItem *, const QString &, const QIcon &);
	
	public slots:
	void reload();
	
	private slots:
	void selectionChanged(QTreeWidgetItem *, QTreeWidgetItem *);
	
	signals:
	void locationChanged(const ElementsLocation &);
	
	// attributes
	private:
	bool display_elements_;
	int selectables_;
	bool first_load;
};
#endif
