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
#include "elementdialog.h"
#include <QtGui>
#include "qetapp.h"
#include "elementscategorieslist.h"
#include "elementscollectionitem.h"
#include "qfilenameedit.h"
#include "qetmessagebox.h"

/**
	Constructeur par defaut.
	Construit un dialogue permettant d'ouvrir un element
	@param mode Mode du dialogue
	@see ElementDialog::Mode
	@param parentWidget QWidget parent
	@param parent QObject parent
	
*/
ElementDialog::ElementDialog(uint mode, QWidget *parentWidget, QObject *parent) :
	QObject(parent),
	mode_(mode),
	buttons_(0),
	list_(0),
	textfield_(0)
{
	dialog_  = new QDialog(parentWidget);
	dialog_ -> setWindowModality(Qt::WindowModal);
#ifdef Q_WS_MAC
	dialog_ -> setWindowFlags(Qt::Sheet);
#endif
	buttons_ = new QDialogButtonBox();
	
	// types selectionnables dans la liste
	bool display_elements = (mode_ == OpenElement || mode_ == SaveElement);
	int selectables = 0;
	switch(mode_) {
		case OpenElement:  selectables = QET::Element; break;
		case SaveElement:  selectables = QET::ElementsCollectionItem; break;
		case OpenCategory: selectables = QET::ElementsContainer; break;
		case SaveCategory: selectables = QET::ElementsContainer; break;
	}
	list_    = new ElementsCategoriesList(display_elements, selectables);
	connect(list_, SIGNAL(locationChanged(const ElementsLocation &)), this, SLOT(locationChanged(const ElementsLocation &)));
	
	// titre et label
	if (!mode) {
		title_ = tr("Ouvrir un \351l\351ment", "dialog title");
		label_ = tr("Choisissez l'\351l\351ment que vous souhaitez ouvrir.", "dialog content");
	} else if (mode == 1) {
		title_ = tr("Enregistrer un \351l\351ment", "dialog title");
		label_ = tr("Choisissez l'\351l\351ment dans lequel vous souhaitez enregistrer votre d\351finition.", "dialog content");
	} else if (mode == 2) {
		title_ = tr("Ouvrir une cat\351gorie", "dialog title");
		label_ = tr("Choisissez une cat\351gorie.", "dialog content");
	} else {
		title_ = tr("Enregistrer une cat\351gorie", "dialog title");
		label_ = tr("Choisissez une cat\351gorie.", "dialog content");
	}
	
	// mode ouverture / enregistrement
	if (mode_ == SaveCategory || mode_ == SaveElement) {
		buttons_ -> setStandardButtons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
		textfield_ = new QFileNameEdit();
		connect(textfield_, SIGNAL(textChanged(const QString &)), this, SLOT(textFieldChanged(const QString &)));
	} else {
		buttons_ -> setStandardButtons(QDialogButtonBox::Open | QDialogButtonBox::Cancel);
	}
	
	// connexions boutons -> dialogue
	connect(buttons_, SIGNAL(accepted()), this,    SLOT(checkDialog()));
	connect(buttons_, SIGNAL(rejected()), dialog_, SLOT(reject()));
	
	// connexions dialogue -> classe
	connect(dialog_,  SIGNAL(accepted()), this, SLOT(dialogAccepted()));
	connect(dialog_,  SIGNAL(rejected()), this, SLOT(dialogRejected()));
	
	makeInterface();
}

/**
	Destructeur
*/
ElementDialog::~ElementDialog() {
	dialog_ -> setParent(0);
	delete dialog_;
}

/**
	Affiche un dialogue permettant a l'utilisateur de selectionner une categorie existant deja
	@param parentWidget QWidget parent
	@return le chemin virtuel de cette categorie
*/
ElementsLocation ElementDialog::getExistingCategoryLocation(QWidget *parentWidget) {
	return(ElementDialog::execConfiguredDialog(ElementDialog::OpenCategory, parentWidget));
}

/**
	Affiche un dialogue permettant a l'utilisateur de selectionner une nouvelle categorie
	@param parentWidget QWidget parent
	@return le chemin virtuel de cette categorie
*/
ElementsLocation ElementDialog::getNewCategoryLocation(QWidget *parentWidget) {
	return(ElementDialog::execConfiguredDialog(ElementDialog::SaveCategory, parentWidget));
}

