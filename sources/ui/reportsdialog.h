/*
	Copyright 2006-2026 The QElectroTech Team
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
#ifndef REPORTSDIALOG_H
#define REPORTSDIALOG_H

#include <reports/projectreports.h>

#include <QDialog>
#include <QPointer>

class QComboBox;
class QLabel;
class QETProject;
class QPushButton;
class QTableWidget;

/**
	@brief The ReportsDialog class
	Project > Reports...: shows one of the project's lists
	(ProjectReports), for the whole project or one place, and writes it
	to CSV or PDF.
*/
class ReportsDialog : public QDialog
{
	Q_OBJECT
	public:
		explicit ReportsDialog(QETProject *project, QWidget *parent = nullptr);

	public slots:
		void refresh();
		bool exportCsv(const QString &path = QString());
		bool exportPdf(const QString &path = QString());

	private:
		ProjectReports::Report currentReport() const;
		ProjectReports::Place currentPlace() const;
		QString chooseFile(const QString &filter, const QString &suffix);
		void buildFolios();

		QPointer<QETProject> m_project;
		QComboBox *m_report;
		QComboBox *m_place;
		QLabel *m_summary;
		QTableWidget *m_table;
		QPushButton *m_csv;
		QPushButton *m_pdf;
		QList<ProjectReports::Report> m_reports;
		QList<ProjectReports::Place> m_places;
		ProjectReports::Table m_shown;
};

#endif // REPORTSDIALOG_H
