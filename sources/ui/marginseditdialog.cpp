/*
	Copyright 2006-2020 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "marginseditdialog.h"
#include "ui_marginseditdialog.h"

#include <QScopedPointer>

MarginsEditDialog::MarginsEditDialog(QMargins margins, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::MarginsEditDialog)
{
	ui->setupUi(this);
	ui->m_top_sb->setValue(margins.top());
	ui->m_left_sb->setValue(margins.left());
	ui->m_right_sb->setValue(margins.right());
	ui->m_bottom_sb->setValue(margins.bottom());
}

MarginsEditDialog::~MarginsEditDialog()
{
	delete ui;
}

QMargins MarginsEditDialog::margins() const
{
	return QMargins(ui->m_left_sb->value(),
			ui->m_top_sb->value(),
			ui->m_right_sb->value(),
			ui->m_bottom_sb->value());
}

/**
	@brief MarginsEditDialog::getMargins
	@param margins : margins to set by default
	@param accepted : bool to know if dialog is accepted
	@param parent : parent widget.
	@return The a margins with the edited value if dialog is accepted
	or a default constructed QMargins() if dialog is rejected
*/
QMargins MarginsEditDialog::getMargins(
		QMargins margins, bool *accepted, QWidget *parent)
{
	QScopedPointer<MarginsEditDialog> d(
				new MarginsEditDialog(margins, parent));
	if (d->exec())
	{
		if (accepted) {
			*accepted = true;
		}
		return d->margins();
	}

	if (accepted) {
		*accepted = false;
	}
	return QMargins();
}
