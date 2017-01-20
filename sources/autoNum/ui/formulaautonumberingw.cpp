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
#include "formulaautonumberingw.h"
#include "ui_formulaautonumberingw.h"
#include <QMessageBox>
#include <QPushButton>
#include <QtWidgets>

/**
 * Constructor
 */
FormulaAutonumberingW::FormulaAutonumberingW(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::FormulaAutonumberingW)

{
	ui->setupUi(this);
	setContext(formula_);
}

/**
 * Destructor
 */
FormulaAutonumberingW::~FormulaAutonumberingW()
{
	delete ui;
}

/**
 * @brief FormulaAutonumberingW::setContext
 * @param formula to be inserted into context
 */
void FormulaAutonumberingW::setContext(QString formula) {
	ui->m_formula_le->insert(formula);
}

/**
 * @brief FormulaAutonumberingW::clearContext
 * @param clear formula line edit text
 */
void FormulaAutonumberingW::clearContext() {
	ui->m_formula_le->clear();
}

/**
 * @brief FormulaAutonumberingW::formula
 * @return formula to be stored into project
 */
QString FormulaAutonumberingW::formula() {
	return ui->m_formula_le->text();
}

/**
 * @brief FormulaAutonumberingW::on_m_formula_le_textChanged
 * Update Apply Button
 */
void FormulaAutonumberingW::on_m_formula_le_textChanged(QString text) {
	emit (textChanged(text));
}
