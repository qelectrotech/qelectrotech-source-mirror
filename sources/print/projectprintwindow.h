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
#ifndef PROJECTPRINTWINDOW_H
#define PROJECTPRINTWINDOW_H

#include <QMainWindow>

#include "exportproperties.h"
#include <QPrinter>

namespace Ui {
class ProjectPrintWindow;
}

class QETProject;
class QPrintPreviewWidget;
class QPrinter;
class ExportProperties;
class Diagram;
class QCheckBox;

/**
 * @brief The ProjectPrintWindow class
 * Windows used to configur and view diagram befor print
 */
class ProjectPrintWindow : public QMainWindow
{
    Q_OBJECT

	public:
		static void launchDialog(QETProject *project, QPrinter::OutputFormat format = QPrinter::NativeFormat, QWidget *parent = nullptr);
		static QString docName(QETProject *project);

		explicit ProjectPrintWindow(QETProject *project, QPrinter *printer, QWidget *parent = nullptr);
		~ProjectPrintWindow();

	private slots:
		void on_m_draw_border_cb_clicked();
		void on_m_draw_titleblock_cb_clicked();
		void on_m_keep_conductor_color_cb_clicked();
		void on_m_draw_terminal_cb_clicked();
		void on_m_fit_in_page_cb_clicked();
		void on_m_use_full_page_cb_clicked();
		void on_m_zoom_out_action_triggered();
		void on_m_zoom_in_action_triggered();
		void on_m_adjust_width_action_triggered();
		void on_m_adjust_page_action_triggered();
		void on_m_landscape_action_triggered();
		void on_m_portrait_action_triggered();
		void on_m_first_page_action_triggered();
		void on_m_previous_page_action_triggered();
		void on_m_next_page_action_triggered();
		void on_m_last_page_action_triggered();
		void on_m_display_single_page_action_triggered();
		void on_m_display_two_page_action_triggered();
		void on_m_display_all_page_action_triggered();
		void on_m_page_setup_triggered();
		void on_m_check_all_pb_clicked();
		void on_m_uncheck_all_clicked();
		void print();
		void on_m_date_cb_userDateChanged(const QDate &date);
		void on_m_date_from_cb_currentIndexChanged(int index);
		void on_m_apply_date_pb_clicked();

	private:
		void requestPaint();
		void printDiagram(Diagram *diagram, bool fit_page, QPainter *painter, QPrinter *printer);
		QRect diagramRect(Diagram *diagram, const ExportProperties &option) const;
		int horizontalPagesCount(Diagram *diagram, const ExportProperties &option, bool full_page) const;
		int verticalPagesCount(Diagram *diagram, const ExportProperties &option, bool full_page) const;
		ExportProperties exportProperties() const;
		void setUpDiagramList();
		QString settingsSectionName(const QPrinter *printer);
		void loadPageSetupForCurrentPrinter();
		void savePageSetupForCurrentPrinter();
		void saveReloadDiagramParameters(Diagram *diagram, const ExportProperties &options, bool save);
		QList<Diagram *> selectedDiagram() const;
		void exportToPDF();


	private:
		Ui::ProjectPrintWindow *ui;
		QETProject *m_project = nullptr;
		QPrinter *m_printer = nullptr;
		QPrintPreviewWidget *m_preview=nullptr;
		QColor m_backup_diagram_background_color;
		QHash<Diagram *, QCheckBox *> m_diagram_list_hash;
};

#endif // PROJECTPRINTWINDOW_H
