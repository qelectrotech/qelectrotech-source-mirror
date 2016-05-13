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
#include "interactivemoveelementshandler.h"
#include <QtWidgets>
#include "elementscategory.h"
#include "elementdefinition.h"
#include "qfilenameedit.h"
#include "qetmessagebox.h"

/**
	Constructeur
	@param parent QWidget parent a utiliser pour l'affichage des dialogues lors
	des interactions avec l'utilisateur
*/
InteractiveMoveElementsHandler::InteractiveMoveElementsHandler(QWidget *parent) :
	BasicMoveElementsHandler(parent),
	parent_widget_(parent),
	rename_(""),
	always_erase_(false),
	always_skip_(false),
	aborted_(false),
	conflict_dialog_(0)
{
}

/**
	Destructeur
*/
InteractiveMoveElementsHandler::~InteractiveMoveElementsHandler() {
}

/**
	@param src Categorie source
	@param dst Categorie cible / destination
	@return l'action a effectuer si la categorie cible existe deja
*/
QET::Action InteractiveMoveElementsHandler::categoryAlreadyExists(ElementsCategory *src, ElementsCategory  *dst) {
	// verifie si la reponse n'est pas systematique
	if (aborted_)      return(QET::Abort);
	if (always_erase_) return(QET::Erase);
	if (always_skip_)  return(QET::Ignore);
	
	// a ce stade, l'action a effectuer pour gerer le conflit doit etre
	// demandee a l'utilisateur via un dialogue
	initConflictDialog();
	
	QString src_location(src -> location().toString());
	QString dst_location(dst -> location().toString());
	
	// prepare le dialogue
	QString dialog_title(QString(tr("Copie de %1 vers %2", "dialog title")).arg(src_location).arg(dst_location));
	
	QLabel *question_label = new QLabel(
		QString(
			tr(
				"La catégorie « %1 » (%2) existe déjà. "
				"Que souhaitez-vous faire ?",
				"dialog content"
			)
		)
		.arg(dst -> name())
		.arg(dst_location)
	);
	question_label -> setWordWrap(true);
	
	setConflictDialogTitle(dialog_title);
	setConflictDialogMainWidget(question_label);
	
	// execute le dialogue
	conflict_dialog_ -> exec();
	
	// enleve et detruit le widget principal
	setConflictDialogMainWidget(0);
	delete question_label;
	
	// renvoie la reponse obtenue via le dialogue
	return(conflict_result_);
}

/**
	@param src Element source
	@param dst Element cible / destination
	@return l'action a effectuer si l'element cible existe deja
*/
QET::Action InteractiveMoveElementsHandler::elementAlreadyExists(ElementDefinition *src, ElementDefinition *dst) {
	// verifie si la reponse n'est pas systematique
	if (aborted_)      return(QET::Abort);
	if (always_erase_) return(QET::Erase);
	if (always_skip_)  return(QET::Ignore);
	
	// a ce stade, l'action a effectuer pour gerer le conflit doit etre
	// demandee a l'utilisateur via un dialogue
	initConflictDialog();
	
	QString src_location(src -> location().toString());
	QString dst_location(dst -> location().toString());
	
	// prepare le dialogue
	QString dialog_title(QString(tr("Copie de %1 vers %2", "dialog title")).arg(src_location).arg(dst_location));
	
	QLabel *question_label = new QLabel(
		QString(
			tr(
				"L'élément « %1 » existe déjà. "
				"Que souhaitez-vous faire ?",
				"dialog content"
			)
		)
		.arg(dst_location)
	);
	question_label -> setWordWrap(true);
	
	setConflictDialogTitle(dialog_title);
	setConflictDialogMainWidget(question_label);
	
	// execute le dialogue
	conflict_dialog_ -> exec();
	
	// enleve et detruit le widget principal
	setConflictDialogMainWidget(0);
	delete question_label;
	
	if (conflict_result_ == QET::Rename) {
		if (!rename_.endsWith(".elmt")) rename_ += ".elmt";
	}
	
	// renvoie la reponse obtenue via le dialogue
	return(conflict_result_);
}

/**
	Cette methode permet de savoir comment agir lorsqu'une categorie n'est pas lisible
	@param category La categorie qui n'est pas lisible
	@return QET::Retry, QET::Ignore ou QET::Abort
*/
QET::Action InteractiveMoveElementsHandler::categoryIsNotReadable(ElementsCategory *category) {
	QString message = QString(tr("La catégorie %1 n'est pas accessible en lecture.", "message box content")).arg(category -> location().toString());
	return(retryErrorMessage(message));
}

