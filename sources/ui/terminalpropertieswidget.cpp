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
#include "terminalpropertieswidget.h"
#include "ui_terminalpropertieswidget.h"

#include "qetgraphicsitem/element.h"
#include "diagram.h"
#include "undocommand/changeelementdatacommand.h"
#include "qetinformation.h"
#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalcatalog.h"
#include "TerminalStrip/terminalcolor.h"

#include <QIcon>
#include <QLineEdit>
#include <QRegularExpression>

/**
	The size and color combos read like the ones of the terminal group of
	the symbol editor: a value can be picked in the list or typed in.
*/

/// @return the number at the start of \p text, or 0 if it has none
static double leadingNumber(const QString &text)
{
	static const QRegularExpression re(QStringLiteral("(\\d+(?:[.,]\\d+)?)"));
	const auto match = re.match(text);
	if (!match.hasMatch()) {
		return 0;
	}
	auto number = match.captured(1);
	number.replace(QLatin1Char(','), QLatin1Char('.'));
	return number.toDouble();
}

/// Value of a size combo (picked in the list or typed).
static double comboValue(const QComboBox *cb)
{
	return leadingNumber(cb->currentText());
}

/// Color of the color combo as "#rrggbb" or "#rrggbb/#rrggbb" (empty = none).
/// A text which is not understood keeps the value the combo was loaded with.
static QString colorSpec(const QComboBox *cb)
{
	const auto index = cb->findText(cb->currentText());
	if (index >= 0) {
		return cb->itemData(index).toString();
	}

	QColor main_, stripe_;
	if (TerminalColor::parse(cb->currentText(), &main_, &stripe_)) {
		return TerminalColor::toSpec(main_, stripe_);
	}
	return cb->property("origSpec").toString();
}

/// Select the item with the value \p value in a size combo, or write the
/// value: a size typed by hand is kept as text, so that it does not take
/// the first terminal of the catalog with the same size.
static void setComboValue(QComboBox *cb, double value, const QString &suffix)
{
	if (value <= 0) {
		cb->setCurrentIndex(0);
		return;
	}
	const auto index = cb->findData(value);
	if (index >= 0) {
		cb->setCurrentIndex(index);
		return;
	}
	cb->setCurrentText(QLocale().toString(value, 'g', 6) + suffix);
}

/**
	@brief TerminalPropertiesWidget::TerminalPropertiesWidget
	Constructor
	@param elmt : the terminal to show the properties of
	@param parent : parent widget
*/
TerminalPropertiesWidget::TerminalPropertiesWidget(Element *elmt, QWidget *parent) :
	AbstractElementPropertiesEditorWidget(parent),
	ui(new Ui::TerminalPropertiesWidget)
{
	ui->setupUi(this);

		//The three choices of a terminal, each listed in the order
		//of its enum, so the combo holds the value as its data
	for (const auto type : {ElementData::TTGeneric, ElementData::TTFuse,
							ElementData::TTSectional, ElementData::TTDiode,
							ElementData::TTGround, ElementData::TTAntiParallelDiodes,
							ElementData::TTResistor, ElementData::TTVaristor,
							ElementData::TTTestSocket, ElementData::TTDiodeReversed}) {
		ui->m_type_cb->addItem(ElementData::translatedTerminalType(type), type);
	}
	for (const auto function : {ElementData::TFGeneric, ElementData::TFPhase,
								ElementData::TFNeutral}) {
		ui->m_function_cb->addItem(ElementData::translatedTerminalFunction(function), function);
	}
	for (const auto form : {ElementData::TFormNone, ElementData::TFormFeedThrough2,
							ElementData::TFormFeedThrough3, ElementData::TFormFeedThrough4,
							ElementData::TFormMultiLevel, ElementData::TFormSectionable,
							ElementData::TFormFuse, ElementData::TFormBlock,
							ElementData::TFormSensorActuator, ElementData::TFormPluggable}) {
		ui->m_form_cb->addItem(ElementData::translatedTerminalForm(form), form);
	}

	ui->m_section_cb->addItem(tr("None (default)"), 0.0);
	for (const auto section : TerminalCatalog::sections()) {
		ui->m_section_cb->addItem(
			TerminalCatalog::sectionText(section, tr("%1 mm²").arg(QLocale().toString(section, 'g', 6))),
			section);
	}
	ui->m_width_cb->addItem(tr("None (default)"), 0.0);
	ui->m_manufacturer_cb->addItem(QString());
	for (const auto &name : TerminalCatalog::manufacturers()) {
		ui->m_manufacturer_cb->addItem(name, name);
	}

	setElement(elmt);
}

TerminalPropertiesWidget::~TerminalPropertiesWidget()
{
	delete ui;
}

/**
	@brief TerminalPropertiesWidget::setElement
	Set the edited terminal
	@param element : the terminal to show the properties of
*/
void TerminalPropertiesWidget::setElement(Element *element)
{
	m_element = element;
	updateUi();
}

/**
	@brief TerminalPropertiesWidget::apply
	Apply the edited properties as one undoable change
*/
void TerminalPropertiesWidget::apply()
{
	if (auto undo = associatedUndo()) {
		m_element->diagram()->undoStack().push(undo);
	}
}

/**
	@brief TerminalPropertiesWidget::reset
	Forget the edits not applied yet: show again the properties
	the terminal has.
*/
void TerminalPropertiesWidget::reset()
{
	if (m_element) {
		updateUi();
	}
}

