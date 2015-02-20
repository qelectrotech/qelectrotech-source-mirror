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
#include "integrationmoveelementshandler.h"
#include <QtGui>
#include "elementscategory.h"
#include "elementdefinition.h"
#include "qfilenameedit.h"

/**
	Constructeur
	@param parent QWidget parent a utiliser pour l'affichage des dialogues lors
	des interactions avec l'utilisateur
*/
IntegrationMoveElementsHandler::IntegrationMoveElementsHandler(QWidget *parent) :
	BasicMoveElementsHandler(parent),
	parent_widget_(parent),
	integ_dialog_(0)
{
	// actions par defaut : abort
	setActionIfItemAlreadyExists(QET::Abort);
	setActionIfItemIsNotReadable(QET::Abort);
	setActionIfItemIsNotWritable(QET::Abort);
	setActionIfItemTriggersAnError(QET::Abort);
}

/**
	Destructeur
*/
IntegrationMoveElementsHandler::~IntegrationMoveElementsHandler() {
}

/**
	@param src Element source
	@param dst Element cible / destination
	@return l'action a effectuer si l'element cible existe deja
*/
QET::Action IntegrationMoveElementsHandler::elementAlreadyExists(ElementDefinition *src, ElementDefinition *dst) {
	// premiere etape : on verifie si src et dst ne sont pas identiques
	if (src -> equals(*dst)) {
		// les deux elements sont identiques - il est inutile d'ecraser l'ancien
		return(QET::Ignore);
	}
	
	// les deux elements sont differents - on demande a l'utilisateur ce qu'il
	// prefere : ecrasement ou cohabitation
	return(askUser(src, dst));
}

/**
	@return le nom a utiliser pour le renommage si une methode de cet objet
	a precedemment renvoye QET::Rename.
*/
QString IntegrationMoveElementsHandler::nameForRenamingOperation() {
	return(rename_);
}

/**
	@return la date courante au format yyyyMMddhhmmss
*/
QString IntegrationMoveElementsHandler::dateString() const {
	return(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
}

/**
	@param element Une definition d'element
	@return un nom pour dupliquer l'element passe en parametre. Ce nom est base
	sur la date courante.
*/
QString IntegrationMoveElementsHandler::newNameForElement(const ElementDefinition *element) {
	QString orig_name = element -> pathName();
	if (orig_name.endsWith(".elmt")) orig_name.chop(5);
	return(orig_name + "-" + dateString() + ".elmt");
}

/**
	Demande a l'utilisateur s'il souhaite ecraser l'element deja existant,
	renommer le nouveau ou bien annuler
	@param src Element source
	@param dst Element cible
	@return la reponse de l'utilisateur
*/
QET::Action IntegrationMoveElementsHandler::askUser(ElementDefinition *src, ElementDefinition *dst) {
	Q_UNUSED(src);
	initDialog();
	int result = integ_dialog_ -> exec();
	if (result == QDialog::Accepted) {
		if (use_existing_elmt_ -> isChecked()) {
			return(QET::Ignore);
		} else if (erase_element_ -> isChecked()) {
			return(QET::Erase);
		} else {
			rename_ = newNameForElement(dst);
			return(QET::Rename);
		}
	} else {
		return(QET::Abort);
	}
}

/**
	Initialise le dialogue
*/
void IntegrationMoveElementsHandler::initDialog() {
	if (integ_dialog_) return;
	integ_dialog_ = new QDialog(parent_widget_);
	integ_dialog_ -> setWindowTitle(tr("Int\351gration d'un \351l\351ment"));
	
	dialog_label_ = new QLabel(
		QString(
			tr(
				"L'\351l\351ment a d\351j\340 \351t\351 "
				"int\351gr\351 dans le projet. Toutefois, la version que vous "
				"tentez de poser semble diff\351rente. Que souhaitez-vous "
				"faire ?",
				"dialog content - %1 is an element's path name"
			)
		)
	);
	
	use_existing_elmt_ = new QRadioButton(
		QString(
			tr(
				"Utiliser l'\351l\351ment d\351j\340 int\351gr\351",
				"dialog content"
			)
		)
	);
	
	integrate_new_element_ = new QRadioButton(
		QString(
			tr(
				"Int\351grer l'\351l\351ment d\351pos\351",
				"dialog content"
			)
		)
	);
	radioButtonleftMargin(integrate_new_element_);
	
	erase_element_ = new QRadioButton(
		QString(
			tr(
				"\311craser l'\351l\351ment d\351j\340 int\351gr\351",
				"dialog content"
			)
		)
	);
	radioButtonleftMargin(erase_element_);
	
	integrate_both_ = new QRadioButton(
		QString(
			tr(
				"Faire cohabiter les deux \351l\351ments",
				"dialog content"
			)
		)
	);
	
	button_group1_ = new QButtonGroup(this);
	button_group1_ -> addButton(use_existing_elmt_);
	button_group1_ -> addButton(integrate_new_element_);
	button_group2_ = new QButtonGroup(this);
	button_group2_ -> addButton(erase_element_);
	button_group2_ -> addButton(integrate_both_);
	
	integrate_new_element_ -> setChecked(true);
	integrate_both_ -> setChecked(true);
	
	buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	
	dialog_glayout = new QGridLayout();
	dialog_glayout -> setColumnMinimumWidth(0, 20);
	dialog_glayout -> addWidget(erase_element_,  0, 1);
	dialog_glayout -> addWidget(integrate_both_, 1, 1);
	
	dialog_vlayout_ = new QVBoxLayout(integ_dialog_);
	dialog_vlayout_ -> addWidget(dialog_label_);
	dialog_vlayout_ -> addWidget(use_existing_elmt_);
	dialog_vlayout_ -> addWidget(integrate_new_element_);
	dialog_vlayout_ -> addLayout(dialog_glayout);
	dialog_vlayout_ -> addWidget(buttons_);
	
	connect(use_existing_elmt_,     SIGNAL(toggled(bool)), this,           SLOT(correctRadioButtons()));
	connect(integrate_new_element_, SIGNAL(toggled(bool)), this,           SLOT(correctRadioButtons()));
	connect(buttons_,               SIGNAL(accepted()),    integ_dialog_, SLOT(accept()));
	connect(buttons_,               SIGNAL(rejected()),    integ_dialog_, SLOT(reject()));
}

/**
	S'asure que le dialogue reste coherent
*/
void IntegrationMoveElementsHandler::correctRadioButtons() {
	erase_element_  -> setEnabled(integrate_new_element_ -> isChecked());
	integrate_both_ -> setEnabled(integrate_new_element_ -> isChecked());
}

/**
	@param button bouton radio
	Augmente la marge gauche d'un bouton radio
*/
void IntegrationMoveElementsHandler::radioButtonleftMargin(QRadioButton *button) {
	int a, b, c, d;
	button -> getContentsMargins(&a, &b, &c, &d);
	button -> setContentsMargins(a + 15, b, c, d);
}
