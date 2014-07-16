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
#include "newelementwizard.h"
#include "elementscategory.h"
#include "elementscategorieswidget.h"
#include "elementscategorieslist.h"
#include "nameslistwidget.h"
#include "qetgraphicsitem/element.h"
#include "qetelementeditor.h"
#include "qet.h"
#include "qetapp.h"
#include "elementscollectionitem.h"
#include "qfilenameedit.h"
#include "qetmessagebox.h"

/**
	Constructeur
	@param parent QWidget parent de ce dialogue
	@param f flags pour le dialogue
*/
NewElementWizard::NewElementWizard(QWidget *parent, Qt::WindowFlags f) :
	QWizard(parent, f),
	chosen_category(0)
{
	setOptions(options() & ~QWizard::NoCancelButton);

#ifdef Q_WS_WIN
	setWizardStyle(QWizard::AeroStyle);
#elif defined(Q_WS_MAC)
	setWizardStyle(QWizard::MacStyle);
#endif

	setPixmap(LogoPixmap, QPixmap(":/ico/256x256/qelectrotech.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	setWindowTitle(tr("Cr\351er un nouvel \351l\351ment : Assistant", "window title"));
	setButtonText(QWizard::NextButton, tr("&Suivant >"));
	addPage(buildStep1());
	addPage(buildStep2());
	addPage(buildStep3());
	setFixedSize(705, 325);
}

/**
	Destructeur
*/
NewElementWizard::~NewElementWizard() {
}

/**
	@return la categorie parente selectionnee, ou 0 si celle-ci n'a pas encore
	ete choisie.
*/
ElementsCategory *NewElementWizard::selectedCategory() const {
	return(chosen_category);
}

/**
	@param category Categorie d'elements dans laquelle le nouvel element sera
	place
	@return true si ce choix est possible et a ete pris en compte, false sinon
*/
bool NewElementWizard::preselectCategory(ElementsCategory *category) {
	// verifie si la categorie est utilisable
	if (!category || !category -> exists() || !category -> isWritable()) {
		return(false);
	}
	
	// selectionne la categorie ainsi demandee dans la liste
	if (categories_list -> elementsCategoriesList().selectLocation(category -> location())) {
		chosen_category = category;
		return(true);
	}
	
	return(false);
}

/**
	Met en place l'etape 1 : Categorie
*/
QWizardPage *NewElementWizard::buildStep1() {
	QWizardPage *page = new QWizardPage();
	page -> setProperty("WizardState", Category);
	page -> setTitle(tr("\311tape 1/3 : Cat\351gorie parente", "wizard page title"));
	page -> setSubTitle(tr("S\351lectionnez une cat\351gorie dans laquelle enregistrer le nouvel \351l\351ment.", "wizard page subtitle"));
	QVBoxLayout *layout = new QVBoxLayout();
	
	categories_list = new ElementsCategoriesWidget();
	layout -> addWidget(categories_list);
	
	page -> setLayout(layout);
	return(page);
}

/**
	Met en place l'etape 2 : Nom du fichier
*/
QWizardPage *NewElementWizard::buildStep2() {
	QWizardPage *page = new QWizardPage();
	page -> setProperty("WizardState", Filename);
	page -> setTitle(tr("\311tape 2/3 : Nom du fichier", "wizard page title"));
	page -> setSubTitle(tr("Indiquez le nom du fichier dans lequel enregistrer le nouvel \351l\351ment.", "wizard page subtitle"));
	QVBoxLayout *layout = new QVBoxLayout();
	
	qle_filename = new QFileNameEdit(tr("nouvel_element"));
	qle_filename -> selectAll();
	QLabel *explication2 = new QLabel(tr("Vous n'\352tes pas oblig\351 de pr\351ciser l'extension *.elmt. Elle sera ajout\351e automatiquement."));
	explication2 -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	explication2 -> setWordWrap(true);
	layout -> addWidget(qle_filename);
	layout -> addWidget(explication2);
	layout -> addSpacing(100);
	
	page -> setLayout(layout);
	return(page);
}

/**
	Met en place l'etape 3 : Noms de l'element
*/
QWizardPage *NewElementWizard::buildStep3() {
	QWizardPage *page = new QWizardPage();
	page -> setProperty("WizardState", Names);
	page -> setTitle(tr("\311tape 3/3 : Noms de l'\351l\351ment", "wizard page title"));
	page -> setSubTitle(tr("Indiquez le ou les noms de l'\351l\351ment.", "wizard page subtitle"));
	QVBoxLayout *layout = new QVBoxLayout();
	
	element_names = new NamesListWidget();
	NamesList hash_name;
	hash_name.addName(QLocale::system().name().left(2), tr("Nom du nouvel \351l\351ment", "default name when creating a new element"));
	element_names -> setNames(hash_name);
	layout -> addWidget(element_names);
	
	page -> setLayout(layout);
	return(page);
}

/// @return true si l'etape en cours est validee, false sinon
bool NewElementWizard::validateCurrentPage() {
	WizardState wizard_state = static_cast<WizardState>(currentPage() -> property("WizardState").toInt());
	if      (wizard_state == Category) return(validStep1());
	else if (wizard_state == Filename) return(validStep2());
	else if (wizard_state == Names) {
		// must have one name minimum
		if (element_names -> checkOneName())
			createNewElement();
		return true;

	}
	else return(true);
}

/**
	Valide l'etape 1
	@return true si l'etape est validee, false sinon
*/
bool NewElementWizard::validStep1() {
	// il doit y avoir une categorie selectionnee
	bool step1_ok = false;
	ElementsLocation selected_location = categories_list -> elementsCategoriesList().selectedLocation();
	if (ElementsCollectionItem *collection_item = QETApp::collectionItem(selected_location, false)) {
		if (collection_item -> isCategory()) {
			chosen_category = qobject_cast<ElementsCategory *>(collection_item);
			step1_ok = chosen_category;
		}
	}
	
	if (!step1_ok) {
		QET::MessageBox::critical(
			parentWidget(),
			tr("Erreur", "message box title"),
			tr("Vous devez s\351lectionner une cat\351gorie.", "message box content")
		);
	}
	return(step1_ok);
}

/**
	Valide l'etape 2
	@return true si l'etape est validee, false sinon
*/
bool NewElementWizard::validStep2() {
	// il doit y avoir une categorie selectionnee
	if (!chosen_category) return(false);
	QString file_name = qle_filename -> text();
	
	// un nom doit avoir ete entre
	if (file_name.isEmpty()) {
		QET::MessageBox::critical(
			this,
			tr("Erreur", "message box title"),
			tr("Vous devez entrer un nom de fichier", "message box content")
		);
		return(false);
	}
	
	if (!file_name.endsWith(".elmt")) file_name += ".elmt";
	
	// le nom de fichier contient peut etre des caracteres interdits
	if (QET::containsForbiddenCharacters(file_name)) {
		QET::MessageBox::critical(
			this,
			tr("Erreur", "message box title"),
			tr("Merci de ne pas utiliser les caract\350res suivants : \\ / : * ? \" < > |", "message box content")
		);
		return(false);
	}
	
	// le fichier existe peut etre deja
	if (chosen_category -> element(file_name)) {
		QMessageBox::StandardButton answer = QET::MessageBox::question(
			this,
			"\311craser le fichier ?",
			"Le fichier existe d\351j\340. Souhaitez-vous l'\351craser ?",
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
			QMessageBox::No
		);
		return(answer == QMessageBox::Yes);
	}
	
	chosen_file = file_name;
	return(true);
}

/**
	Cree le nouvel element
*/
void NewElementWizard::createNewElement() {
	QETElementEditor *edit_new_element = new QETElementEditor(parentWidget());
	edit_new_element -> setNames(element_names -> names());
	
	ElementsLocation new_element_location = chosen_category -> location();
	new_element_location.addToPath(chosen_file);
	edit_new_element -> setLocation(new_element_location);
	edit_new_element -> show();
}
