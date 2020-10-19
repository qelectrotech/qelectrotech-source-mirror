/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "configsaveloaderwidget.h"
#include "ui_configsaveloaderwidget.h"

ConfigSaveLoaderWidget::ConfigSaveLoaderWidget(QWidget *parent) :
	QGroupBox(parent),
	ui(new Ui::ConfigSaveLoaderWidget)
{
	ui->setupUi(this);
}

ConfigSaveLoaderWidget::~ConfigSaveLoaderWidget()
{
	delete ui;
}

QString ConfigSaveLoaderWidget::selectedText() const
{
	return ui->m_combo_box->currentText();
}

QString ConfigSaveLoaderWidget::text() const
{
	return ui->m_line_edit->text();
}

void ConfigSaveLoaderWidget::addItem(QString text) {
	ui->m_combo_box->addItem(text);
}

void ConfigSaveLoaderWidget::on_m_load_pb_clicked()
{
	emit loadClicked();
}

void ConfigSaveLoaderWidget::on_m_save_pb_clicked()
{
	emit saveClicked();
}
