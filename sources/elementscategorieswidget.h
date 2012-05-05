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
#ifndef ELEMENTS_CATEGORIES_WIDGET_H
#define ELEMENTS_CATEGORIES_WIDGET_H
#include <QtGui>
class ElementsCategoriesList;
/**
	Cette classe represente un widget integrant la liste des categories
	de l'utilisteur surplombee de boutons permettant d'ajouter, de modifier
	ou de supprimer des categories
*/
class ElementsCategoriesWidget : public QWidget {
	Q_OBJECT
	
	// Constructeurs, destructeur
	public:
	ElementsCategoriesWidget(QWidget * = 0);
	virtual ~ElementsCategoriesWidget();
	
	private:
	ElementsCategoriesWidget(const ElementsCategoriesWidget &);
	
	// attributs
	private:
	ElementsCategoriesList *elementscategorieslist;
	QToolBar *toolbar;
	QAction *action_reload;
	QAction *action_new;
	QAction *action_open;
	QAction *action_delete;
	
	// methodes
	public:
	ElementsCategoriesList &elementsCategoriesList() const;
	
	public slots:
	void newCategory();
	void editCategory();
	void removeCategory();
	void updateButtons();
};

/**
	@return La liste des categories d'elements du widget
*/
inline ElementsCategoriesList &ElementsCategoriesWidget::elementsCategoriesList() const {
	return(*elementscategorieslist);
}

#endif
