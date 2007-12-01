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
#include "elementdeleter.h"

/**
	Constructeur
	@param elmt_path Chemin du fichier representant l'element a supprimer
	@param parent QWidget parent
*/
ElementDeleter::ElementDeleter(const QString &elmt_path, QWidget *parent) :
	QWidget(parent),
	element_path(elmt_path)
{
}

/// Destructeur
ElementDeleter::~ElementDeleter() {
}

/**
	Supprime l'element : verifie l'existence du fichier, demande confirmation a
	l'utilisateur et avertit ce dernier si la suppression a echoue.
*/
void ElementDeleter::exec() {
	// verifie l'existence de l'element
	QFile elmt_file(element_path);
	if (!elmt_file.exists()) return;
	
	// confirmation #1
	QMessageBox::StandardButton answer_1 = QMessageBox::question(
		this,
		tr("Supprimer l'\351l\351ment ?"),
		tr("\312tes-vous s\373r de vouloir supprimer cet \351l\351ment ?\n"),
		QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel
	);
	if (answer_1 != QMessageBox::Yes) return;
	
	// supprime l'element
	if (!elmt_file.remove()) {
		QMessageBox::warning(
			this,
			tr("Suppression de l'\351l\351ment"),
			tr("La suppression de l'\351l\351ment a \351chou\351.\n"
			"V\351rifiez vos droits sur le fichier ") + element_path + tr(".")
		);
	}
}
