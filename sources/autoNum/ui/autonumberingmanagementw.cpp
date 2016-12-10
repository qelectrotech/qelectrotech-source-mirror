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
#include "autonumberingmanagementw.h"
#include "ui_autonumberingmanagementw.h"
#include "numparteditorw.h"
#include <QMessageBox>
#include "numerotationcontextcommands.h"
#include "formulaautonumberingw.h"
#include "ui_formulaautonumberingw.h"
#include "qdebug.h"
#include "qetproject.h"
#include "diagram.h"

/**
 * Constructor
 */
AutoNumberingManagementW::AutoNumberingManagementW(QETProject *project, QWidget *parent) :
	QWidget(parent),
	project_(project)
{
	ui = new Ui::AutoNumberingManagementW;
	ui->setupUi(this);
	ui->m_apply_locations_rb->setHidden(true);
	ui->m_selected_locations_le->setHidden(true);
	ui->folioWidget->setHidden(true);
	ui->m_selected_folios_widget->setDisabled(true);
	ui->m_selected_folios_le->setDisabled(true);
	ui->m_selected_folios_le->setReadOnly(true);
	ui->m_apply_project_rb->setChecked(true);
	setProjectContext();
}

/**
 * Destructor
 */
AutoNumberingManagementW::~AutoNumberingManagementW()
{
	delete ui;
}

/**
 * @brief AutoNumberingManagementW::setProjectContext
 * Add Default Project Status
 */
void AutoNumberingManagementW::setProjectContext() {
	ui->m_status_cb->addItem(tr("Under Development"));
	ui->m_status_cb->addItem(tr("Installing"));
	ui->m_status_cb->addItem(tr("Built"));
}

/**
 * @brief AutoNumberingManagementW::on_m_status_cb_currentIndexChanged
 * Load Default Status Options
 */
void AutoNumberingManagementW::on_m_status_cb_currentIndexChanged(int index) {

	//Under Development
	if (index == 0) {
		ui->conductorWidget->setEnabled(true);
		ui->elementWidget->setEnabled(true);
		ui->folioWidget->setEnabled(true);
		ui->m_both_conductor_rb->setChecked(true);
		ui->m_both_element_rb->setChecked(true);
		ui->m_both_folio_rb->setChecked(true);
	}
	//Installing
	else if (index == 1) {
		ui->conductorWidget->setEnabled(true);
		ui->elementWidget->setEnabled(true);
		ui->folioWidget->setEnabled(true);
		ui->m_new_conductor_rb->setChecked(true);
		ui->m_new_element_rb->setChecked(true);
		ui->m_new_folio_rb->setChecked(true);
	}
	//Built
	else if (index == 2) {
		ui->m_disable_conductor_rb->setChecked(true);
		ui->m_disable_element_rb->setChecked(true);
		ui->m_disable_folio_rb->setChecked(true);
	}
}

/**
 * @brief AutoNumberingManagementW::on_m_apply_folios_rb_clicked
 * Set From Folios Combobox
 */
void AutoNumberingManagementW::on_m_apply_folios_rb_clicked() {
	if (ui->m_apply_folios_rb->isChecked()) {
		ui->m_selected_folios_widget->setEnabled(true);
		ui->m_selected_folios_le->setEnabled(true);
		if (ui->m_from_folios_cb->count()<=0) {
			ui->m_from_folios_cb->clear();
			ui->m_from_folios_cb->addItem("");
			foreach (Diagram *diagram, project_->diagrams()){
				if (diagram->title() != "")
					ui->m_from_folios_cb->addItem(diagram->title(),diagram->folioIndex());
				else ui->m_from_folios_cb->addItem(QString::number(diagram->folioIndex()),diagram->folioIndex());
			}
		}
		if (ui->m_from_folios_cb->currentIndex() > 0)
		applyEnable(true);
		else applyEnable(false);
	}
}

/**
 * @brief AutoNumberingManagementW::on_m_from_folios_cb_currentIndexChanged
 * Set To Folios Combobox
 */
