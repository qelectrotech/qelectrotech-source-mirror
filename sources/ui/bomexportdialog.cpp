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
#include "bomexportdialog.h"

#include "../dataBase/ui/elementquerywidget.h"
#include "../bomexport.h"
#include "../qetapp.h"
#include "../qetinformation.h"
#include "../qetproject.h"
#include "ui_bomexportdialog.h"

#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>

/**
	@brief BOMExportDialog::BOMExportDialog
	@param project
	@param parent
*/
BOMExportDialog::BOMExportDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::BOMExportDialog),
	m_project(project)
{
	ui->setupUi(this);

	m_query_widget = new ElementQueryWidget(this);
	ui->m_main_layout->insertWidget(0, m_query_widget);
	m_query_widget->setQuery(BomExport::defaultQuery());
	on_m_format_as_bom_clicked(false);
}

/**
	@brief BOMExportDialog::~BOMExportDialog
*/
BOMExportDialog::~BOMExportDialog()
{
	delete ui;
}

/**
	@brief BOMExportDialog::exec
	@return
*/
int BOMExportDialog::exec()
{
	auto r = QDialog::exec();
	if (r == QDialog::Accepted)
	{
			//save in csv file in same directory as project by default
		QString dir = m_project->currentDir();
		if (dir.isEmpty()) dir = QETApp::documentDir();
		QString file_name = dir % "/" % tr("nomenclature_") % QString(m_project ->title() % ".csv");
		QString file_path = QFileDialog::getSaveFileName(this, tr("Enregister sous... "), file_name, tr("Fichiers csv (*.csv)"));
		if (!file_path.isEmpty())
		{
			QString error;
			const auto csv = getBom(&error);
			if (!error.isEmpty() || !BomExport::writeCsv(file_path, csv, &error)) {
				QMessageBox::critical(
						this, tr("Erreur"),
						tr("Impossible d'enregistrer la nomenclature dans %1.\n%2")
								.arg(file_path, error));
			}
		}
	}
	return r;
}

QByteArray BOMExportDialog::getBom(QString *error)
{
	if (error) {
		error->clear();
	}
	m_project->dataBase()->updateDB();
	auto query_ = m_project->dataBase()->newQuery(m_query_widget->queryStr());

	if (!query_.exec()) {
		qDebug() << "BOMExportDialog::getBom : query errir : " << query_.lastError();
		if (error) {
			*error = query_.lastError().text();
		}
		return {};
	}

	QStringList header_names;
	if (ui->m_include_headers->isChecked())
	{
		const auto record_ = query_.record();
		for (int i = 0; i < record_.count(); ++i)
		{
			const auto field_name = record_.fieldName(i);
			if (field_name == QLatin1String("position")) {
				header_names << tr("Position");
			} else if (field_name == QLatin1String("diagram_position")) {
				header_names << tr("Position du folio");
			} else if (field_name == QLatin1String("designation_qty")) {
				header_names << tr("Quantité numéro d'article", "Special field with name : designation quantity");
			} else {
				const auto translated = QETInformation::translatedInfoKey(field_name);
				header_names << (translated.isEmpty() ? field_name : translated);
			}
		}
	}
	return BomExport::toCsv(
			query_, header_names, ui->m_include_headers->isChecked());
}

/**
	@brief BOMExportDialog::on_m_format_as_bom_clicked
	@param checked
*/
void BOMExportDialog::on_m_format_as_bom_clicked(bool checked) {
	m_query_widget->setGroupBy("designation", checked);
	m_query_widget->setCount("COUNT(*) AS designation_qty", checked);
}
