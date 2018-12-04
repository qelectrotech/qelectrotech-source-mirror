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
#ifndef REPLACEFOLIOWIDGET_H
#define REPLACEFOLIOWIDGET_H

#include <QWidget>
#include <QDialog>

#include "diagramcontext.h"
#include "titleblockproperties.h"

class DiagramContextWidget;
class QDialogButtonBox;

namespace Ui {
	class ReplaceFolioWidget;
}
 
class ReplaceFolioWidget : public QWidget
{
	Q_OBJECT
	
	public:
		explicit ReplaceFolioWidget(QWidget *parent = nullptr);
		~ReplaceFolioWidget();
	
		TitleBlockProperties titleBlockProperties() const;
		void setTitleBlockProperties (const TitleBlockProperties &properties);
		
	private slots:
		void on_m_title_cb_clicked();
		void on_m_author_cb_clicked();
		void on_m_file_cb_clicked();
		void on_m_folio_cb_clicked();
        void on_m_plant_cb_clicked();
		void on_m_loc_cb_clicked();
		void on_m_indice_cb_clicked();
		
	private:
		Ui::ReplaceFolioWidget *ui;
		DiagramContextWidget *m_diagram_context_widget = nullptr;
		
};

class ReplaceFolioDialog : public QDialog
{
	Q_OBJECT
	
	public:
		ReplaceFolioDialog(QWidget *parent = nullptr);
		~ReplaceFolioDialog();
		
		TitleBlockProperties titleBlockProperties() const;
		void setTitleBlockProperties (const TitleBlockProperties &properties);
		
	private:
		ReplaceFolioWidget *m_widget = nullptr;
		QDialogButtonBox *m_button_box = nullptr;
};

#endif // REPLACEFOLIOWIDGET_H
