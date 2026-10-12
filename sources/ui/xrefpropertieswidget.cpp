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

#include <utility>
#include <QFontDialog>
#include <QHash>
#include <QMetaEnum>
#include <QPushButton>

#include "xrefpropertieswidget.h"
#include "ui_xrefpropertieswidget.h"
#include "qdebug.h"
#include "../qetapp.h"
#include "../utils/qetutils.h"

/**
	@brief XRefPropertiesWidget::XRefPropertiesWidget
	Default constructor
	@param properties: properties to use
	@param parent: parent widget
*/
XRefPropertiesWidget::XRefPropertiesWidget(QHash <QString, XRefProperties> properties, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::XRefPropertiesWidget),
	m_properties(std::move(properties))
{
	ui->setupUi(this);
	buildUi();
	fillMissingTypes();
	connect(ui->m_display_has_cross_rb, &QRadioButton::toggled, ui->m_cross_properties_gb, &QWidget::setEnabled);
	connect(ui->m_display_has_contacts_rb, &QRadioButton::toggled, ui->m_show_all_slaves_cb, &QWidget::setEnabled);
	connect(ui->m_type_cb, qOverload<int>(&QComboBox::currentIndexChanged), this, &XRefPropertiesWidget::typeChanged);
	connect(ui->m_snap_to_cb, qOverload<int>(&QComboBox::currentIndexChanged), this, &XRefPropertiesWidget::enableOffsetSB);
	connect(ui->m_font_pb, &QPushButton::clicked, this, &XRefPropertiesWidget::chooseXRefFont);
	updateDisplay();
}

/**
	@brief XRefPropertiesWidget::~XRefPropertiesWidget
	Default destructor
*/
XRefPropertiesWidget::~XRefPropertiesWidget()
{
	disconnect(ui->m_display_has_cross_rb, &QRadioButton::toggled, ui->m_cross_properties_gb, &QWidget::setEnabled);
	disconnect(ui->m_display_has_contacts_rb, &QRadioButton::toggled, ui->m_show_all_slaves_cb, &QWidget::setEnabled);
	disconnect(ui->m_type_cb, qOverload<int>(&QComboBox::currentIndexChanged), this, &XRefPropertiesWidget::typeChanged);
	disconnect(ui->m_snap_to_cb, qOverload<int>(&QComboBox::currentIndexChanged), this, &XRefPropertiesWidget::enableOffsetSB);	
	disconnect(ui->m_font_pb, &QPushButton::clicked, this, &XRefPropertiesWidget::chooseXRefFont);
	delete ui;
}

/**
	@brief XRefPropertiesWidget::setProperties
	set new properties for this widget
	@param properties
*/
void XRefPropertiesWidget::setProperties(const QHash <QString,
					 XRefProperties> &properties) {
	m_properties = properties;
	fillMissingTypes();
	updateDisplay();
	m_previous_type_index = ui->m_type_cb->currentIndex();
}

/**
	@brief XRefPropertiesWidget::fillMissingTypes
	Make sure every type the combo box offers has properties of its own.

	A project built before a type existed does not hold that type, and
	neither does a settings file written before it. Without an entry the
	form would make one up out of thin air, and for a cable that would be
	the very default of the coil ("%f-%l%c"), which reads as a stray dash
	behind the folio number and two empty numbers after it.
*/
void XRefPropertiesWidget::fillMissingTypes()
{
	const QHash<QString, XRefProperties> defaults = XRefProperties::defaultProperties();
	for (int i = 0; i < ui->m_type_cb->count(); ++i)
	{
		const QString type = ui->m_type_cb->itemData(i).toString();
		if (!m_properties.contains(type)) {
			m_properties.insert(type, defaults.value(type));
		}
	}
}

/**
	@brief XRefPropertiesWidget::properties
	@return the properties edited by this widget
*/
QHash <QString, XRefProperties> XRefPropertiesWidget::properties()
{
	saveProperties(ui->m_type_cb->currentIndex());
	return m_properties;
}

