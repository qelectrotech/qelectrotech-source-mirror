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
#include "nomenclaturemodelpropertieswidget.h"
#include "ui_nomenclaturemodelpropertieswidget.h"
#include "nomenclaturemodel.h"
#include "qetproject.h"
#include "elementquerywidget.h"

#include <QDialogButtonBox>

/**
 * @brief NomenclatureModelPropertiesWidget::NomenclatureModelPropertiesWidget
 * @param model
 * @param parent
 */
NomenclatureModelPropertiesWidget::NomenclatureModelPropertiesWidget(NomenclatureModel *model, QWidget *parent) :
	PropertiesEditorWidget(parent),
    ui(new Ui::NomenclatureModelPropertiesWidget)
{
    ui->setupUi(this);
	setModel(model);
}

/**
 * @brief NomenclatureModelPropertiesWidget::~NomenclatureModelPropertiesWidget
 */
NomenclatureModelPropertiesWidget::~NomenclatureModelPropertiesWidget() {
    delete ui;
}

/**
 * @brief NomenclatureModelPropertiesWidget::setModel
 * @param model
 */
void NomenclatureModelPropertiesWidget::setModel(NomenclatureModel *model) {
    m_model = model;
	ui->m_edit_query_pb->setEnabled(m_model);
	ui->m_refresh_pb->setEnabled(m_model);
}

/**
 * @brief NomenclatureModelPropertiesWidget::on_m_edit_query_pb_clicked
 */
void NomenclatureModelPropertiesWidget::on_m_edit_query_pb_clicked()
{
	QDialog d(this);
	auto l = new QVBoxLayout(this);
	d.setLayout(l);

	auto query_widget = new ElementQueryWidget(&d);
	l->addWidget(query_widget);

	auto button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	l->addWidget(button_box);
	connect(button_box, &QDialogButtonBox::accepted, &d, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, &d, &QDialog::reject);

	if (d.exec())
	{
		m_model->query(query_widget->queryStr());
		auto headers = query_widget->header();
		for (auto i=0 ; i<headers.size() ; ++i) {
			m_model->setHeaderData(i, Qt::Horizontal, headers.at(i));
		}
	}
}

void NomenclatureModelPropertiesWidget::on_m_refresh_pb_clicked() {
	if (m_model && m_model->project()) {
		m_model->project()->dataBase()->updateDB();
	}
}
