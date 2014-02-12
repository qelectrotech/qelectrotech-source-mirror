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
#include "elementinfopartwidget.h"
#include "ui_elementinfopartwidget.h"


/**
 * @brief ElementInfoPartWidget::ElementInfoPartWidget
 * Constructor
 * @param key the string key what represent this info part
 * @param translated_key the string key translated
 * @param parent parent widget
 */
ElementInfoPartWidget::ElementInfoPartWidget(QString key, QString translated_key, QWidget *parent):
	QWidget(parent),
	ui(new Ui::ElementInfoPartWidget),
	key_(key)
{
	ui->setupUi(this);
	ui->label_->setText(translated_key);
	if(key == "label") ui->checkBox->setChecked(true);
}

/**
 * @brief ElementInfoPartWidget::~ElementInfoPartWidget
 * destructor
 */
ElementInfoPartWidget::~ElementInfoPartWidget()
{
	delete ui;
}

/**
 * @brief ElementInfoPartWidget::setText
 * Set text to line edit
 * @param txt
 */
void ElementInfoPartWidget::setText(const QString &txt) {
	ui->line_edit->setText(txt);
}

/**
 * @brief ElementInfoPartWidget::text
 * @return the text in the line edit
 */
QString ElementInfoPartWidget::text() const {
	return (ui->line_edit->text());
}

/**
 * @brief ElementInfoPartWidget::mustShow
 * @return return true if the value must be show, else false
 */
bool ElementInfoPartWidget::mustShow() const  {
	return (ui->checkBox->isChecked());
}

void ElementInfoPartWidget::setShow(bool s) {
	ui->checkBox->setChecked(s);
}