/**
	Cette methode permet de savoir comment agir lorsqu'un element n'est pas lisible
	@param element L'element qui n'est pas lisible
	@return QET::Retry, QET::Ignore ou QET::Abort
*/
QET::Action InteractiveMoveElementsHandler::elementIsNotReadable(ElementDefinition *element) {
	QString message = QString(tr("L'élément %1 n'est pas accessible en lecture.", "message box content")).arg(element -> location().toString());
	return(retryErrorMessage(message));
}

/**
	Cette methode permet de savoir comment agir lorsqu'une categorie n'est pas accessible en ecriture
	@param category La categorie qui n'est pas lisible
	@return QET::Retry, QET::Ignore ou QET::Abort
*/
QET::Action InteractiveMoveElementsHandler::categoryIsNotWritable(ElementsCategory *category) {
	QString message = QString(tr("La catégorie %1 n'est pas accessible en écriture.", "message box content")).arg(category -> location().toString());
	return(retryErrorMessage(message));
}

/**
	Cette methode permet de savoir comment agir lorsqu'un element n'est pas accessible en ecriture
	@param element L'element qui n'est pas lisible
	@return QET::Retry, QET::Ignore ou QET::Abort
*/
QET::Action InteractiveMoveElementsHandler::elementIsNotWritable(ElementDefinition *element) {
	QString message = QString(tr("L'élément %1 n'est pas accessible en écriture.", "message box content")).arg(element -> location().toString());
	return(retryErrorMessage(message));
}

/**
	Affiche un message d'erreur relatif a une categorie
	@param category La categorie concernee par l'erreur
	@param message Le message d'erreur a afficher
	@return toujours QET::Ignore
*/
QET::Action InteractiveMoveElementsHandler::errorWithACategory(ElementsCategory *category, const QString &message) {
	QString category_location = category -> location().toString();
	QString error_message = QString("Une erreur s'est produite avec la catégorie %1 : %2").arg(category_location).arg(message);
	simpleErrorMessage(error_message);
	return(QET::Ignore);
}

/**
	Affiche un message d'erreur relatif a un element
	@param element L'element concerne par l'erreur
	@param message Le message d'erreur a afficher
	@return toujours QET::Ignore
*/
QET::Action InteractiveMoveElementsHandler::errorWithAnElement(ElementDefinition *element, const QString &message) {
	QString element_location = element -> location().toString();
	QString error_message = QString("Une erreur s'est produite avec l'élément %1 : %2").arg(element_location).arg(message);
	simpleErrorMessage(error_message);
	return(QET::Ignore);
}

/**
	@return le nom a utiliser pour le renommage si une methode de cet objet
	a precedemment renvoye QET::Rename.
*/
QString InteractiveMoveElementsHandler::nameForRenamingOperation() {
	return(rename_);
}

/**
	Initialise le dialogue qui sera utilise pour les conflits
	elements / categories.
*/
void InteractiveMoveElementsHandler::initConflictDialog() {
	// n'agit qu'une seule fois
	if (conflict_dialog_) return;
	
	conflict_dialog_ = new QDialog(parent_widget_);
	conflict_dialog_ -> setMaximumSize(600, 200);
	
	// initialisation du champ de texte
	rename_label_ = new QLabel(tr("Nouveau nom :"));
	rename_textfield_ = new QFileNameEdit();
	connect(
		rename_textfield_,
		SIGNAL(textEdited(const QString &)),
		this,
		SLOT(conflictDialogFileNameFieldChanged())
	);
	
	// initialisation des boutons
	rename_button_     = new QPushButton(tr("Renommer"));
	erase_button_      = new QPushButton(tr("Écraser"));
	erase_all_button_  = new QPushButton(tr("Écraser tout"));
	ignore_button_     = new QPushButton(tr("Ignorer"));
	ignore_all_button_ = new QPushButton(tr("Ignorer tout"));
	abort_button_      = new QPushButton(tr("Annuler"));
	
	conflict_buttons_ = new QDialogButtonBox();
	conflict_buttons_ -> addButton(rename_button_,       QDialogButtonBox::ActionRole);
	conflict_buttons_ -> addButton(erase_button_,        QDialogButtonBox::AcceptRole);
	conflict_buttons_ -> addButton(erase_all_button_,    QDialogButtonBox::AcceptRole);
	conflict_buttons_ -> addButton(ignore_button_,       QDialogButtonBox::AcceptRole);
	conflict_buttons_ -> addButton(ignore_all_button_,   QDialogButtonBox::AcceptRole);
	conflict_buttons_ -> addButton(abort_button_,        QDialogButtonBox::AcceptRole);
	
	rename_button_ -> setEnabled(false);
	connect(
		conflict_buttons_,
		SIGNAL(clicked(QAbstractButton *)),
		this,
		SLOT(conflictDialogButtonClicked(QAbstractButton *))
	);
	
	// layout
	conflict_layout1_ = new QHBoxLayout();
	conflict_layout1_ -> addWidget(rename_label_);
	conflict_layout1_ -> addWidget(rename_textfield_);
	
	conflict_layout0_ = new QVBoxLayout(conflict_dialog_);
	conflict_layout0_ -> insertLayout(1, conflict_layout1_);
	conflict_layout0_ -> insertWidget(2, conflict_buttons_);
}