/**
	@brief XRefPropertiesWidget::setReadOnly
	Set all of this widget disable if true
	@param ro
*/
void XRefPropertiesWidget::setReadOnly(bool ro) {
	ui->m_type_cb->setDisabled(ro);
	ui->m_display_gb->setDisabled(ro);
	ui->m_cross_properties_gb->setDisabled(ro);
		//The texts stand on their own since they are needed for a cable
		//too, and they are as much a setting as the rest.
	ui->m_labels_gb->setDisabled(ro);

	if (!ro && ui->m_display_has_contacts_rb->isChecked()) {
		ui->m_cross_properties_gb->setDisabled(true);
	}
}

/**
	@brief XRefPropertiesWidget::buildUi
	Build some widget of this ui.
*/
void XRefPropertiesWidget::buildUi()
{
	//Wording of the master label as the .ui wrote it, kept to put it
	//back for every type which does not rename it.
	m_master_label_text = ui->label_6->text();
	ui -> m_type_cb -> addItem(tr("Coil"), "coil");
	ui -> m_type_cb -> addItem(tr("Organ of protection"), "protection");
	ui -> m_type_cb -> addItem(tr("Switch / button"), "commutator");
	ui -> m_type_cb -> addItem(tr("Programmable Logic Controller (PLC)"), "plc");
		//Not an element: the type of the cross references a cable writes
		//in its own label when it runs on several folios.
	ui -> m_type_cb -> addItem(tr("Cable"), "cable");

	ui -> m_snap_to_cb -> addItem(tr("Footer"), "bottom");
	ui -> m_snap_to_cb -> addItem(tr("Under the label of the element"), "label");

	ui -> m_xrefpos_cb -> addItem(tr("Top"),"top");
	ui -> m_xrefpos_cb -> addItem(tr("Bottom"),"bottom");
	ui -> m_xrefpos_cb -> addItem(tr("Left"),"left");
	ui -> m_xrefpos_cb -> addItem(tr("Right"),"right");
	ui -> m_xrefpos_cb -> addItem(tr("Text alignment"),"alignment");
	ui -> m_xrefpos_cb -> addItem(tr("Text field"),"text_field");
	m_previous_type_index = ui -> m_type_cb -> currentIndex();
}

/**
	@brief XRefPropertiesWidget::saveProperties
	Save the properties of the type define at index of the combo box m_type_cb
	@param index
*/
void XRefPropertiesWidget::saveProperties(int index) {
	QString type = ui->m_type_cb->itemData(index).toString();
	XRefProperties xrp = m_properties[type];

	if (ui->m_display_has_cross_rb->isChecked())
		xrp.setDisplayHas(XRefProperties::Cross);
	else if (ui->m_display_has_contacts_rb->isChecked())
		xrp.setDisplayHas(XRefProperties::Contacts);
	if (ui->m_snap_to_cb->itemData(
				ui->m_snap_to_cb->currentIndex()).toString()
			== "bottom")
		xrp.setSnapTo(XRefProperties::Bottom);
	else
		xrp.setSnapTo(XRefProperties::Label);

	if(ui->m_xrefpos_cb->itemData(ui->m_xrefpos_cb->currentIndex()).toString() == "bottom") xrp.setXrefPos(Qt::AlignBottom);
	else if(ui->m_xrefpos_cb->itemData(ui->m_xrefpos_cb->currentIndex()).toString() == "top") xrp.setXrefPos(Qt::AlignTop);
	else if(ui->m_xrefpos_cb->itemData(ui->m_xrefpos_cb->currentIndex()).toString() == "left") xrp.setXrefPos(Qt::AlignLeft);
	else if(ui->m_xrefpos_cb->itemData(ui->m_xrefpos_cb->currentIndex()).toString() == "right") xrp.setXrefPos(Qt::AlignRight);
	else if(ui->m_xrefpos_cb->itemData(ui->m_xrefpos_cb->currentIndex()).toString() == "alignment") xrp.setXrefPos(Qt::AlignBaseline);
	else if(ui->m_xrefpos_cb->itemData(ui->m_xrefpos_cb->currentIndex()).toString() == "text_field") xrp.setXrefPos(Qt::AlignHCenter);
	xrp.setShowPowerContac(ui->m_show_power_cb->isChecked());
	xrp.setShowTerminalName(ui->m_show_terminal_name_cb->isChecked());
	xrp.setShowAllConfiguredSlaves(ui->m_show_all_slaves_cb->isChecked());
	xrp.setStackOverlapping(ui->m_stack_overlapping_cb->isChecked());
	xrp.setPrefix("power",  ui->m_power_prefix_le->text());
	xrp.setPrefix("delay",  ui->m_delay_prefix_le->text());
	xrp.setPrefix("switch", ui->m_switch_prefix_le->text());
	xrp.setMasterLabel(ui->m_master_le->text());
	xrp.setSlaveLabel(ui->m_slave_le->text());
	xrp.setFont(m_current_font);
		//The boxes cannot show a value below their minimum (the offset's
		//minimum is its "Default" entry, standing for the stored 0): keep
		//the stored value unless the box shows something else.
	if (ui->m_offset_sb->value() != qBound(ui->m_offset_sb->minimum(), xrp.offset(),
										   ui->m_offset_sb->maximum()))
		xrp.setOffset(ui->m_offset_sb->value());
	if (ui->m_slave_offset_sb->value() != qBound(ui->m_slave_offset_sb->minimum(), xrp.slaveOffset(),
												 ui->m_slave_offset_sb->maximum()))
		xrp.setSlaveOffset(ui->m_slave_offset_sb->value());

	m_properties.insert(type, xrp);
}

