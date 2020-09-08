/*
	Copyright 2006-2020 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "projectdbmodelpropertieswidget.h"
#include "ui_projectdbmodelpropertieswidget.h"
#include "projectdbmodel.h"
#include "qetproject.h"
#include "elementquerywidget.h"
#include "summaryquerywidget.h"

#include <QDialogButtonBox>

/**
	@brief projectDBModelPropertiesWidget::projectDBModelPropertiesWidget
	@param model
	@param parent
*/
ProjectDBModelPropertiesWidget::ProjectDBModelPropertiesWidget(
		ProjectDBModel *model,
		QWidget *parent) :
	PropertiesEditorWidget(parent),
	ui(new Ui::ProjectDBModelPropertiesWidget)
{
	ui->setupUi(this);
	setModel(model);
}

/**
	@brief projectDBModelPropertiesWidget::~projectDBModelPropertiesWidget
*/
ProjectDBModelPropertiesWidget::~ProjectDBModelPropertiesWidget()
{
	delete ui;
}

/**
	@brief projectDBModelPropertiesWidget::setModel
	@param model
*/
void ProjectDBModelPropertiesWidget::setModel(ProjectDBModel *model)
{
	m_model = model;
	ui->m_edit_query_pb->setEnabled(m_model);
	ui->m_refresh_pb->setEnabled(m_model);
}

/**
	@brief projectDBModelPropertiesWidget::on_m_edit_query_pb_clicked
*/
void ProjectDBModelPropertiesWidget::on_m_edit_query_pb_clicked()
{
	QDialog d(this);
	auto l = new QVBoxLayout;
	d.setLayout(l);

	ElementQueryWidget *nom_w = nullptr;
	SummaryQueryWidget *sum_w = nullptr;
	if (m_model->identifier() == "nomenclature")
	{
		nom_w = new ElementQueryWidget(&d);
		nom_w->setQuery(m_model->queryString());
		l->addWidget(nom_w);
	}
	else if (m_model->identifier() == "summary")
	{
		sum_w = new SummaryQueryWidget(&d);
		sum_w->setQuery(m_model->queryString());
		l->addWidget(sum_w);
	}

	auto button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	l->addWidget(button_box);
	connect(button_box, &QDialogButtonBox::accepted, &d, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, &d, &QDialog::reject);

	if (d.exec())
	{
		if (nom_w) {
			m_model->setQuery(nom_w->queryStr());
		} else if (sum_w) {
			m_model->setQuery(sum_w->queryStr());
		}
	}
}

void ProjectDBModelPropertiesWidget::on_m_refresh_pb_clicked()
{
	if (m_model && m_model->project()) {
		m_model->project()->dataBase()->updateDB();
	}
}
