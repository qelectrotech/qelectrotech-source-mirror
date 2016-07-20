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
#ifndef AUTONUMBERINGMANAGEMENTW_H
#define AUTONUMBERINGMANAGEMENTW_H

#include <QWidget>

class NumPartEditorW;
class QAbstractButton;
class QETProject;

namespace Ui {
	class AutoNumberingManagementW;
}

class AutoNumberingManagementW : public QWidget
{
	Q_OBJECT
	
	//METHODS
	public:
		explicit AutoNumberingManagementW(QETProject *project, QWidget *parent = 0);
		~AutoNumberingManagementW();
		Ui::AutoNumberingManagementW *ui;
		void setProjectContext ();
		void contextToFormula ();
		QString elementFormula();

	//SIGNALS
	signals:
		void applyPressed();

	//SLOTS
	private slots:
		void on_m_from_folios_cb_currentIndexChanged(int);
		void on_m_to_folios_cb_currentIndexChanged(int);
		void on_m_status_cb_currentIndexChanged(int);
		void on_m_apply_folios_rb_clicked();
		void on_m_apply_project_rb_clicked();
		void on_buttonBox_clicked(QAbstractButton *);
		void applyEnable (bool = true);

	//ATTRIBUTES
	private:
		QETProject *project_;
};

#endif // AUTONUMBERINGMANAGEMENTW_H
