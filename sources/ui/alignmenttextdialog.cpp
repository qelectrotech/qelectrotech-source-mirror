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
#include "alignmenttextdialog.h"
#include "ui_alignmenttextdialog.h"
#include "dynamicelementtextitem.h"

AlignmentTextDialog::AlignmentTextDialog(Qt::Alignment alignment, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AlignmentTextDialog)
{
	ui->setupUi(this);
	
	if(alignment == (Qt::AlignTop|Qt::AlignLeft))
		ui->top_left->setChecked(true);
	else if(alignment == (Qt::AlignTop|Qt::AlignHCenter))
		ui->top->setChecked(true);
	else if(alignment == (Qt::AlignTop|Qt::AlignRight))
		ui->top_right->setChecked(true);
	else if(alignment == (Qt::AlignVCenter|Qt::AlignLeft))
		ui->left->setChecked(true);
	else if(alignment == Qt::AlignCenter)
		ui->center->setChecked(true);
	else if(alignment == (Qt::AlignVCenter|Qt::AlignRight))
		ui->right->setChecked(true);
	else if(alignment == (Qt::AlignBottom|Qt::AlignLeft))
		ui->bottom_left->setChecked(true);
	else if(alignment == (Qt::AlignBottom|Qt::AlignHCenter))
		ui->bottom->setChecked(true);
	else if(alignment == (Qt::AlignBottom|Qt::AlignRight))
		ui->bottom_right->setChecked(true);
}

AlignmentTextDialog::~AlignmentTextDialog()
{
	delete ui;
}

/**
 * @brief AlignmentTextDialog::alignment
 * @return the selected alignment
 */
Qt::Alignment AlignmentTextDialog::alignment() const
{
	if(ui->top_left->isChecked())
		return (Qt::AlignTop|Qt::AlignLeft);
	else if(ui->top->isChecked())
		return (Qt::AlignTop|Qt::AlignHCenter);
	else if(ui->top_right->isChecked())
		return (Qt::AlignTop|Qt::AlignRight);
	else if(ui->left->isChecked())
		return (Qt::AlignVCenter|Qt::AlignLeft);
	else if (ui->center->isChecked())
		return Qt::AlignCenter;
	else if(ui->right->isChecked())
		return (Qt::AlignVCenter|Qt::AlignRight);
	else if(ui->bottom_left->isChecked())
		return (Qt::AlignBottom|Qt::AlignLeft);
	else if(ui->bottom->isChecked())
		return (Qt::AlignBottom|Qt::AlignHCenter);
	else if(ui->bottom_right->isChecked())
		return (Qt::AlignBottom|Qt::AlignRight);
	else
		return (Qt::AlignTop|Qt::AlignLeft);
}

bool AlignmentTextDialog::event(QEvent *event)
{
		//Little hack when this dialog is called from a QAbstractItemModel, to set focus to a radio button
		//if we not do that, when the user click on the title bar (for move the dialog) or try to resize the dialog,
		//the dialog lose focus and close.
	if(event->type() == QEvent::Show && m_first_show)
	{
		QTimer::singleShot(50, [this](){ui->top_left->setFocus();});
		m_first_show = false;
	}
	
	return QDialog::event(event);
}
