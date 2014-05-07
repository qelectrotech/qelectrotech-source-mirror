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
	connect(ui->m_display_has_cross_rb, SIGNAL(toggled(bool)), ui->m_cross_properties_gb, SLOT(setEnabled(bool)));
	updateDisplay();
}

/**
 * @brief XRefPropertiesWidget::~XRefPropertiesWidget
 * Default destructor
 */
XRefPropertiesWidget::~XRefPropertiesWidget()
{
	disconnect(ui->m_display_has_cross_rb, SIGNAL(toggled(bool)), ui->m_cross_properties_gb, SLOT(setEnabled(bool)));
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
	if		(ui->m_display_has_cross_rb->isChecked())	 m_properties.setDisplayHas(XRefProperties::Cross);
	else if (ui->m_display_has_contacts_rb->isChecked()) m_properties.setDisplayHas(XRefProperties::Contacts);
	m_properties.setShowPowerContac(ui->m_show_power_cb->isChecked());
	m_properties.setPrefix("power", ui->m_power_prefix_le->text());
	m_properties.setPrefix("delay", ui->m_delay_prefix_le->text());

	return m_properties;
}

/**
 * @brief XRefPropertiesWidget::setReadOnly
 * Set all of this widget disable if true
 * @param ro
 */
void XRefPropertiesWidget::setReadOnly(bool ro) {
	ui->m_display_has_cross_rb->setDisabled(ro);
	ui->m_display_has_contacts_rb->setDisabled(ro);

	if (m_properties.displayHas() != XRefProperties::Cross)
		ui->m_cross_properties_gb->setDisabled(true);
	else
		ui->m_cross_properties_gb->setDisabled(ro);
}

/**
 * @brief XRefPropertiesWidget::updateDisplay
 * Update display with the content of the properties
 */
void XRefPropertiesWidget::updateDisplay() {
	XRefProperties::DisplayHas dh = m_properties.displayHas();
	if		(dh == XRefProperties::Cross)	 {
		ui->m_display_has_cross_rb->setChecked(true);
	}
	else if (dh == XRefProperties::Contacts) {
		ui->m_display_has_contacts_rb->setChecked(true);
	}

	ui->m_show_power_cb->setChecked(m_properties.showPowerContact());
	ui->m_power_prefix_le->setText(m_properties.prefix("power"));
	ui->m_delay_prefix_le->setText(m_properties.prefix("delay"));
	ui->m_cross_properties_gb->setDisabled(!ui->m_display_has_cross_rb->isChecked());
}
