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
#include "elementautonumberingw.h"
#include "ui_elementautonumberingw.h"
#include <QMessageBox>
#include <QPushButton>
#include <QtWidgets>

/**
 * Constructor
 */
ElementAutonumberingW::ElementAutonumberingW(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ElementAutonumberingW)

{
	ui->setupUi(this);
	setContext(formula_);
}

/**
 * Destructor
 */
ElementAutonumberingW::~ElementAutonumberingW()
{
	delete ui;
}

/**
 * @brief ElementAutonumberingW::setContext
 * @param formula to be inserted into context
 */
void ElementAutonumberingW::setContext(QString formula) {
	ui->m_formula_le->insert(formula);
}

/**
 * @brief ElementAutonumberingW::clearContext
 * @param clear formula line edit text
 */
void ElementAutonumberingW::clearContext() {
	ui->m_formula_le->clear();
}

/**
 * @brief ElementAutonumberingW::formula
 * @return formula to be stored into project
 */
QString ElementAutonumberingW::formula() {
	return ui->m_formula_le->text();
}

/**
 * @brief ElementAutonumberingW::on_m_formula_le_textChanged
 * Update Apply Button
 */
void ElementAutonumberingW::on_m_formula_le_textChanged(QString text) {
	emit (textChanged(text));
}
