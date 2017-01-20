/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "importelementdialog.h"
#include "ui_importelementdialog.h"

ImportElementDialog::ImportElementDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ImportElementDialog)
{
	ui->setupUi(this);
	setUpWidget();
}

ImportElementDialog::~ImportElementDialog()
{
	delete ui;
}

QET::Action ImportElementDialog::action() const
{
	if (ui->m_use_actual_rd->isChecked()) { return QET::Ignore; }
	else if (ui->m_erase_actual_rb->isChecked()) { return QET::Erase; }
	else if (ui->m_use_both_rb->isChecked()) { return QET::Rename; }
	else return QET::Abort;
}

void ImportElementDialog::setUpWidget()
{
	QButtonGroup *button_group = new QButtonGroup(this);
	button_group->addButton(ui->m_use_actual_rd);
	button_group->addButton(ui->m_use_drop_rb);
	QButtonGroup *button_group_drop = new QButtonGroup(this);
	button_group_drop->addButton(ui->m_erase_actual_rb);
	button_group_drop->addButton(ui->m_use_both_rb);

	ui->m_use_drop_rb->setChecked(true);
	ui->m_use_both_rb->setChecked(true);
}
