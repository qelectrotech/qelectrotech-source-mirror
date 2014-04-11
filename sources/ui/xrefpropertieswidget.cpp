/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "xrefpropertieswidget.h"
#include "ui_xrefpropertieswidget.h"
#include "qdebug.h"

/**
 * @brief XRefPropertiesWidget::XRefPropertiesWidget
 * Default constructor
 * @param properties: properties to use
 * @param parent: parent widget
 */
XRefPropertiesWidget::XRefPropertiesWidget(XRefProperties properties, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::XRefPropertiesWidget),
	m_properties(properties)
{
	ui->setupUi(this);
	updateDisplay();
}

/**
 * @brief XRefPropertiesWidget::~XRefPropertiesWidget
 * Default destructor
 */
XRefPropertiesWidget::~XRefPropertiesWidget()
{
	delete ui;
}

/**
 * @brief XRefPropertiesWidget::setProperties
 * set new properties for this widget
 * @param properties
 */
void XRefPropertiesWidget::setProperties(const XRefProperties &properties) {
	m_properties = properties;
	updateDisplay();
}

/**
 * @brief XRefPropertiesWidget::properties
 * @return the propertie edited by this widget
 */
XRefProperties XRefPropertiesWidget::properties() {
	m_properties.setShowPowerContac(ui->cb_show_power->isChecked());
	return m_properties;
}

/**
 * @brief XRefPropertiesWidget::setReadOnly
 * Set all of this widget disable if true
 * @param ro
 */
void XRefPropertiesWidget::setReadOnly(bool ro) {
	ui->cb_show_power->setDisabled(ro);
}

/**
 * @brief XRefPropertiesWidget::updateDisplay
 * Update display with the content of the properties
 */
void XRefPropertiesWidget::updateDisplay() {
	ui->cb_show_power->setChecked(m_properties.showPowerContact());
}
