/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "borderpropertieswidget.h"
#include "ui_borderpropertieswidget.h"
#include "diagram.h"

/**
 * @brief BorderPropertiesWidget::BorderPropertiesWidget
 * default constructor
 * @param bp properties
 * @param parent paretn widget
 */
BorderPropertiesWidget::BorderPropertiesWidget(const BorderProperties &bp, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::BorderPropertiesWidget)
{
	ui->setupUi(this);
	setProperties(bp);
}

/**
 * @brief BorderPropertiesWidget::~BorderPropertiesWidget
 * default destructor
 */
BorderPropertiesWidget::~BorderPropertiesWidget()
{
	delete ui;
}

/**
 * @brief BorderPropertiesWidget::setProperties
 * Set the current properties to edit
 * @param bp properties to edit
 */
void BorderPropertiesWidget::setProperties(const BorderProperties &bp) {
	m_properties = bp;
	ui -> m_colums_count_sp    ->setValue   (m_properties.columns_count);
	ui -> m_columns_width_sp   ->setValue   (m_properties.columns_width);
	ui -> m_display_columns_cb ->setChecked (m_properties.display_columns);
	ui -> m_rows_count_sp      ->setValue   (m_properties.rows_count);
	ui -> m_rows_height_sp     ->setValue   (m_properties.rows_height);
	ui -> m_display_rows_cb    ->setChecked (m_properties.display_rows);
}

/**
 * @brief BorderPropertiesWidget::properties
 * @return the edited border properties
 */
const BorderProperties &BorderPropertiesWidget::properties () {
	m_properties.columns_count   = ui -> m_colums_count_sp    -> value();
	m_properties.columns_width   = ui -> m_columns_width_sp   -> value();
	m_properties.display_columns = ui -> m_display_columns_cb -> isChecked();
	m_properties.rows_count      = ui -> m_rows_count_sp      -> value();
	m_properties.rows_height     = ui -> m_rows_height_sp     -> value();
	m_properties.display_rows    = ui -> m_display_rows_cb    -> isChecked();
	return m_properties;
}

/**
 * @brief BorderPropertiesWidget::setReadOnly
 * Enable or disable this widget
 * @param ro true-disable / false-enable
 */
void BorderPropertiesWidget::setReadOnly(const bool &ro) {
	ui->border_gb->setDisabled(ro);
}