/**
	@brief XRefPropertiesWidget::updateDisplay
	Update display with the current displayed type.
*/
void XRefPropertiesWidget::updateDisplay()
{
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

		//Which font that type's reference is written with: the button
		//offers it for a cable alone, but every type carries its own.
	m_current_font = xrp.font();

	QString slave = xrp.slaveLabel();
	ui->m_slave_le->setText(slave);

	int offset = xrp.offset();
	ui->m_offset_sb->setValue(offset);

	int slave_offset = xrp.slaveOffset();
	ui->m_slave_offset_sb->setValue(slave_offset);

	if (xrp.snapTo() == XRefProperties::Bottom){
		 ui->m_snap_to_cb->setCurrentIndex(ui->m_snap_to_cb->findData("bottom"));
		 ui->m_offset_sb->setEnabled(true);
	}
	else {
		ui->m_snap_to_cb->setCurrentIndex(ui->m_snap_to_cb->findData("label"));
		ui->m_offset_sb->setEnabled(false);
	}
	ui->m_stack_overlapping_cb->setChecked(xrp.stackOverlapping());
	ui->m_stack_overlapping_cb->setEnabled(ui->m_offset_sb->isEnabled());

	if(xrp.getXrefPos() == Qt::AlignTop) ui->m_xrefpos_cb->setCurrentIndex(ui->m_xrefpos_cb->findData("top"));
	else if(xrp.getXrefPos() == Qt::AlignLeft) ui->m_xrefpos_cb->setCurrentIndex(ui->m_xrefpos_cb->findData("left"));
	else if(xrp.getXrefPos() == Qt::AlignRight) ui->m_xrefpos_cb->setCurrentIndex(ui->m_xrefpos_cb->findData("right"));
	else if(xrp.getXrefPos() == Qt::AlignBaseline) ui->m_xrefpos_cb->setCurrentIndex(ui->m_xrefpos_cb->findData("alignment"));
	else if(xrp.getXrefPos() == Qt::AlignBottom) ui->m_xrefpos_cb->setCurrentIndex(ui->m_xrefpos_cb->findData("bottom"));
	else if(xrp.getXrefPos() == Qt::AlignHCenter) ui->m_xrefpos_cb->setCurrentIndex(ui->m_xrefpos_cb->findData("text_field"));
	ui->m_show_power_cb->setChecked(xrp.showPowerContact());
	ui->m_show_terminal_name_cb->setChecked(xrp.showTerminalName());
	ui->m_show_all_slaves_cb->setChecked(xrp.showAllConfiguredSlaves());
	//The radio button only emits toggled() when it really changes: loading
	//a type whose display did not change left the checkbox with the enabled
	//state of the previously displayed type (it stayed clickable although
	//the cross display was selected). Set the state explicitly here.
	ui->m_show_all_slaves_cb->setEnabled(
				ui->m_display_has_contacts_rb->isChecked());
	ui->m_power_prefix_le-> setText(xrp.prefix("power"));
	ui->m_delay_prefix_le-> setText(xrp.prefix("delay"));
	ui->m_switch_prefix_le->setText(xrp.prefix("switch"));
	ui->m_cross_properties_gb->setDisabled(!ui->m_display_has_cross_rb->isChecked());

	//The cross ref of a PLC master is always drawn as its IO table, and
	//the slaves are referenced directly into that table: the contacts/
	//cross choice, the two display checkboxes and the cross options below
	//have no effect at all for this type, so they are hidden instead of
	//being offered for nothing. The positioning settings and the labels
	//(the table really uses them) stay available.
	const bool is_plc = type == QLatin1String("plc");
	ui->m_display_has_contacts_rb->setVisible(!is_plc);
	ui->m_display_has_cross_rb->setVisible(!is_plc);
	ui->m_show_terminal_name_cb->setVisible(!is_plc);
	ui->m_show_all_slaves_cb->setVisible(!is_plc);

	//A cable is not an element: it draws no cross of its own and stands
	//at no place of the sheet to be pointed at. Its reference is a line
	//of the label of the cable itself, written once per other folio the
	//cable runs on, so the whole presentation (where a cross is put, how
	//it is drawn) and the text of the slave have no meaning for it: what
	//is left is the one text which is written into that line.
	const bool is_cable = type == QLatin1String("cable");
	ui->m_display_gb->setVisible(!is_cable);
	ui->label_7->setVisible(!is_cable);
	ui->m_slave_le->setVisible(!is_cable);
		//The wording of every type which is not a cable comes back from
	//the .ui, so the label cannot drift away from the designer one.
	ui->label_6->setText(is_cable ? tr("Text:") : m_master_label_text);

	//Only a cable writes its reference into its own label, so only a
	//cable has a text whose font to choose here: the cross references
	//of the elements keep the font they have always been drawn with.
	ui->m_font_pb->setVisible(is_cable);
	if (m_current_font.isEmpty())
	{
		ui->m_font_pb->setToolTip(tr("Font and size: those of the cable texts"));
	}
	else
	{
		QFont font;
		if (QETUtils::fontFromString(font, m_current_font)) {
			ui->m_font_pb->setToolTip(tr("Font and size: %1, %2 pt")
									  .arg(font.family()).arg(font.pointSizeF()));
		}
	}

		//A cable stacks the lines of its references by itself, so the
		//option is only worth showing for the elements.
	ui->m_stack_overlapping_cb->setVisible(!is_plc && !is_cable);
	ui->m_cross_properties_gb->setVisible(!is_plc && !is_cable);
}

