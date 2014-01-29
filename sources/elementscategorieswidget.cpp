/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "elementscategorieswidget.h"
#include "elementscategorieslist.h"
#include "elementscategoryeditor.h"
#include "elementscategorydeleter.h"
#include "elementscategory.h"
#include "qeticons.h"

/**
	Constructeur
	@param parent Le QWidget parent
*/
ElementsCategoriesWidget::ElementsCategoriesWidget(QWidget *parent) : QWidget(parent) {
	// initialise la liste des categories
	elementscategorieslist = new ElementsCategoriesList(false, QET::All, this);
	
	// actions
	action_reload = new QAction(QET::Icons::ViewRefresh,    tr("Recharger les cat\351gories"), this);
	action_new    = new QAction(QET::Icons::FolderNew,    tr("Nouvelle cat\351gorie"),       this);
	action_open   = new QAction(QET::Icons::FolderEdit,   tr("\311diter la cat\351gorie"),   this);
	action_delete = new QAction(QET::Icons::FolderDelete, tr("Supprimer la cat\351gorie"),   this);
	
	// initialise la barre d'outils
	toolbar = new QToolBar(this);
	toolbar -> setMovable(false);
	toolbar -> addAction(action_reload);
	toolbar -> addAction(action_new);
	toolbar -> addAction(action_open);
	toolbar -> addAction(action_delete);
	
	connect(action_reload,          SIGNAL(triggered()),              elementscategorieslist, SLOT(reload())        );
	connect(action_new,             SIGNAL(triggered()),              this,                   SLOT(newCategory())   );
	connect(action_open,            SIGNAL(triggered()),              this,                   SLOT(editCategory())  );
	connect(action_delete,          SIGNAL(triggered()),              this,                   SLOT(removeCategory()));
	connect(elementscategorieslist, SIGNAL(itemSelectionChanged()),   this,                   SLOT(updateButtons()) );
	
	updateButtons();
	
	// disposition verticale
	QVBoxLayout *vlayout = new QVBoxLayout(this);
	vlayout -> setMargin(0);
	vlayout -> setSpacing(0);
	vlayout -> addWidget(toolbar);
	vlayout -> addWidget(elementscategorieslist);
	vlayout -> setStretchFactor(elementscategorieslist, 75000);
	setLayout(vlayout);
}

/**
	Destructeur
*/
ElementsCategoriesWidget::~ElementsCategoriesWidget() {
}

/**
	Lance un editeur de categorie en mode "creation de categorie"
*/
void ElementsCategoriesWidget::newCategory() {
	// recupere le chemin virtuel de la categorie selectionnee
	ElementsLocation s_c_path = elementscategorieslist -> selectedLocation();
	if (s_c_path.isNull()) return;
	
	// lance un editeur de categorie
	ElementsCategoryEditor *editor = new ElementsCategoryEditor(s_c_path, false, this);
	int result = editor -> exec();
	
	// recharge la collection si besoin
	if (result == QDialog::Accepted) {
		elementscategorieslist -> reload();
	}
}

/**
	Lance un editeur de categorie en mode "edition de categorie"
*/
void ElementsCategoriesWidget::editCategory() {
	ElementsLocation s_c_path = elementscategorieslist -> selectedLocation();
	if (s_c_path.isNull()) return;
	(new ElementsCategoryEditor(s_c_path, true, this)) -> exec();
	elementscategorieslist -> reload();
}

/**
	Supprime la categorie selectionnee
*/
void ElementsCategoriesWidget::removeCategory() {
	// recupere le chemin de la categorie
	ElementsLocation s_c_path = elementscategorieslist -> selectedLocation();
	if (s_c_path.isNull()) return;
	
	// supprime la categorie
	ElementsCategoryDeleter cat_deleter(s_c_path, this);
	cat_deleter.exec();
	
	// recharge la liste des categories
	elementscategorieslist -> reload();
}

/**
	Met a jour l'etat (active / desactive) des boutons en fonction de ce qui
	est selectionne.
*/
void ElementsCategoriesWidget::updateButtons() {
	QList<QTreeWidgetItem *> sel_items = elementscategorieslist -> selectedItems();
	bool sel_items_empty = !sel_items.isEmpty();
	bool is_top_lvl_item = sel_items_empty && (elementscategorieslist -> indexOfTopLevelItem(sel_items.at(0)) != -1);
	action_new    -> setEnabled(sel_items_empty);
	action_open   -> setEnabled(sel_items_empty && !is_top_lvl_item);
	action_delete -> setEnabled(sel_items_empty && !is_top_lvl_item);
}