/**
	Affiche un dialogue permettant a l'utilisateur de selectionner un element a ouvrir
	@param parentWidget QWidget parent
	@return le chemin virtuel de cet element
*/
ElementsLocation ElementDialog::getOpenElementLocation(QWidget *parentWidget) {
	return(ElementDialog::execConfiguredDialog(ElementDialog::OpenElement, parentWidget));
}

/**
	Affiche un dialogue permettant a l'utilisateur de selectionner un element (existant ou non)
	qu'il souhaite enregistrer
	@param parentWidget QWidget parent
	@return le chemin virtuel de cet element
*/
ElementsLocation ElementDialog::getSaveElementLocation(QWidget *parentWidget) {
	return(ElementDialog::execConfiguredDialog(ElementDialog::SaveElement, parentWidget));
}

/**
	Lance un dialogue selon la configuration mode
	@param mode Mode du dialogue
	@param parentWidget QWidget parent
*/
ElementsLocation ElementDialog::execConfiguredDialog(int mode, QWidget *parentWidget) {
	ElementDialog element_dialog(mode, parentWidget);
	element_dialog.exec();
	return(element_dialog.location());
}

/**
	Assemble les widgets pour obtenir le dialogue final
*/
void ElementDialog::makeInterface() {
	dialog_ -> setWindowTitle(title_);
	
	// disposition verticale
	QVBoxLayout *layout = new QVBoxLayout(dialog_);
	layout -> addWidget(new QLabel(label_));
	layout -> addWidget(list_);
	if (textfield_) {
		QHBoxLayout *filename_layout = new QHBoxLayout();
		filename_layout -> addWidget(new QLabel(tr("Nom :")));
		filename_layout -> addWidget(textfield_);
		layout -> addLayout(filename_layout);
	}
	layout -> addWidget(buttons_);
}

/**
	Execute le dialogue
	@return QDialog::Accepted si le dialogue a ete accepte, false sinon
	@see DialogCode
*/
int ElementDialog::exec() {
	return(dialog_ -> exec());
}

/**
	@return le chemin virtuel choisi via le dialogue
	Si l'utilisateur n'a pas pu faire son choix, une chaine vide est retournee.
*/
ElementsLocation ElementDialog::location() const {
	return(location_);
}

/**
	gere le changement de chemin virtuel par l'utilisateur
	@param new_loc le nouveau chemin virtuel choisi par l'utilisateur
*/
void ElementDialog::locationChanged(const ElementsLocation &new_loc) {
	ElementsCollectionItem *item = QETApp::collectionItem(new_loc);
	if (!item) return;
	if (mode_ == OpenElement) {
		buttons_ -> button(QDialogButtonBox::Open) -> setEnabled(item -> isElement());
	} else if (mode_ == SaveElement) {
		// si l'utilisateur choisit un element existant, on desactive le champ
		textfield_ -> setEnabled(!item -> isElement());
		// il faut soit un element selectionne soit une categorie et un nom
		buttons_ -> button(QDialogButtonBox::Save) -> setEnabled(
			((item -> isCategory() || item -> isCollection()) && !textfield_ -> text().isEmpty()) ||\
			item -> isElement()
		);
	} else if (mode_ == OpenCategory) {
		/// @todo
	} else if (mode_ == SaveCategory) {
		/// @todo
	}
	location_ = new_loc;
}

/**
	Gere le changement de contenu dans le champ de texte
	@param text Contenu du champ de texte
*/
void ElementDialog::textFieldChanged(const QString &text) {
	ElementsCollectionItem *item = QETApp::collectionItem(list_ -> selectedLocation());
	if (!item) return;
	if (mode_ == SaveElement) {
		// il faut soit un element selectionne soit une categorie et un nom
		buttons_ -> button(QDialogButtonBox::Save) -> setEnabled(
			((item -> isCategory() || item -> isCollection()) && !text.isEmpty()) ||\
			item -> isElement()
		);
	} else if (mode_ == SaveCategory) {
		// il faut forcement un nom pour la nouvelle categorie
		buttons_ -> button(QDialogButtonBox::Save) -> setEnabled(!text.isEmpty());
	}
}

