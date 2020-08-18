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
#ifndef BOMEXPORTDIALOG_H
#define BOMEXPORTDIALOG_H

#include <QDialog>

class QETProject;
class ElementQueryWidget;

namespace Ui {
class BOMExportDialog;
}

/**
	@brief The BOMExportDialog class
*/
class BOMExportDialog : public QDialog
{
		Q_OBJECT

	public:
		explicit BOMExportDialog(QETProject *project, QWidget *parent = nullptr);
		~BOMExportDialog() override;

		virtual int exec() override;
		QString getBom();

	private slots:
		void on_m_format_as_bom_clicked(bool checked);

		private:
		Ui::BOMExportDialog *ui;
		ElementQueryWidget *m_query_widget = nullptr;
		QETProject *m_project = nullptr;
};

#endif // BOMEXPORTDIALOG_H
