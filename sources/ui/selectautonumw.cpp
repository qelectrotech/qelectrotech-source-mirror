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
#include "selectautonumw.h"
#include "ui_selectautonumw.h"
#include "numparteditorw.h"
#include <QMessageBox>
#include "qdebug.h"

/**
 * Constructor
 */
SelectAutonumW::SelectAutonumW(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SelectAutonumW)
{
	ui->setupUi(this);
	setContext(NumerotationContext());
}

SelectAutonumW::SelectAutonumW(const NumerotationContext &context, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SelectAutonumW)
{
	ui->setupUi(this);
	setContext(context);
}

/**
 * Destructor
 */
SelectAutonumW::~SelectAutonumW()
{
	delete ui;
}

/**
 * @brief SelectAutonumW::setCurrentContext
 * build the context of current diagram selected in the @diagram_chooser QcomboBox
 */
void SelectAutonumW::setContext(const NumerotationContext &context) {
	m_context = context;

	qDeleteAll(num_part_list_);
	num_part_list_.clear();

	if (m_context.size() == 0) { //@context contain nothing, build a default numPartEditor
		on_add_button_clicked();
	}
	else {
		for (int i=0; i<m_context.size(); ++i) { //build with the content of @context
			NumPartEditorW *part= new NumPartEditorW(m_context, i, this);
			connect (part, SIGNAL(changed()), this, SLOT(applyEnable()));
			num_part_list_ << part;
			ui -> editor_layout -> addWidget(part);
		}
	}

	num_part_list_.size() == 1 ?
				ui -> remove_button -> setDisabled(true):
				ui -> remove_button -> setEnabled (true);

	applyEnable(false);
}

/**
 * @brief SelectAutonumW::toNumContext
 * @return the content to @num_part_list to NumerotationContext
 */
NumerotationContext SelectAutonumW::toNumContext() const {
	NumerotationContext nc;
	foreach (NumPartEditorW *npew, num_part_list_) nc << npew -> toNumContext();
	return nc;
}

/**
 * @brief SelectAutonumW::on_add_button_clicked
 *	Action on add_button, add a @NumPartEditor
 */
void SelectAutonumW::on_add_button_clicked() {
	applyEnable(false);
	NumPartEditorW *part = new NumPartEditorW(this);
	connect (part, SIGNAL(changed()), this, SLOT(applyEnable()));
	num_part_list_ << part;
	ui -> editor_layout -> addWidget(part);
	ui -> remove_button -> setEnabled(true);
}

/**
 * @brief SelectAutonumW::on_remove_button_clicked
 *	Action on remove button, remove the last @NumPartEditor
 */
void SelectAutonumW::on_remove_button_clicked() {
	//remove if @num_part_list contains more than one item
	if (num_part_list_.size() > 1) {
		NumPartEditorW *part = num_part_list_.takeLast();
		disconnect(part, SIGNAL(changed()), this, SLOT(applyEnable()));
		delete part;
		if (num_part_list_.size() == 1) {
			ui -> remove_button -> setDisabled(true);
		}
	}
	applyEnable();
}

/**
 * @brief SelectAutonumW::on_buttonBox_clicked
 * Action on @buttonBox clicked
 */
void SelectAutonumW::on_buttonBox_clicked(QAbstractButton *button) {
	//transform button to int
	int answer = ui -> buttonBox -> buttonRole(button);

	switch (answer) {
			//Reset the curent context
		case QDialogButtonBox::ResetRole:
			setContext(m_context);
			break;
			//help dialog
		case QDialogButtonBox::HelpRole:
			QMessageBox::information (this, tr("Autonum\351rotation", "title window"),
																	tr("C'est ici que vous pouvez d\351finir la mani\350re dont sera num\351rot\351 les nouveaux conducteurs.\n"
																	   "-Chaque Folio poss\350de sa propre m\351thode de num\351rotation.\n"
																	   "-Une num\351rotation est compos\351e d'une variable minimum.\n"
																	   "-Vous pouvez ajouter ou supprimer une variable de num\351rotation par le biais des boutons - et +.\n"
																	   "-Une variable de num\351rotation comprant: un type, une valeur et une incr\351mentation.\n"

																	   "\n-les types \"Chiffre 1\", \"Chiffre 01\" et \"Chiffre 001\", repr\351sente un type num\351rique d\351finie dans le champs \"Valeur\", "
																	   "qui s'incr\351mente \340 chaque nouveau conducteur de la valeur du champ \"Incr\351mentation\".\n"
																	   "-\"Chiffre 01\" et \"Chiffre 001\", sont respectivement repr\351sent\351 sur le sch\351ma par deux et trois digits minimum.\n"
																	   "Si le chiffre d\351finie dans le champs Valeur poss\351de moins de digits que le type choisit,"
																	   "celui-ci sera pr\351c\351d\351 par un ou deux 0 afin de respecter son type.\n"

																	   "\n-Le type \"Texte\", repr\351sente un texte fixe.\nLe champs \"Incr\351mentation\" n'est pas utilis\351.\n"

																	   "\n-Le type \"N\260 folio\" repr\351sente le n\260 du folio en cours.\nLes autres champs ne sont pas utilis\351s.",
																	   "help dialog about the autonumerotation"));

			//apply the context in the diagram displayed by @diagram_chooser.
		case QDialogButtonBox::ApplyRole:
			applyEnable(false);
			emit applyPressed();
			break;
	};
}

/**
 * @brief SelectAutonumW::applyEnable
 * enable/disable the apply button
 */
void SelectAutonumW::applyEnable(bool b) {
	if (b){
		bool valid= true;
		foreach (NumPartEditorW *npe, num_part_list_) if (!npe -> isValid()) valid= false;
		ui -> buttonBox -> button(QDialogButtonBox::Apply) -> setEnabled(valid);
	}
	else
		ui -> buttonBox -> button(QDialogButtonBox::Apply) -> setEnabled(b);
}