/**
	Slot appele lorsque l'utilisateur modifie le contenu du champ 
*/
void InteractiveMoveElementsHandler::conflictDialogFileNameFieldChanged() {
	if (rename_textfield_ -> isValid()) {
		/// @todo verifier que le nom n'est pas deja pris
		rename_button_ -> setEnabled(true);
	} else {
		rename_button_ -> setEnabled(false);
	}
}

/**
	Slot appele lorsque l'utilisateur presse un des boutons du dialogue de
	conflit.
	@param button Bouton presse par l'utilisateur
*/
void InteractiveMoveElementsHandler::conflictDialogButtonClicked(QAbstractButton *button) {
	conflict_dialog_ -> accept();
	// change la valeur de l'attribut 
	if (button == rename_button_) {
		rename_ = rename_textfield_ -> text();
		conflict_result_= QET::Rename;
	} else if (button == erase_button_) {
		conflict_result_= QET::Erase;
	} else if (button == erase_all_button_) {
		always_erase_ = true;
		conflict_result_= QET::Erase;
	} else if (button == ignore_button_) {
		conflict_result_= QET::Ignore;
	} else if (button == ignore_all_button_) {
		always_skip_ = true;
		conflict_result_= QET::Ignore;
	} else if (button == abort_button_) {
		aborted_ = true;
		conflict_result_= QET::Abort;
	}
}

/**
	Change le titre du dialogue de conflit
	@param new_title Nouveau titre pour le dialogue de conflit
*/
void InteractiveMoveElementsHandler::setConflictDialogTitle(const QString &new_title) {
	conflict_dialog_ -> setWindowTitle(new_title);
}

/**
	@return le titre du dialogue de conflit
*/
QString InteractiveMoveElementsHandler::conflictDialogTitle() const {
	return(conflict_dialog_ -> windowTitle());
}

/**
	Change le widget affiche au centre du dialogue de conflit
	@param widget Widget a inserer dans le dialogue de conflit
	Si widget vaut 0, le widget central est retire.
*/
void InteractiveMoveElementsHandler::setConflictDialogMainWidget(QWidget *widget) {
	// gere l'enlevement du widget principal
	if (!widget) {
		if (conflict_layout0_ -> count() != 3) return;
		conflict_layout0_ -> removeItem(conflict_layout0_ -> itemAt(0));
	} else {
		conflict_layout0_ -> insertWidget(0, widget);
	}
}

/**
	@return le widget insere dans le dialogue de conflit, ou 0 s'il n'y en a
	aucun.
*/
QWidget *InteractiveMoveElementsHandler::conflictDialogMainWidget() const {
	if (conflict_layout0_ -> count() != 3) return(0);
	return(conflict_layout0_ -> itemAt(0) -> widget());
}

/**
	Affiche un message d'erreur en donnant la possibilite d'ignorer l'item en cours,
	d'annuler tout le mouvement ou de le reessayer.
	@param message Message d'erreur a afficher
	@return L'action choisie par l'utilisateur
*/
QET::Action InteractiveMoveElementsHandler::retryErrorMessage(const QString &message) const {
	int todo = QET::QetMessageBox::critical(
		parent_widget_,
		tr("Erreur", "message box title"),
		message,
		QMessageBox::Abort | QMessageBox::Retry | QMessageBox::Ignore,
		QMessageBox::Ignore
	);
	
	if (todo == QMessageBox::Abort) {
		return(QET::Abort);
	} else if (todo == QMessageBox::Retry) {
		return(QET::Retry);
	} else {
		return(QET::Ignore);
	}
}

/**
	Affiche un simple message d'erreur
	@param message Message d'erreur a afficher
*/
void InteractiveMoveElementsHandler::simpleErrorMessage(const QString &message) const {
	QET::QetMessageBox::critical(
		parent_widget_,
		tr("Erreur", "message box title"),
		message,
		QMessageBox::Ok,
		QMessageBox::Ok
	);
}
