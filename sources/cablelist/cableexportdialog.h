/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef CABLEEXPORTDIALOG_H
#define CABLEEXPORTDIALOG_H

#include <QCoreApplication>
#include <QDialog>

class QCheckBox;
class QTableView;

class CableListModel;
class CableQueryWidget;
class QETProject;

/**
	@brief The CableExportDialog asks which columns of the cable list
	are exported and writes them to a CSV file, built the same way the
	BOMExportDialog does it for the material list: the column picker
	the cable list is built with on top, the page settings under it, a
	preview of what will be written, and the file asked for when the
	dialog is accepted.
*/
class CableExportDialog : public QDialog
{
	Q_DECLARE_TR_FUNCTIONS(CableExportDialog)

	public:
		explicit CableExportDialog(QETProject *project, QWidget *parent = nullptr);
		int exec() override;

	private:
		QStringList selectedColumns() const;
		void preview();

		QETProject *m_project;
		CableQueryWidget *m_query_widget;
		CableListModel *m_model;
		QCheckBox *m_include_headers;
		QTableView *m_preview_table;
};

#endif // CABLEEXPORTDIALOG_H
