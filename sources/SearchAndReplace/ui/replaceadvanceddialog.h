/*
	Copyright 2006-2018 The QElectroTech Team
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
#ifndef REPLACEADVANCEDDIALOG_H
#define REPLACEADVANCEDDIALOG_H

#include "searchandreplaceworker.h"

#include <QDialog>

namespace Ui {
	class replaceAdvancedDialog;
}

class replaceAdvancedDialog : public QDialog
{
	Q_OBJECT
	
	public:
		explicit replaceAdvancedDialog(advancedReplaceStruct advanced, QWidget *parent = nullptr);
		~replaceAdvancedDialog();
	
		void setAdvancedStruct(advancedReplaceStruct advanced);
		advancedReplaceStruct advancedStruct() const;
	
	private:
		void fillWhatComboBox(int index);
	
	private slots:
		void on_m_who_cb_currentIndexChanged(int index);
		
	private:
		Ui::replaceAdvancedDialog *ui;
};

#endif // REPLACEADVANCEDDIALOG_H