/**
	Verifie l'etat du dialogue au moment ou l'utilisateur le valide.
*/
void ElementDialog::checkDialog() {
	// verifie si ce qui a ete selectionne par l'utilisateur correspond au mode du widget
	if (mode_ == OpenElement) {
		// l'utilisateur doit avoir choisi un element existant
		
		// on verifie d'abord que l'utilisateur a choisi quelque chose
		ElementsLocation location = list_ -> selectedLocation();
		if (location.isNull()) {
			QET::MessageBox::critical(
				dialog_,
				tr("Pas de s\351lection", "message box title"),
				tr("Vous devez s\351lectionner un \351l\351ment.", "message box content")
			);
			return;
		}
		
		// on verifie donc que la selection existe
		ElementsCollectionItem *item = QETApp::collectionItem(location);
		if (!item) {
			QET::MessageBox::critical(
				dialog_,
				tr("S\351lection inexistante", "message box title"),
				tr("La s\351lection n'existe pas.", "message box content")
			);
			return;
		}
		
		// puis on verifie qu'il s'agit bien d'un element
		if (!item -> isElement()) {
			QET::MessageBox::critical(
				dialog_,
				tr("S\351lection incorrecte", "message box title"),
				tr("La s\351lection n'est pas un \351l\351ment.", "message box content")
			);
			return;
		}
		
		location_ = location;
	} else if (mode_ == SaveElement) {
		/*
			l'utilisateur doit avoir choisi soit :
			-une categorie et un nom d'element
			-un element existant
		*/
		ElementsLocation location = list_ -> selectedLocation();
		if (location.isNull()) {
			QET::MessageBox::critical(
				dialog_,
				tr("Pas de s\351lection", "message box title"),
				tr("Vous devez s\351lectionner une cat\351gorie ou un \351l\351ment.", "message box content")
			);
			return;
		}
		
		// on verifie donc que la selection existe
		ElementsCollectionItem *item = QETApp::collectionItem(location);
		if (!item) {
			QET::MessageBox::critical(
				dialog_,
				tr("S\351lection inexistante", "message box title"),
				tr("La s\351lection n'existe pas.", "message box content")
			);
			return;
		}
		
		ElementsLocation final_location(location);
		if (!item -> isElement()) {
			QString element_name(textfield_ -> text());
			// si on a une categorie (ou une collection), il nous faut un nom d'element
			if (element_name.isEmpty()) {
				QET::MessageBox::critical(
					dialog_,
					tr("Nom manquant", "message box title"),
					tr("Vous devez entrer un nom pour l'\351l\351ment", "message box content")
				);
				return;
			}
			
			// ce nom d'element doit etre valide
			if (QET::containsForbiddenCharacters(element_name)) {
				QET::MessageBox::critical(
					dialog_,
					tr("Nom invalide", "message box title"),
					QString(
						tr(
							"Vous ne pouvez pas utiliser les caract\350res "
							"suivants dans le nom de l'\351l\351ment : %1"
						)
					).arg(QET::forbiddenCharactersString(true))
				);
				return;
			}
			
			// ajoute .elmt a la fin du nom si necessaire
			if (!element_name.endsWith(".elmt", Qt::CaseInsensitive)) {
				element_name += ".elmt";
			}
			final_location.addToPath(element_name);
		}
		
		// determine si l'element n'existe pas deja
		bool element_already_exists = (
			item -> isElement() ||\
			QETApp::collectionItem(final_location)
		);
		
		// si l'element existe, on demande confirmation pour son ecrasement
		if (element_already_exists) {
			QMessageBox::StandardButton answer = QET::MessageBox::question(
				dialog_,
				tr("\311craser l'\351l\351ment ?", "message box title"),
				tr("L'\351l\351ment existe d\351j\340. Voulez-vous l'\351craser ?", "message box content"),
				QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
				QMessageBox::No
			);
			if (answer != QMessageBox::Yes) return;
		}
		
		location_ = final_location;
	} else if (mode_ == OpenCategory) {
		// l'utilisateur doit avoir choisi une categorie ou une collection existante
		/// @todo effectuer les verifications necessaires
	} else if (mode_ == SaveCategory) {
		// l'utilisateur doit avoir choisi une categorie inexistante
		/// @todo effectuer les verifications necessaires
	}
	
	// le dialogue est verifie, il peut etre accepte
	dialog_ -> accept();
}

/**
	Slot execute apres la validation du dialogue par l'utilisateur
*/
void ElementDialog::dialogAccepted() {
}

/**
	Gere le rejet du dialogue par l'utilisateur.
*/
void ElementDialog::dialogRejected() {
	location_ = ElementsLocation();
}
