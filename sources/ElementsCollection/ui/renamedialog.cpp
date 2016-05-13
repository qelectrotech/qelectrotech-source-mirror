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
#include "renamedialog.h"
#include "ui_renamedialog.h"

RenameDialog::RenameDialog(QString path, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RenameDialog),
    m_path(path)
{
    ui->setupUi(this);
    m_name = m_path.split("/").last();
	if (m_name.endsWith(".elmt")) m_name.remove(".elmt");
	ui->m_label->setText(tr("L'élément « %1 » existe déjà. Que souhaitez-vous faire ?").arg(m_path));
	ui->lineEdit->setText(m_name + QDate::currentDate().toString("dd-MM-yy"));
}

RenameDialog::~RenameDialog()
{
    delete ui;
}

void RenameDialog::on_lineEdit_textEdited(const QString &arg1)
{
    if (arg1.isEmpty() || (arg1 == m_name))
		ui->m_rename_pb->setDisabled(true);
    else
		ui->m_rename_pb->setEnabled(true);
}

void RenameDialog::on_m_erase_pb_clicked()
{
	m_action = QET::Erase;
	accept();
}

void RenameDialog::on_m_rename_pb_clicked()
{
	m_action = QET::Rename;
	m_new_name = ui->lineEdit->text();
	if (m_path.endsWith(".elmt")) m_new_name.append(".elmt");
	accept();
}

void RenameDialog::on_m_cancel_pb_clicked()
{
	m_action = QET::Abort;
	reject();
}
