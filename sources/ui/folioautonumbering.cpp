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
#include "folioautonumbering.h"
#include "ui_folioautonumbering.h"
#include <QMessageBox>
#include <QPushButton>
#include <QtWidgets>
#include "qetproject.h"
#include "diagram.h"
/**
 * Constructor
 */
FolioAutonumberingW::FolioAutonumberingW(QETProject *project, QWidget *parent) :
	QWidget(parent),
	project_(project),
	ui(new Ui::FolioAutonumberingW)

{
	ui->setupUi(this);
	applyEnable(false);
	ui->m_from_cb->setEnabled(false);
	ui->m_new_tabs_sb->setEnabled(false);
	ui->m_to_cb->setEnabled(false);
}

/**
 * Destructor
 */
FolioAutonumberingW::~FolioAutonumberingW()
{
	delete ui;
}

/**
 * @brief FolioAutonumberingW::setContext
 * construct autonums in the comboBox selected in the @autonum_chooser QcomboBox
 */
void FolioAutonumberingW::setContext(QList <QString> autonums) {
	foreach (QString str, autonums) { ui->m_autonums_cb->addItem(str);}
}

/**
 * @brief FolioAutonumberingW::autoNumSelected
 * returns the current autonum selected
 */
QString FolioAutonumberingW::autoNumSelected(){
	return ui->m_autonums_cb->currentText();
}

/**
 * @brief FolioAutonumberingW::fromFolio
 * returns the current "From Folio" index
 */
int FolioAutonumberingW::fromFolio(){
	return ui->m_from_cb->currentIndex()-1;
}

/**
 * @brief FolioAutonumberingW::toFolio
 * returns the current "To Folio" index
 */
int FolioAutonumberingW::toFolio(){
	return ui->m_to_cb->currentIndex()+this->fromFolio()+1;
}

/**
 * @brief FolioAutonumberingW::newFoliosNumber
 * returns the number of folios to create
 */
int FolioAutonumberingW::newFoliosNumber(){
	return ui->m_new_tabs_sb->value();
}

/**
 * @brief FolioAutonumberingW::updateFolioList
 * update Folio List in From and To ComboBox
 */
void FolioAutonumberingW::updateFolioList(){
	ui   -> m_from_cb->clear();
	ui   -> m_to_cb->clear();
	if (newFolios){
		this -> on_m_create_new_tabs_rb_clicked();
	} else {
		this -> on_m_autonumber_tabs_rb_clicked();
	}
}

/**
 * @brief FolioAutonumberingW::on_m_create_new_tabs_rb_clicked
 *	Enable New Tabs SpinBox
 */
void FolioAutonumberingW::on_m_create_new_tabs_rb_clicked() {
	ui->m_from_cb->setEnabled(false);
	ui->m_to_cb->setEnabled(false);
	ui->m_new_tabs_sb->setEnabled(true);
	applyEnable();
	newFolios = true;
}

/**
 * @brief FolioAutonumberingW::on_m_autonumber_tabs_rb_clicked
 *	Enable From ComboBox, fill From ComboBox
 */
void FolioAutonumberingW::on_m_autonumber_tabs_rb_clicked() {
	ui->m_new_tabs_sb->setEnabled(false);
	ui->m_from_cb->setEnabled(true);
	ui->m_to_cb->setEnabled(true);
	if (ui->m_from_cb->count()<=0){
		ui->m_from_cb->clear();
		ui->m_from_cb->addItem("");
		foreach (Diagram *diagram, project_->diagrams()){
			ui->m_from_cb->addItem(diagram->title());
		}
	}
	applyEnable();
	newFolios = false;
}

/**
 * @brief FolioAutonumberingW::on_m_new_tabs_sb_valueChanged
 *	Enable Apply if any new folio is to be created
 */
void FolioAutonumberingW::on_m_new_tabs_sb_valueChanged(){
	if (ui->m_new_tabs_sb->value()>0) applyEnable(true);
	else applyEnable(false);
}

/**
 * @brief FolioAutonumberingW::on_m_from_cb_currentIndexChanged
 *	Enable To ComboBox
 */
void FolioAutonumberingW::on_m_from_cb_currentIndexChanged(){
	int index = ui->m_from_cb->currentIndex();
	ui->m_to_cb->clear();
	if (index > 0){
		ui->m_to_cb->setEnabled(true);
		for (int i=index;i<project_->diagrams().size();i++)
			ui->m_to_cb->addItem(project_->diagrams().at(i)->title());
		applyEnable(true);
	}
	else{
		applyEnable();
	}

}

/**
 * @brief SelectAutonumW::on_buttonBox_clicked
 * Action on @buttonBox clicked
 */
void FolioAutonumberingW::on_buttonBox_clicked(QAbstractButton *button) {
	//transform button to int
	int answer = ui -> buttonBox -> buttonRole(button);

	switch (answer) {
			//help dialog - not implemented yet -
		case QDialogButtonBox::HelpRole:
			QMessageBox::information (this, tr("Folio Autonumbering", "title window"),
																	tr("C'est ici que vous pouvez définir la manière dont sera numéroté les nouveaux folios.\n"
																	   "-Une numérotation est composée d'une variable minimum.\n"
																	   "-Vous pouvez ajouter ou supprimer une variable de numérotation par le biais des boutons - et +.\n"
																	   "-Une variable de numérotation comprant: un type, une valeur et une incrémentation.\n"

																	   "\n-les types \"Chiffre 1\", \"Chiffre 01\" et \"Chiffre 001\", représente un type numérique définie dans le champs \"Valeur\", "
																	   "qui s'incrémente à chaque nouveau folio de la valeur du champ \"Incrémentation\".\n"
																	   "-\"Chiffre 01\" et \"Chiffre 001\", sont respectivement représenté sur le schéma par deux et trois digits minimum.\n"
																	   "Si le chiffre définie dans le champs Valeur posséde moins de digits que le type choisit,"
																	   "celui-ci sera précédé par un ou deux 0 afin de respecter son type.\n"

																	   "\n-Le type \"Texte\", représente un texte fixe.\nLe champs \"Incrémentation\" n'est pas utilisé.\n",
																	   "help dialog about the folio autonumerotation"
																	   ));
			break;
		case QDialogButtonBox::ApplyRole:
			applyEnable(true);
			emit applyPressed();
			updateFolioList();
			break;
	}
}

/**
 * @brief SelectAutonumW::applyEnable
 * enable/disable the apply button
 */
void FolioAutonumberingW::applyEnable(bool b) {
	if (b){
		bool valid = true;
		if (ui->m_create_new_tabs_rb->isChecked()){
			if (ui->m_new_tabs_sb->value()==0) valid = false;
			ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(valid);
		}
		else{
			if (ui->m_to_cb->currentText()=="") valid = false;
			ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(valid);
		}
	}
	else{
		ui -> buttonBox -> button(QDialogButtonBox::Apply) -> setEnabled(b);
		}
}
