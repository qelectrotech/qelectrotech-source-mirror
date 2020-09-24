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
#include "bomexportdialog.h"
#include "ui_bomexportdialog.h"
#include "elementquerywidget.h"
#include "qetproject.h"
#include "qetapp.h"

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
		//By default format as bom is clicked
	on_m_format_as_bom_clicked(true);
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
			//save in csv file
		QString file_name = tr("nomenclature_") + QString(m_project ->title() + ".csv");
		QString file_path = QFileDialog::getSaveFileName(this, tr("Enregister sous... "), file_name, tr("Fichiers csv (*.csv)"));
		QFile file(file_path);
		if (!file_path.isEmpty())
		{
			if (QFile::exists(file_path ))
			{
				// if file already exist -> delete it
				if (!QFile::remove(file_path) )
				{
					QMessageBox::critical(this, tr("Erreur"),
										  tr("Impossible de remplacer le fichier!\n\n")+
										  "Destination : "+file_path+"\n");
				}
			}
			if (file.open(QIODevice::WriteOnly | QIODevice::Text))
			{
				QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)	// ### Qt 6: remove
				stream << getBom() << endl;
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 5.15 or later")
#endif
				stream << getBom() << &Qt::endl(stream);
#endif
			}
		}
	}
	return r;
}

QString BOMExportDialog::getBom()
{
	m_project->dataBase()->updateDB();
	auto query_ = m_project->dataBase()->newQuery(m_query_widget->queryStr());
	QString return_string;

	if (!query_.exec()) {
		qDebug() << "BOMExportDialog::getBom : query errir : " << query_.lastError();
	}
	else
	{
			//HEADERS
		if (ui->m_include_headers)
		{
			auto record_ = query_.record();
			QStringList header_name;
			for (auto i=0 ; i<record_.count() ; ++i)
			{
				auto field_name = record_.fieldName(i);

				qDebug() << "field name = " << field_name;
				if (field_name == "position") {
					header_name << tr("Position");
				} else if (field_name == "diagram_position") {
					header_name << tr("Position du folio");
				} else if (field_name == "designation_qty") {
					header_name << tr("Quantité numéro d'article", "Special field with name : designation quantity");
				} else {
					header_name << QETApp::elementTranslatedInfoKey(field_name);
					if (header_name.isEmpty()) {
						header_name << QETApp::diagramTranslatedInfoKey(field_name);
					}
					if (header_name.isEmpty()) {
						header_name << field_name;
					}
				}

			}
			return_string = header_name.join(";") + "\n";
		}

			//ROWS
		while (query_.next())
		{
			auto i=0;
			QStringList values;
			while (query_.value(i).isValid())
			{
				auto date = query_.value(i).toDate();
				if (!date.isNull()) {
					values << QLocale::system().toString(query_.value(i).toDate(), QLocale::ShortFormat);
				} else {
					values << query_.value(i).toString();
				}
				++i;
			}

			return_string += values.join(";") + "\n";
			values.clear();
		}
	}

	qDebug() << return_string;
	return return_string;
}

/**
	@brief BOMExportDialog::on_m_format_as_bom_clicked
	@param checked
*/
void BOMExportDialog::on_m_format_as_bom_clicked(bool checked) {
	m_query_widget->setGroupBy("designation", checked);
	m_query_widget->setCount("COUNT(*) AS designation_qty", checked);
}
