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
#ifndef ELEMENTS_CATEGORIES_WIDGET_H
#define ELEMENTS_CATEGORIES_WIDGET_H
#include <QtGui>
class ElementsCategoriesList;
/**
	This class provides a widget listing available elements categories along
	with buttons to add, change or create categories.
*/
class ElementsCategoriesWidget : public QWidget {
	Q_OBJECT
	
	// Constructors, destructor
	public:
	ElementsCategoriesWidget(QWidget * = 0);
	virtual ~ElementsCategoriesWidget();
	
	private:
	ElementsCategoriesWidget(const ElementsCategoriesWidget &);
	
	// attributes
	private:
	ElementsCategoriesList *elementscategorieslist;
	QToolBar *toolbar;
	QAction *action_reload;
	QAction *action_new;
	QAction *action_open;
	QAction *action_delete;
	
	// methods
	public:
	ElementsCategoriesList &elementsCategoriesList() const;
	
	public slots:
	void newCategory();
	void editCategory();
	void removeCategory();
	void updateButtons();
};

/**
	@return The ElementsCategoriesList embedded within this widget.
*/
inline ElementsCategoriesList &ElementsCategoriesWidget::elementsCategoriesList() const {
	return(*elementscategorieslist);
}

#endif
