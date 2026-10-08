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
#include "cableexportdialog.h"

#include "cablelistmodel.h"
#include "cablelistrows.h"
#include "cablequerywidget.h"

#include "../bomexport.h"
#include "../qet.h"
#include "../qetapp.h"
#include "../qetproject.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

/**
	@brief CableExportDialog::CableExportDialog
	@param project the project whose cables are exported
	@param parent
*/
CableExportDialog::CableExportDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	m_project(project)
{
	setWindowTitle(tr("Exporter la liste des câbles au format CSV"));
	QET::trackDialogGeometry(this);

	auto *main_layout = new QVBoxLayout(this);

		//The same column picker the cable list is built with, so the
		//export asks for its columns the same way the list does
	m_query_widget = new CableQueryWidget(this);
	main_layout->addWidget(m_query_widget);

	auto *page_group = new QGroupBox(tr("Mise en page"), this);
	auto *page_layout = new QVBoxLayout(page_group);
	m_include_headers = new QCheckBox(tr("inclure les en-têtes"), page_group);
	m_include_headers->setChecked(true);
	page_layout->addWidget(m_include_headers);
	page_layout->addStretch();
	main_layout->addWidget(page_group);

	auto *preview_layout = new QHBoxLayout;
	auto *preview_pb = new QPushButton(tr("Aperçu"), this);
	preview_layout->addWidget(preview_pb);
	preview_layout->addStretch();
	main_layout->addLayout(preview_layout);

	m_preview_table = new QTableView(this);
	m_preview_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	main_layout->addWidget(m_preview_table);

	auto *buttons = new QDialogButtonBox(
			QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	main_layout->addWidget(buttons);
	connect(buttons, &QDialogButtonBox::accepted,
			this, &CableExportDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected,
			this, &CableExportDialog::reject);
	connect(preview_pb, &QPushButton::clicked,
			this, &CableExportDialog::preview);

	m_model = new CableListModel(m_project, this);
	m_preview_table->setModel(m_model);
	preview();

	setMinimumSize(QSize(760, 560));
}

/**
	@brief CableExportDialog::exec
	Ask for the file once the user accepted, and write the chosen
	columns to it -- the same flow as the material list export
	@return
*/
int CableExportDialog::exec()
{
	const auto r = QDialog::exec();
	if (r == QDialog::Accepted)
	{
		QString dir = m_project->currentDir();
		if (dir.isEmpty()) dir = QETApp::documentDir();
		const QString file_name = dir + QLatin1Char('/')
				+ tr("liste_cables_") + m_project->title()
				+ QStringLiteral(".csv");
		const QString file_path = QFileDialog::getSaveFileName(
				this, tr("Enregistrer sous... "), file_name,
				tr("Fichiers csv (*.csv)"));
		if (!file_path.isEmpty())
		{
			QString error;
			const auto csv = CableList::toCsv(
					m_project, selectedColumns(),
					m_include_headers->isChecked());
			if (!BomExport::writeCsv(file_path, csv, &error)) {
				QMessageBox::critical(
						this, tr("Erreur"),
						tr("Impossible d'enregistrer la liste des câbles dans %1.\n%2")
								.arg(file_path, error));
			}
		}
	}
	return r;
}

/**
	@brief CableExportDialog::selectedColumns
	@return the picked columns, and the default ones when the user
	picked nothing -- an export of everything is what a dialog
	confirmed without choosing asks for, the same fallback the drawn
	table uses
*/
QStringList CableExportDialog::selectedColumns() const
{
	auto keys = m_query_widget->selectedKeys();
	if (keys.isEmpty()) {
		keys = CableList::defaultKeys();
	}
	return keys;
}

/**
	@brief CableExportDialog::preview
	Show what will be written: the chosen columns, worked out from the
	cables of the project as they are right now
*/
void CableExportDialog::preview()
{
	m_model->setFields(selectedColumns());
	m_preview_table->resizeColumnsToContents();
}
