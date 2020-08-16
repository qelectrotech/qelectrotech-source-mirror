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
#ifndef MARGINSEDITDIALOG_H
#define MARGINSEDITDIALOG_H

#include <QDialog>
#include <QMargins>

namespace Ui {
class MarginsEditDialog;
}

/**
	@brief The MarginsEditDialog class
	A simple dialog to edit QMargins
*/
class MarginsEditDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit MarginsEditDialog(QMargins margins = QMargins(),
					   QWidget *parent = nullptr);
		~MarginsEditDialog();

		QMargins margins() const;

		static QMargins getMargins(QMargins margins = QMargins(),
					   bool *accepted = nullptr,
					   QWidget *parent = nullptr);

	private:
		Ui::MarginsEditDialog *ui;
};

#endif // MARGINSEDITDIALOG_H
