/*
	Copyright 2006-2020 The QElectroTech Team
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
#ifndef ADDTABLEDIALOG_H
#define ADDTABLEDIALOG_H

#include <QDialog>

#include "qetapp.h"

namespace Ui {
class AddTableDialog;
}

/**
	@brief The AddTableDialog class
	Provide a dialog used to edit the properties of table befor adding to a diagram.
	The main difference betwen this dialog and the widget used to edit the properties of table
	is that the dialog have two extra check box.
	One for adjust the size of the table to diagram
	Second for add new tables on new folios if the table can't fit into diagram
*/
class AddTableDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit AddTableDialog(QWidget *content_widget, QWidget *parent = nullptr);
		~AddTableDialog();

		void setQueryWidget(QWidget *widget);
		bool adjustTableToFolio() const;
		bool addNewTableToNewDiagram() const;

		QString tableName() const;

		QMargins headerMargins() const;
		Qt::Alignment headerAlignment() const;
		QFont headerFont() const;

		QMargins tableMargins() const;
		Qt::Alignment tableAlignment() const;
		QFont tableFont() const;
		QWidget *contentWidget() const;

	private slots:
		void on_m_header_font_pb_clicked();
		void on_m_table_font_pb_clicked();
		void on_m_edit_header_margins_pb_clicked();
		void on_m_table_margins_pb_clicked();
		void saveConfig();
		void loadConfig();
		void fillSavedQuery();

	private:
		Ui::AddTableDialog *ui;

		QWidget *m_content_widget = nullptr;

		QMargins m_header_margins = QMargins(5,5,10,5),
				 m_table_margins  = QMargins(5,5,10,5);

		QFont m_header_font = QETApp::diagramTextsFont();
		QFont m_table_font  = QETApp::diagramTextsFont();

		QString m_identifier;

};

#endif // ADDTABLEDIALOG_H
