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
#ifndef FOLIOAUTONUMBERING_H
#define FOLIOAUTONUMBERING_H

#include <QWidget>
#include "numerotationcontext.h"

class NumPartEditorW;
class QAbstractButton;
class QETProject;

namespace Ui {
		class FolioAutonumberingW;
}

class FolioAutonumberingW : public QWidget
{
	Q_OBJECT
	
	//METHODS
	public:
		explicit FolioAutonumberingW(QETProject *project, QWidget *parent = 0);
		~FolioAutonumberingW();

		void setContext (QList <QString> autonums);
		NumerotationContext toNumContext() const;
		QString autoNumSelected();
		int newFoliosNumber();
		bool newFolios;
		int fromFolio();
		int toFolio();

	// SIGNALS
	signals:
		void applyPressed();
		void m_autonumber_tabs_rb_clicked();

	//SLOTS
	private slots:
		void on_m_create_new_tabs_rb_clicked();
		void on_m_autonumber_tabs_rb_clicked();
		void on_m_new_tabs_sb_valueChanged(int);
		void on_buttonBox_clicked(QAbstractButton *);
		void on_m_from_cb_currentIndexChanged(int);
		void applyEnable (bool = true);

	//ATTRIBUTES
	private:
		QETProject *project_;
		Ui::FolioAutonumberingW *ui;
		QList <NumPartEditorW *> num_part_list_;
		NumerotationContext m_context;
		void updateFolioList();
};

#endif // FOLIOAUTONUMBERING_H
