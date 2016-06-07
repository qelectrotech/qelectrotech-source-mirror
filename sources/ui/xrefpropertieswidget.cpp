/*
	Copyright 2006-2016 The QElectroTech Team
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
XRefPropertiesWidget::XRefPropertiesWidget(QHash <QString, XRefProperties> properties, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::XRefPropertiesWidget),
	m_properties(properties)
{
	ui->setupUi(this);
	buildUi();
	connect(ui->m_display_has_cross_rb, SIGNAL(toggled(bool)),            ui->m_cross_properties_gb, SLOT(setEnabled(bool)));
	connect(ui->m_type_cb,              SIGNAL(currentIndexChanged(int)), this,                      SLOT(typeChanged()));
	updateDisplay();
}

/**
 * @brief XRefPropertiesWidget::~XRefPropertiesWidget
 * Default destructor
 */
XRefPropertiesWidget::~XRefPropertiesWidget()
{
	disconnect(ui->m_display_has_cross_rb, SIGNAL(toggled(bool)),            ui->m_cross_properties_gb, SLOT(setEnabled(bool)));
	disconnect(ui->m_type_cb,              SIGNAL(currentIndexChanged(int)), this,                      SLOT(typeChanged()));
	delete ui;
}

/**
 * @brief XRefPropertiesWidget::setProperties
 * set new properties for this widget
 * @param properties
 */
void XRefPropertiesWidget::setProperties(const QHash <QString, XRefProperties> &properties) {
	m_properties = properties;
	updateDisplay();
	m_previous_type_index = ui->m_type_cb->currentIndex();
}

/**
 * @brief XRefPropertiesWidget::properties
 * @return the properties edited by this widget
 */
QHash <QString, XRefProperties> XRefPropertiesWidget::properties(){
	saveProperties(ui->m_type_cb->currentIndex());
	return m_properties;
}

/**
 * @brief XRefPropertiesWidget::setReadOnly
 * Set all of this widget disable if true
 * @param ro
 */
void XRefPropertiesWidget::setReadOnly(bool ro) {
	ui->m_type_cb->setDisabled(ro);
	ui->m_display_gb->setDisabled(ro);
	ui->m_cross_properties_gb->setDisabled(ro);

	if (!ro && ui->m_display_has_contacts_rb->isChecked()) {
		ui->m_cross_properties_gb->setDisabled(true);
	}
}

/**
 * @brief XRefPropertiesWidget::buildUi
 * Build some widget of this ui.
 */
void XRefPropertiesWidget::buildUi()
{
	ui -> m_type_cb -> addItem(tr("Bobine"),			   "coil");
	ui -> m_type_cb -> addItem(tr("Organe de protection"), "protection");
	ui -> m_type_cb -> addItem(tr("Commutateur / bouton"), "commutator");

	ui -> m_snap_to_cb -> addItem(tr("En bas de page"),					  "bottom");
	ui -> m_snap_to_cb -> addItem(tr("Sous le label de l'élément"), "label");

	m_previous_type_index = ui -> m_type_cb -> currentIndex();
}

/**
 * @brief XRefPropertiesWidget::saveProperties
 * Save the properties of the type define at @index of the combo box m_type_cb
 * @param index
 */
void XRefPropertiesWidget::saveProperties(int index) {
	QString type = ui->m_type_cb->itemData(index).toString();
	XRefProperties xrp = m_properties[type];

	if		(ui->m_display_has_cross_rb->isChecked())	 xrp.setDisplayHas(XRefProperties::Cross);
	else if (ui->m_display_has_contacts_rb->isChecked()) xrp.setDisplayHas(XRefProperties::Contacts);
	if (ui->m_snap_to_cb->itemData(ui->m_snap_to_cb->currentIndex()).toString() == "bottom")
		 xrp.setSnapTo(XRefProperties::Bottom);
	else xrp.setSnapTo(XRefProperties::Label);
	xrp.setShowPowerContac(ui->m_show_power_cb->isChecked());
	xrp.setPrefix("power",  ui->m_power_prefix_le->text());
	xrp.setPrefix("delay",  ui->m_delay_prefix_le->text());
	xrp.setPrefix("switch", ui->m_switch_prefix_le->text());
	xrp.setMasterLabel(ui->m_master_le->text());
	xrp.setSlaveLabel(ui->m_slave_le->text());
	xrp.setOffset(ui->m_offset_sb->value());

	m_properties.insert(type, xrp);
}

/**
 * @brief XRefPropertiesWidget::updateDisplay
 * Update display with the curent displayed type.
 */
void XRefPropertiesWidget::updateDisplay() {
	QString type = ui->m_type_cb->itemData(ui->m_type_cb->currentIndex()).toString();
	XRefProperties xrp = m_properties[type];

	XRefProperties::DisplayHas dh = xrp.displayHas();
	if		(dh == XRefProperties::Cross)	 {
		ui->m_display_has_cross_rb->setChecked(true);
	}
	else if (dh == XRefProperties::Contacts) {
		ui->m_display_has_contacts_rb->setChecked(true);
	}

	QString master = xrp.masterLabel();
	ui->m_master_le->setText(master);

	QString slave = xrp.slaveLabel();
	ui->m_slave_le->setText(slave);

	int offset = xrp.offset();
	ui->m_offset_sb->setValue(offset);

	if (xrp.snapTo() == XRefProperties::Bottom)
		 ui->m_snap_to_cb->setCurrentIndex(ui->m_snap_to_cb->findData("bottom"));
	else ui->m_snap_to_cb->setCurrentIndex(ui->m_snap_to_cb->findData("label"));
	ui->m_show_power_cb->setChecked(xrp.showPowerContact());
	ui->m_power_prefix_le-> setText(xrp.prefix("power"));
	ui->m_delay_prefix_le-> setText(xrp.prefix("delay"));
	ui->m_switch_prefix_le->setText(xrp.prefix("switch"));
	ui->m_cross_properties_gb->setDisabled(!ui->m_display_has_cross_rb->isChecked());
}

/**
 * @brief XRefPropertiesWidget::typeChanged
 * manage the save of the current properties,
 * when the combo box of type change.
 */
void XRefPropertiesWidget::typeChanged() {
	//save the properties of the previous xref type
	saveProperties(m_previous_type_index);
	//update display with the current xref type
	updateDisplay();
	//everything is done
	//previous index is now the current index
	m_previous_type_index = ui->m_type_cb->currentIndex();
}
