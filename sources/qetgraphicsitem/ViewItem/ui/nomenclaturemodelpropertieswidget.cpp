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
    if (model) {
        setModel(model);
    }
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
}

void NomenclatureModelPropertiesWidget::on_m_edit_query_pb_clicked()
{}

void NomenclatureModelPropertiesWidget::on_m_refresh_pb_clicked() {
	if (m_model) {
		m_model->query("SELECT plant, location, label, comment, description FROM element_info ORDER BY plant, location, label, comment, description");
	}
}
