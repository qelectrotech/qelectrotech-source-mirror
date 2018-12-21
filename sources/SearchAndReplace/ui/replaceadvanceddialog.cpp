/*
	Copyright 2006-2018 The QElectroTech Team
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
#include "replaceadvanceddialog.h"
#include "ui_replaceadvanceddialog.h"
#include "qetapp.h"

#include <QAbstractButton>

/**
 * @brief replaceAdvancedDialog::replaceAdvancedDialog
 * @param advanced
 * @param parent
 */
replaceAdvancedDialog::replaceAdvancedDialog(advancedReplaceStruct advanced, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::replaceAdvancedDialog)
{
	ui->setupUi(this);
	
	connect(ui->m_button_box, &QDialogButtonBox::clicked, [this](QAbstractButton *button_) {
		this->done(ui->m_button_box->buttonRole(button_));
	});
	
	fillWhatComboBox(ui->m_who_cb->currentIndex());
	ui->m_search_le->setFocus();
	setAdvancedStruct(advanced);
}

replaceAdvancedDialog::~replaceAdvancedDialog()
{
	delete ui;
}

/**
 * @brief replaceAdvancedDialog::setAdvancedStruct
 * Set the edited advanced struct
 * @param advanced
 */
void replaceAdvancedDialog::setAdvancedStruct(advancedReplaceStruct advanced)
{
	int index = advanced.who;
	if (index == -1) {
		return;
	}
	ui->m_who_cb->setCurrentIndex(index);

	for (int i=0 ; i < ui->m_what_cb->count() ; i++)
	{
		if (ui->m_what_cb->itemData(i).toString() == advanced.what)
		{
			ui->m_what_cb->setCurrentIndex(i);
			continue;
		}
	}
	
	ui->m_search_le->setText(advanced.search);
	ui->m_replace_le->setText(advanced.replace);
}

/**
 * @brief replaceAdvancedDialog::advancedStruct
 * @return the edited advanced struct
 */
advancedReplaceStruct replaceAdvancedDialog::advancedStruct() const
{
	advancedReplaceStruct a;
	a.who     = ui->m_who_cb->currentIndex();
	a.what    = ui->m_what_cb->currentData().toString();
	a.search  = ui->m_search_le->text();
	a.replace = ui->m_replace_le->text();
	
	return a;
}

void replaceAdvancedDialog::fillWhatComboBox(int index)
{
	ui->m_what_cb->clear();
	
	if (index == 0)
	{
		for (QString str : QETApp::diagramInfoKeys()) {
			ui->m_what_cb->addItem(QETApp::diagramTranslatedInfoKey(str), str);
		}
	}
	else if (index == 1) {
		for (QString str : QETApp::elementInfoKeys()) {
			ui->m_what_cb->addItem(QETApp::elementTranslatedInfoKey(str), str);
		}
	}
	else if (index == 2) {
		for (QString str : QETApp::conductorInfoKeys()) {
			ui->m_what_cb->addItem(QETApp::conductorTranslatedInfoKey(str), str);
		}
	}
}

/**
 * @brief replaceAdvancedDialog::on_m_who_cb_currentIndexChanged
 * @param index
 */
void replaceAdvancedDialog::on_m_who_cb_currentIndexChanged(int index) {
	fillWhatComboBox(index);
}
