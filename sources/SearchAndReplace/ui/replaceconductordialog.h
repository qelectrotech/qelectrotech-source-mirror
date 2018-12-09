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
#ifndef REPLACECONDUCTORDIALOG_H
#define REPLACECONDUCTORDIALOG_H

#include "conductorproperties.h"

#include <QDialog>

namespace Ui {
	class ReplaceConductorDialog;
}

/**
 * @brief The ReplaceConductorDialog class
 * A Qdialog to edit a conductor properties, use for the search and replace feature of QElectrotech.
 */
class ReplaceConductorDialog : public QDialog
{
	Q_OBJECT
	
	public:
		explicit ReplaceConductorDialog(const ConductorProperties &properties, QWidget *parent = 0);
		~ReplaceConductorDialog();
		void updatePreview(bool b=true);
		void setProperties(const ConductorProperties &properties);
		ConductorProperties properties() const;
		
	private:
		void initWidget();
		void setColorButton(const QColor &color);
		void setColorButton2(const QColor &color);
	
	private slots:
		void on_m_erase_formula_cb_clicked();
		void on_m_erase_text_cb_clicked();
		void on_m_erase_function_cb_clicked();
		void on_m_erase_tension_protocol_cb_clicked();
		void on_m_earth_cb_toggled(bool checked);
		void on_m_neutral_cb_toggled(bool checked);
		void on_m_update_preview_pb_clicked();
		void on_m_color_pb_clicked();
		void on_m_color_2_pb_clicked();
		
		private:
		Ui::ReplaceConductorDialog *ui;
		ConductorProperties m_properties;
};

#endif // REPLACECONDUCTORDIALOG_H
