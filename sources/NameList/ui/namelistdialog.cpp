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
#include "namelistdialog.h"
#include "ui_namelistdialog.h"
#include "namelistwidget.h"

#include <QPushButton>
#include <QMessageBox>

NameListDialog::NameListDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::NameListDialog)
{
	ui->setupUi(this);

	m_namelist_widget = new NameListWidget(this);
	ui->m_main_layout->insertWidget(1, m_namelist_widget);
#ifdef Q_OS_MAC
	this->setWindowFlags(Qt::Sheet);
#endif
}

NameListDialog::~NameListDialog() {
	delete ui;
}

void NameListDialog::setInformationText(const QString &text) {
	ui->m_top_label->setText(text);
}

/**
 * @brief NameListDialog::namelistWidget
 * @return the name list widget used by this dialog.
 * The ownership of the namelistwidget stay to this dialog
 */
NameListWidget *NameListDialog::namelistWidget() const {
	return m_namelist_widget;
}

void NameListDialog::setHelpText(const QString &text)
{
	m_help_text = text;
	if (!m_help_text.isEmpty())
	{
		QPushButton *button = ui->m_button_box->addButton(QDialogButtonBox::Help);
		connect(button, &QPushButton::clicked, this, &NameListDialog::showHelpDialog);
	}
}

void NameListDialog::showHelpDialog() {
	QMessageBox::information(this, tr("Variables de cartouche"), m_help_text);
}
