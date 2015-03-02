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
#include "elementdeleter.h"
#include "qetapp.h"
#include "qetmessagebox.h"

/**
	Constructeur
	@param elmt_path Chemin virtuel du fichier representant l'element a supprimer
	@param parent QWidget parent
*/
ElementDeleter::ElementDeleter(const ElementsLocation &elmt_path, QWidget *parent) :
	QWidget(parent),
	element(0)
{
	// recupere l'element a supprimer
	ElementsCollectionItem *element_item = QETApp::collectionItem(elmt_path);
	if (!element_item) return;
	
	// on exige un element
	if (!element_item -> isElement()) return;
	
	element = element_item;
}

/// Destructeur
ElementDeleter::~ElementDeleter() {
}

/**
	Supprime l'element : verifie l'existence du fichier, demande confirmation a
	l'utilisateur et avertit ce dernier si la suppression a echoue.
*/
bool ElementDeleter::exec() {
	// verifie l'existence de l'element
	if (!element || !element -> isElement()) return(false);
	
	// confirmation #1
	QMessageBox::StandardButton answer_1 = QET::QetMessageBox::question(
		this,
		tr("Supprimer l'élément ?", "message box title"),
		tr("Êtes-vous s\373r de vouloir supprimer cet élément ?\n", "message box content"),
		QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
	);
	if (answer_1 != QMessageBox::Yes) return(false);
	
	/**
		@todo Regression : rafficher le chemin de l'element
	*/
	
	// supprime l'element
	if (!element -> remove()) {
		QET::QetMessageBox::warning(
			this,
			tr("Suppression de l'élément", "message box title"),
			tr("La suppression de l'élément a échoué.", "message box content")
		);
		return(false);
	}
	return(true);
}
