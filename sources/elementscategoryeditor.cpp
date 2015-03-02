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
#include "elementscategoryeditor.h"
#include "elementscollection.h"
#include "elementscategory.h"
#include "nameslistwidget.h"
#include "qet.h"
#include "qetapp.h"
#include "qfilenameedit.h"
#include "qetmessagebox.h"

/**
	Constructeur fournissant un dialogue d'edition de categorie.
	@param category_path Chemin de la categorie a editer ou de la categorie parente en cas de creation
	@param edit booleen a true pour le mode edition, a false pour le mode creation
	@param parent QWidget parent du dialogue
*/
ElementsCategoryEditor::ElementsCategoryEditor(const ElementsLocation &category_path, bool edit, QWidget *parent) :
	QDialog(parent),
	mode_edit(edit)
{
	// dialogue basique
	buildDialog();
	
	// recupere la categorie a editer
	ElementsCollectionItem *category_item = QETApp::collectionItem(category_path);
	if (category_item) category_item = category_item -> toCategory();
	
	if (!category_item || !category_item -> isCategory()) {
		QET::QetMessageBox::warning(
			this,
			tr("Catégorie inexistante", "message box title"),
			tr("La catégorie demandée n'existe pas. Abandon.", "message box content")
		);
		return;
	} else {
		category = category_item -> toPureCategory();
	}
	
	if (mode_edit) {
		setWindowTitle(tr("Éditer une catégorie", "window title"));
		connect(buttons, SIGNAL(accepted()), this, SLOT(acceptUpdate()));
		
		// edition de categorie = affichage des noms deja existants
		names_list -> setNames(category -> categoryNames());
		internal_name_ -> setText(category -> pathName());
		internal_name_ -> setReadOnly(true);
	} else {
		setWindowTitle(tr("Créer une nouvelle catégorie", "window title"));
		connect(buttons, SIGNAL(accepted()), this, SLOT(acceptCreation()));
		
		// nouvelle categorie = une ligne pre-machee
		NamesList cat_names;
		cat_names.addName(QLocale::system().name().left(2), tr("Nom de la nouvelle catégorie", "default name when creating a new category"));
		names_list -> setNames(cat_names);
	}
	
	// gestion de la lecture seule
	if (!category -> isWritable()) {
		QET::QetMessageBox::warning(
			this,
			tr("Édition en lecture seule", "message box title"),
			tr("Vous n'avez pas les privilèges nécessaires pour modifier cette catégorie. Elle sera donc ouverte en lecture seule.", "message box content")
		);
		names_list -> setReadOnly(true);
		internal_name_ -> setReadOnly(true);
	}
}

/**
	Destructeur
*/
ElementsCategoryEditor::~ElementsCategoryEditor() {
}

/**
	Bases du dialogue de creation / edition
*/
void ElementsCategoryEditor::buildDialog() {
	QVBoxLayout *editor_layout = new QVBoxLayout();
	setLayout(editor_layout);
	
	names_list = new NamesListWidget();
	internal_name_label_ = new QLabel(tr("Nom interne : "));
	internal_name_ = new QFileNameEdit();
	
	buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
	
	QHBoxLayout *internal_name_layout = new QHBoxLayout();
	internal_name_layout -> addWidget(internal_name_label_);
	internal_name_layout -> addWidget(internal_name_);
	
	editor_layout -> addLayout(internal_name_layout);
	editor_layout -> addWidget(new QLabel(tr("Vous pouvez spécifier un nom par langue pour la catégorie.")));
	editor_layout -> addWidget(names_list);
	editor_layout -> addWidget(buttons);
}

/**
	Valide les donnees entrees par l'utilisateur lors d'une creation de
	categorie
*/
void ElementsCategoryEditor::acceptCreation() {
	if (!category -> isWritable()) QDialog::accept();
	
	// il doit y avoir au moins un nom
	if (!names_list -> checkOneName()) return;
	
	// exige un nom de dossier de la part de l'utilisateur
	if (!internal_name_ -> isValid()) {
		QET::QetMessageBox::critical(
			this,
			tr("Nom interne manquant", "message box title"),
			tr("Vous devez spécifier un nom interne.", "message box content")
		);
		return;
	}
	QString dirname = internal_name_ -> text();
	
	// verifie que le nom interne n'est pas deja pris
	if (category -> category(dirname)) {
		QET::QetMessageBox::critical(
			this,
			tr("Nom interne déjà utilisé", "message box title"),
			tr(
				"Le nom interne que vous avez choisi est déjà utilisé "
				"par une catégorie existante. Veuillez en choisir un autre.",
				"message box content"
			)
		);
		return;
	}
	
	// cree la nouvelle categorie
	ElementsCategory *new_category = category -> createCategory(dirname);
	if (!new_category) {
		QET::QetMessageBox::critical(
			this,
			tr("Erreur", "message box title"),
			tr("Impossible de créer la catégorie", "message box content")
		);
		return;
	}
	
	// chargement des noms
	NamesList names = names_list -> names();
	foreach(QString lang, names.langs()) {
		new_category -> addName(lang, names[lang]);
	}
	
	// ecriture de la 
	if (!new_category -> write()) {
		QET::QetMessageBox::critical(
			this,
			tr("Erreur", "message box title"),
			tr("Impossible d'enregistrer la catégorie", "message box content")
		);
		return;
	}
	
	QDialog::accept();
}

/**
	Valide les donnees entrees par l'utilisateur lors d'une modification de
	categorie
*/
void ElementsCategoryEditor::acceptUpdate() {
	
	if (!category -> isWritable()) QDialog::accept();
	
	// il doit y avoir au moins un nom
	if (!names_list -> checkOneName()) return;
	
	// chargement des noms
	category -> clearNames();
	NamesList names = names_list -> names();
	foreach(QString lang, names.langs()) {
		category -> addName(lang, names[lang]);
	}
	
	category -> write();
	
	QDialog::accept();
}