/**
	@brief TerminalPropertiesWidget::associatedUndo
	@return a command with the change if the edited properties differ
	from the ones of the terminal, else nullptr
*/
QUndoCommand *TerminalPropertiesWidget::associatedUndo() const
{
	if (!m_element) {
		return nullptr;
	}

	auto new_data = m_element->elementData();
	new_data.setTerminalType(ui->m_type_cb->currentData().value<ElementData::TerminalType>());
	new_data.setTerminalFunction(ui->m_function_cb->currentData().value<ElementData::TerminalFunction>());
	new_data.setTerminalForm(ui->m_form_cb->currentData().value<ElementData::TerminalForm>());
	new_data.setTerminalLED(ui->m_led_cb->isChecked());

		//A value which is empty again is stored empty when the key is
		//there, the same way the terminal strip manager writes it
	const auto set_info = [&new_data](const QString &key, const QString &value) {
		if (!value.isEmpty() || new_data.m_informations.contains(key)) {
			new_data.m_informations.addValue(key, value);
		}
	};
		//The color and the size are stored with the dot as decimal
		//separator, whatever the locale
	set_info(RealTerminal::colorInfoKey(), colorSpec(ui->m_color_cb));
	const double section = comboValue(ui->m_section_cb);
	set_info(RealTerminal::sectionInfoKey(), section > 0 ? QString::number(section, 'g', 6) : QString());
	const double width = comboValue(ui->m_width_cb);
	set_info(RealTerminal::widthInfoKey(), width > 0 ? QString::number(width, 'g', 6) : QString());
	set_info(QETInformation::ELMT_MANUFACTURER, ui->m_manufacturer_cb->currentText().trimmed());

	if (new_data == m_element->elementData()) {
		return nullptr;
	}
	return new ChangeElementDataCommand(m_element, new_data);
}

/**
	@brief TerminalPropertiesWidget::updateUi
	Show the properties the terminal has
*/
void TerminalPropertiesWidget::updateUi()
{
	if (!m_element) {
		return;
	}
	const auto data = m_element->elementData();
	ui->m_type_cb->setCurrentIndex(ui->m_type_cb->findData(data.terminalType()));
	ui->m_function_cb->setCurrentIndex(ui->m_function_cb->findData(data.terminalFunction()));
	ui->m_form_cb->setCurrentIndex(ui->m_form_cb->findData(data.terminalForm()));
	ui->m_led_cb->setChecked(data.terminalLed());

	const auto info = data.m_informations;
	setComboValue(ui->m_section_cb, info.value(RealTerminal::sectionInfoKey()).toDouble(), tr(" mm²"));
	setComboValue(ui->m_width_cb, info.value(RealTerminal::widthInfoKey()).toDouble(), tr(" mm"));
	ui->m_manufacturer_cb->setCurrentText(info.value(QETInformation::ELMT_MANUFACTURER).toString());

		//The color combo is filled again: a color which is not in the
		//palette stays in the list as it is written
	const QString spec = info.value(RealTerminal::colorInfoKey()).toString();
	ui->m_color_cb->setProperty("origSpec", spec);
	ui->m_color_cb->clear();
	ui->m_color_cb->addItem(tr("None (default)"), QString());
	for (const auto &entry : TerminalColor::palette()) {
		QColor main_, stripe_;
		TerminalColor::parse(TerminalColor::specOf(entry), &main_, &stripe_);
		ui->m_color_cb->addItem(QIcon(TerminalColor::swatch(main_, stripe_)),
								TerminalColor::translatedName(entry),
								TerminalColor::specOf(entry));
	}
	const auto color_index = ui->m_color_cb->findData(spec);
	if (color_index >= 0) {
		ui->m_color_cb->setCurrentIndex(color_index);
	} else if (!spec.isEmpty()) {
		QColor main_, stripe_;
		TerminalColor::parse(spec, &main_, &stripe_);
		ui->m_color_cb->addItem(QIcon(TerminalColor::swatch(main_, stripe_)), spec, spec);
		ui->m_color_cb->setCurrentIndex(ui->m_color_cb->count() - 1);
	} else {
		ui->m_color_cb->setCurrentIndex(0);
	}
}

/**
	@brief TerminalPropertiesWidget::setLiveEdit
	@param live_edit : true to apply each change as it is made
*/
bool TerminalPropertiesWidget::setLiveEdit(bool live_edit)
{
	if (m_live_edit == live_edit) {
		return true;
	}
	m_live_edit = live_edit;

	if (m_live_edit) {
		enableLiveEdit();
	} else {
		disableLiveEdit();
	}

	return true;
}

void TerminalPropertiesWidget::enableLiveEdit()
{
	for (QComboBox *cb : {ui->m_type_cb, ui->m_function_cb, ui->m_form_cb,
						  ui->m_section_cb, ui->m_width_cb,
						  ui->m_manufacturer_cb, ui->m_color_cb}) {
		connect(cb, QOverload<int>::of(&QComboBox::activated), this, &TerminalPropertiesWidget::apply);
		if (cb->isEditable()) {
			connect(cb->lineEdit(), &QLineEdit::editingFinished, this, &TerminalPropertiesWidget::apply);
		}
	}
	connect(ui->m_led_cb, &QCheckBox::clicked, this, &TerminalPropertiesWidget::apply);
}

void TerminalPropertiesWidget::disableLiveEdit()
{
	for (QComboBox *cb : {ui->m_type_cb, ui->m_function_cb, ui->m_form_cb,
						  ui->m_section_cb, ui->m_width_cb,
						  ui->m_manufacturer_cb, ui->m_color_cb}) {
		disconnect(cb, QOverload<int>::of(&QComboBox::activated), this, &TerminalPropertiesWidget::apply);
		if (cb->isEditable()) {
			disconnect(cb->lineEdit(), &QLineEdit::editingFinished, this, &TerminalPropertiesWidget::apply);
		}
	}
	disconnect(ui->m_led_cb, &QCheckBox::clicked, this, &TerminalPropertiesWidget::apply);
}
