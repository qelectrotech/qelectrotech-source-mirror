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
#include <QtXml>
#include "elementscategorieslist.h"
#include "qetapp.h"
#include "elementscategory.h"
#include "qetdiagrameditor.h"

/**
	Constructeur
	@param parent QWidget parent de ce widget
*/
ElementsCategoriesList::ElementsCategoriesList(QWidget *parent) : QTreeWidget(parent) {
	// selection unique
	setSelectionMode(QAbstractItemView::SingleSelection);
	setColumnCount(1);
	header() -> hide();
	
	// charge les categories
	reload();
}

/**
	Destructeur
*/
ElementsCategoriesList::~ElementsCategoriesList() {
}

/**
	Recharge l'arbre des categories
*/
void ElementsCategoriesList::reload() {
	// vide l'arbre
	while (takeTopLevelItem(0));
	
	// chargement des elements de la collection utilisateur
	addDir(invisibleRootItem(), QETApp::customElementsDir(), tr("Collection utilisateur"));
}

/**
	Methode privee permettant d'ajouter un dossier au panel d'appareils
	@param qtwi_parent QTreeWidgetItem parent sous lequel sera insere l'element
	@param dossier Chemin absolu du dossier a inserer
	@param nom Parametre facultatif permettant de forcer le nom du dossier.
	S'il n'est pas precise, la fonction ouvre le fichier qet_directory situe
	dans le dossier et y lit le nom du dossier ; si ce fichier n'existe pas ou
	est invalide, la fonction utilise le nom du dossier.
*/
void ElementsCategoriesList::addDir(QTreeWidgetItem *qtwi_parent, QString adr_dossier, QString nom) {
	ElementsCategory category(adr_dossier);
	if (!category.exists()) return;
	
	// recupere le nom de la categorie
	QString nom_categorie = (nom != QString()) ? nom : category.name();
	
	// creation du QTreeWidgetItem representant le dossier
	QTreeWidgetItem *qtwi_dossier = new QTreeWidgetItem(qtwi_parent, QStringList(nom_categorie));
	qtwi_dossier -> setData(0, Qt::UserRole, category.absolutePath());
	qtwi_dossier -> setExpanded(true);
	
	// ajout des sous-categories / sous-dossiers
	QStringList dossiers = category.entryList(QStringList(), QDir::AllDirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDir::Name);
	foreach(QString dossier, dossiers) addDir(qtwi_dossier, adr_dossier + dossier + "/");
}

/**
	@return Le dossier de la categorie selectionnee
*/
QString ElementsCategoriesList::selectedCategoryPath() {
	QTreeWidgetItem *qtwi = currentItem();
	if (qtwi) return(qtwi -> data(0, Qt::UserRole).toString());
	else return(NULL);
}

/**
	@return Le nom de la categorie selectionnee
*/
QString ElementsCategoriesList::selectedCategoryName() {
	QTreeWidgetItem *qtwi = currentItem();
	if (qtwi) return(qtwi -> data(0, Qt::DisplayRole).toString());
	else return(NULL);
}
