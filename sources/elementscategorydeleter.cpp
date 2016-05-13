/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "elementscategorydeleter.h"
#include "qetapp.h"
#include "qetmessagebox.h"

/**
	Constructeur
	@param category_path Chemin virtuel de la categorie a supprimer
	@param parent QWidget parent
*/
ElementsCategoryDeleter::ElementsCategoryDeleter(const ElementsLocation &category_path, QWidget *parent) :
	QWidget(parent),
	category(0)
{
	// recupere la categorie a supprimer
	ElementsCollectionItem *category_item = QETApp::collectionItem(category_path);
	if (!category_item) return;
	
	// on exige une collection ou une categorie
	if (!category_item -> isCollection() && !category_item -> isCategory()) return;
	
	category = category_item;
}

/// Destructeur
ElementsCategoryDeleter::~ElementsCategoryDeleter() {
}

/**
	Supprime la categorie et ses elements : verifie l'existence du dossier,
	demande deux fois confirmation a l'utilisateur et avertit ce dernier si la
	suppression a echoue.
	@return true si la suppression a ete effectuee, false sinon
*/
bool ElementsCategoryDeleter::exec() {
	// verifie l'existence de la categorie
	if (!category) return(false);
	
	// gere le cas ou la suppression d'une collection est demandee
	if (category -> isCollection()) {
		QMessageBox::StandardButton answer_0 = QET::QetMessageBox::question(
			this,
			tr("Vider la collection ?", "message box title"),
			tr("Êtes-vous sûr  de vouloir vider cette collection ?", "message box content"),
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
		);
		if (answer_0 != QMessageBox::Yes) return(false);
	}
	
	/**
		@todo Regression : rafficher le nom de la categorie supprimee
	*/
	// QString cat_name(category -> name().replace("<", "&lt;").replace(">", "&gt;"));
	
	
	// avertissement pour la suppression d'une collection
	// confirmation #1
	QMessageBox::StandardButton answer_1 = QET::QetMessageBox::question(
		this,
		tr("Supprimer la catégorie ?", "message box title"),
		tr(
			"Êtes-vous sûr  de vouloir supprimer la catégorie ?\nTous "
			"les éléments et les catégories contenus dans cette "
			"catégorie seront supprimés.",
			"message box content"
		),
		QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
	);
	if (answer_1 != QMessageBox::Yes) return(false);
	
	// confirmation #2
	QMessageBox::StandardButton answer_2 = QET::QetMessageBox::question(
		this,
		tr("Supprimer la catégorie ?", "message box title"),
		tr(
			"Êtes-vous vraiment sur de vouloir supprimer cette "
			"catégorie ?\nLes changements seront définitifs.",
			"message box content"
		),
		QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel
	);
	if (answer_2 != QMessageBox::Yes) return(false);
	
	// supprime la categorie
	if (!category -> remove()) {
		QET::QetMessageBox::warning(
			this,
			tr("Suppression de la catégorie", "message box title"),
			tr("La suppression de la catégorie a échoué.", "message box content")
		);
		return(false);
	}
	
	return(true);
}