void AutoNumberingManagementW::on_m_from_folios_cb_currentIndexChanged(int index) {
	ui->m_to_folios_cb->clear();
	ui->m_selected_folios_le->clear();
	ui->m_selected_folios_le->setEnabled(true);
	if (index > 0) {
		ui->m_to_folios_cb->setEnabled(true);
		ui->m_to_folios_cb->addItem("");
		for (int i=index;i<project_->diagrams().size();i++) {
				if (project_->diagrams().at(i)->title() != "") {
					ui->m_to_folios_cb->addItem(project_->diagrams().at(i)->title(),project_->diagrams().at(i)->folioIndex());
				}
				else ui->m_to_folios_cb->addItem(QString::number(project_->diagrams().at(i)->folioIndex()),project_->diagrams().at(i)->folioIndex());
		}
		applyEnable(true);
		ui->m_selected_folios_le->clear();
		ui->m_selected_folios_le->insert(ui->m_from_folios_cb->currentText());
	}
	else applyEnable(false);
}

/**
 * @brief AutoNumberingManagementW::on_m_to_folios_cb_currentIndexChanged
 * Set selected folios Line Edit content
 */
void AutoNumberingManagementW::on_m_to_folios_cb_currentIndexChanged(int index) {
	if (index > 0) {
		QString from = ui->m_from_folios_cb->currentText();
		QString to = ui->m_to_folios_cb->currentText();
		ui->m_selected_folios_le->clear();
		ui->m_selected_folios_le->insert(from + " - " + to);
		ui->m_selected_folios_le->setDisabled(true);
	}
	applyEnable(true);
}

/**
 * @brief AutoNumberingManagementW::on_m_apply_project_rb_clicked
 * Disable folio widget
 */
void AutoNumberingManagementW::on_m_apply_project_rb_clicked() {
	ui->m_selected_folios_widget->setDisabled(true);
	ui->m_selected_folios_le->setDisabled(true);
	applyEnable(true);
}

/**
 * @brief AutoNumberingManagementW::on_buttonBox_clicked
 * Action on @buttonBox clicked
 */
void AutoNumberingManagementW::on_buttonBox_clicked(QAbstractButton *button) {
	//transform button to int
	int answer = ui -> buttonBox -> buttonRole(button);
	switch (answer) {
			//apply the context in the diagram displayed by @diagram_chooser.
		case QDialogButtonBox::ApplyRole:
			applyEnable(false);
			emit applyPressed();
			break;
		case QDialogButtonBox::HelpRole:
			QMessageBox::information(this, tr("Auto Numbering Management", "title window"),
									tr("In this Menu you can set whether you want the Auto Numberings to be updated or not."
									   " For Element Auto Numbering you have 4 options of Update Policy:\n"
									   "-Both: both New and Existent Element labels will be updated. This is the default option.\n"
									   "-Update Only New: only new created Elements will be updated. Existent Element labels will be frozen.\n"
									   "-Update Only Existent: only existent Elements will be updated. New Elements will be assigned "
									   "their formula but will not update once created.\n"
									   "-Disable: both New and Existent Element labels will not be updated. This is valid for new folios as well.\n"
									   "Note: These options DO NOT allow or block Auto Numberings, only their Update Policy."
									   ));
			break;
	}
}

/**
 * @brief AutoNumberingManagementW::applyEnable
 * enable/disable the apply button
 */
void AutoNumberingManagementW::applyEnable(bool b) {
	if (b){
		bool valid= true;
		if (ui->m_apply_project_rb->isChecked())
			ui -> buttonBox -> button(QDialogButtonBox::Apply) -> setEnabled(valid);
		else if (ui->m_apply_folios_rb->isChecked())
			ui -> buttonBox -> button(QDialogButtonBox::Apply) -> setEnabled(valid);
		}
	else {
		ui -> buttonBox -> button(QDialogButtonBox::Apply) -> setEnabled(b);
	}
}
