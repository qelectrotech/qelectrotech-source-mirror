/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "elementpropertieseditorwidget.h"
#include "ui_elementpropertieseditorwidget.h"

/**
 * @brief ElementPropertiesEditorWidget::ElementPropertiesEditorWidget
 * Default constructor
 * @param basic_type : QString of the drawed element
 * @param dc : DiagramContext to store kindInfo of drawed element
 * @param parent : parent widget
 */
ElementPropertiesEditorWidget::ElementPropertiesEditorWidget(QString &basic_type, DiagramContext &dc, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ElementPropertiesEditorWidget),
	m_basic_type(basic_type),
	m_dc (dc)
{
	ui->setupUi(this);
	setUpInterface();
	upDateInterface();
}

/**
 * @brief ElementPropertiesEditorWidget::~ElementPropertiesEditorWidget
 * Default destructor
 */
ElementPropertiesEditorWidget::~ElementPropertiesEditorWidget()
{
	delete ui;
}

/**
 * @brief ElementPropertiesEditorWidget::upDateInterface
 * Update the interface with the curent value
 */
void ElementPropertiesEditorWidget::upDateInterface() {
	ui -> m_base_type_cb -> setCurrentIndex (ui -> m_base_type_cb -> findData (QVariant(m_basic_type)));

	if (m_basic_type == "slave") {
		ui -> m_state_cb	 -> setCurrentIndex (ui -> m_state_cb	  -> findData (m_dc["state"].toString()));
		ui -> m_type_cb		 -> setCurrentIndex (ui -> m_type_cb	  -> findData (m_dc["type"].toString()));
		ui -> m_number_ctc	 -> setValue		(m_dc["number"].toInt());
	}

	else if (m_basic_type == "master") {
		ui -> m_master_type_cb -> setCurrentIndex (ui -> m_master_type_cb -> findData (m_dc["type"]));
	}

	on_m_base_type_cb_currentIndexChanged(ui->m_base_type_cb->currentIndex());
}

/**
 * @brief ElementPropertiesEditorWidget::setUpInterface
 */
void ElementPropertiesEditorWidget::setUpInterface() {
	// Type combo box
	ui -> m_base_type_cb -> addItem (tr("Simple"),							QVariant("simple"));
	ui -> m_base_type_cb -> addItem (tr("Ma\356tre"),						QVariant("master"));
	ui -> m_base_type_cb -> addItem (tr("Esclave"),							QVariant("slave"));
	ui -> m_base_type_cb -> addItem (tr("Renvoi de folio suivant"),			QVariant("next_report"));
	ui -> m_base_type_cb -> addItem (tr("Renvoi de folio pr\351c\351dent"), QVariant("previous_report"));
	ui -> m_base_type_cb -> addItem (tr("Bornier"),                         QVariant("terminal"));

	// Slave option
	ui -> m_state_cb -> addItem(tr("Normalement ouvert"),   QVariant("NO"));
	ui -> m_state_cb -> addItem(tr("Normalement ferm\351"), QVariant("NC"));
	ui -> m_state_cb -> addItem(tr("Inverseur"),			QVariant("SW"));
	ui -> m_type_cb  -> addItem(tr("Simple"),				QVariant("simple"));
	ui -> m_type_cb  -> addItem(tr("Puissance"),			QVariant("power"));
	ui -> m_type_cb  -> addItem(tr("Temporis\351 travail"), QVariant("delayOn"));
	ui -> m_type_cb  -> addItem(tr("Temporis\351 repos"),	QVariant("delayOff"));

	//Master option
	ui -> m_master_type_cb -> addItem(tr("Bobine"),				  QVariant("coil"));
	ui -> m_master_type_cb -> addItem(tr("Organe de protection"), QVariant("protection"));
}

/**
 * @brief ElementPropertiesEditorWidget::on_m_buttonBox_accepted
 * Action on button accepted : the new information is set
 */
void ElementPropertiesEditorWidget::on_m_buttonBox_accepted()
{
	m_basic_type = ui -> m_base_type_cb -> itemData(ui -> m_base_type_cb -> currentIndex()).toString();
	if (m_basic_type == "slave") {
		m_dc.addValue("state",  ui -> m_state_cb -> itemData(ui -> m_state_cb -> currentIndex()));
		m_dc.addValue("type",	ui -> m_type_cb  -> itemData(ui -> m_type_cb  -> currentIndex()));
		m_dc.addValue("number", QVariant(ui -> m_number_ctc -> value()));
	}
	else if(m_basic_type == "master") {
		m_dc.addValue("type", ui -> m_master_type_cb -> itemData(ui -> m_master_type_cb -> currentIndex()));
	}
	this->close();
}

/**
 * @brief ElementPropertiesEditorWidget::on_m_base_type_cb_currentIndexChanged
 * @param index : Action when combo-box base type index change
 */
void ElementPropertiesEditorWidget::on_m_base_type_cb_currentIndexChanged(int index)
{
	bool slave = false , master = false;

	if		(ui -> m_base_type_cb -> itemData(index).toString() == "slave")  slave  = true;
	else if (ui -> m_base_type_cb -> itemData(index).toString() == "master") master = true;

	ui -> m_slave_gb  -> setVisible(slave);
	ui -> m_master_gb -> setVisible(master);
}