/**
	@brief XRefPropertiesWidget::typeChanged
	manage the save of the current properties,
	when the combo box of type change.
*/
void XRefPropertiesWidget::typeChanged()
{
	//save the properties of the previous xref type
	saveProperties(m_previous_type_index);
	//update display with the current xref type
	updateDisplay();
	//everything is done
	//previous index is now the current index
	m_previous_type_index = ui->m_type_cb->currentIndex();
}

/**
	@brief XRefPropertiesWidget::enableOffsetSB
	Enable Offset SB only if Snap to Footer is selected
*/
void XRefPropertiesWidget::enableOffsetSB(int i){
	if (i)
		ui->m_offset_sb->setEnabled(false);
	else
		ui->m_offset_sb->setEnabled(true);
	ui->m_stack_overlapping_cb->setEnabled(ui->m_offset_sb->isEnabled());
}

/**
	@brief XRefPropertiesWidget::chooseXRefFont
	Let him pick the font -- family, size and style, all three at once --
	the reference of a cable is written with.

	The font is only kept when he presses OK: cancelling the dialog
	leaves the previous one standing. What he picked is written into the
	properties by saveProperties(), the way every other field of this
	form is, so leaving the type or pressing OK in the settings window
	is what really stores it.
*/
void XRefPropertiesWidget::chooseXRefFont()
{
	QFont initial;
	if (m_current_font.isEmpty() || !QETUtils::fontFromString(initial, m_current_font)) {
		initial = QETApp::cableTextsFont();
	}

	bool ok = false;
	const QFont font = QFontDialog::getFont(&ok, initial, this,
											tr("Font of the cross-reference text"));
	if (!ok) return;

	m_current_font = QETUtils::fontToString(font);
	ui->m_font_pb->setToolTip(tr("Font and size: %1, %2 pt")
							  .arg(font.family()).arg(font.pointSizeF()));
}
