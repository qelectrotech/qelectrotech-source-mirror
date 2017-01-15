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
#include "formulaassistantdialog.h"
#include "ui_formulaassistantdialog.h"

#include <QPushButton>

FormulaAssistantDialog::FormulaAssistantDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::FormulaAssistantDialog)
{
	ui->setupUi(this);
}

FormulaAssistantDialog::~FormulaAssistantDialog()
{
	delete ui;
}

void FormulaAssistantDialog::setForbiddenVariables(QStringList list)
{
	m_rx.setPattern(list.join("|"));
}

void FormulaAssistantDialog::setText(QString text)
{
	ui->m_label->setText(text);
}

void FormulaAssistantDialog::setFormula(QString text)
{
	ui->m_line_edit->setText(text);
}

QString FormulaAssistantDialog::formula() const
{
	return ui->m_line_edit->text();
}

void FormulaAssistantDialog::on_m_line_edit_textChanged(const QString &arg1)
{
	QPushButton *b = ui->m_button_box->button(QDialogButtonBox::Ok);

	if (arg1.contains(m_rx))
		b->setDisabled(true);
	else
		b->setEnabled(true);
}
