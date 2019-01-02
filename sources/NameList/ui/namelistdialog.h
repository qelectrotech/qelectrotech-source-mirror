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
#ifndef NAMELISTDIALOG_H
#define NAMELISTDIALOG_H

#include <QDialog>

class NameListWidget;

namespace Ui {
	class NameListDialog;
}

/**
 * @brief The NameListDialog class
 * Provide a dialog for let user define localized string;
 */
class NameListDialog : public QDialog
{
	Q_OBJECT
	
	public:
		explicit NameListDialog(QWidget *parent = nullptr);
		~NameListDialog();
	
		void setInformationText(const QString &text);
		NameListWidget *namelistWidget() const;
		void setHelpText(const QString &text);
		void showHelpDialog();
	
	private:
		Ui::NameListDialog *ui;
		NameListWidget *m_namelist_widget = nullptr;
		QString m_help_text;
};

#endif // NAMELISTDIALOG_H
