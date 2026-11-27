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
#include "qetgraphicsitem/terminalelement.h"
#include "diagram.h"
#include "qetproject.h"
#include "undocommand/changeelementdatacommand.h"
#include "qetinformation.h"
#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalcatalog.h"
#include "TerminalStrip/terminalcolor.h"
#include "TerminalStrip/physicalterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "TerminalStrip/terminalstripautoimport.h"
#include "TerminalStrip/UndoCommand/addterminalstripcommand.h"
#include "TerminalStrip/UndoCommand/addterminaltostripcommand.h"
#include "TerminalStrip/UndoCommand/groupterminalscommand.h"
#include "TerminalStrip/UndoCommand/changeterminallevel.h"

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
	ui->m_stage_cb->addItem(tr("0 (PE)"), 0);
	for (int stage = 1 ; stage <= 3 ; ++stage) {
		ui->m_stage_cb->addItem(QString::number(stage), stage);
	}

	ui->m_width_cb->addItem(tr("None (default)"), 0.0);
	ui->m_manufacturer_cb->addItem(QString());
	for (const auto &name : TerminalCatalog::manufacturers()) {
		ui->m_manufacturer_cb->addItem(name, name);
	}

		//The stage follows the form as soon as it is chosen, without
		//waiting for the terminal to be applied and picked again. The
		//jump to the first signal stage belongs to this click alone:
		//what a terminal which already is one carries is read back in
		//updateUi()
	connect(ui->m_form_cb, QOverload<int>::of(&QComboBox::activated),
			this, [this](int) {
				const bool was_multi_ = ui->m_stage_cb->isEnabled();
				updateStageControl();
				if (!was_multi_ && ui->m_stage_cb->isEnabled()
					&& ui->m_stage_cb->currentIndex() == 0) {
					ui->m_stage_cb->setCurrentIndex(1);
				}
			});

	setElement(elmt);
}

/**
	@brief TerminalPropertiesWidget::updateStageControl
	Free the stage combo only for a multi-level terminal and take it
	back to the standard value 0 when the form leaves the multi-level
	terminals.
*/
void TerminalPropertiesWidget::updateStageControl()
{
	const bool multi_ = ui->m_form_cb->currentData().value<ElementData::TerminalForm>()
			== ElementData::TFormMultiLevel;
	ui->m_stage_cb->setEnabled(multi_);
	if (!multi_) {
		ui->m_stage_cb->setCurrentIndex(0);
	}
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

/// @return the terminal element edited, when the element is one
TerminalElement *TerminalPropertiesWidget::terminalElement() const
{
	return qobject_cast<TerminalElement *>(m_element.data());
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

	const bool data_changed = new_data != m_element->elementData();

		//The stage of the terminal, and the block it belongs to: the
		//stages of a block are one after the other (2, 1, 0 or 0, 1, 2),
		//a number which does not continue the block before it starts a
		//new one
	const auto real_terminal = terminalElement() ? terminalElement()->realTerminal() : nullptr;
	const bool multi_ = new_data.terminalForm() == ElementData::TFormMultiLevel;
	const int wanted_ = ui->m_stage_cb->currentData().toInt();
	const bool stage_changed = real_terminal && multi_
			&& wanted_ != real_terminal->stage();
		//Leaving the multi-level terminals takes the stage back to the
		//foot and the terminal out of its block
	const bool leave_block_ = real_terminal && !multi_
			&& real_terminal->stage() != 0;

	if (!data_changed && !stage_changed && !leave_block_) {
		return nullptr;
	}

	auto *macro = new QUndoCommand(tr("Terminal properties"));
	if (data_changed) {
		new ChangeElementDataCommand(m_element, new_data, macro);
	}
	if (!real_terminal) {
		return macro;
	}

	auto strip = real_terminal->parentStrip();
	if (leave_block_) {
		if (strip) {
			new UnGroupTerminalsCommand(strip, {real_terminal}, macro);
		}
			//The stage of a terminal is kept even when it is on no
			//strip yet: the command works on the terminal alone then
		new ChangeTerminalLevel(strip, real_terminal, 0, macro);
		return macro;
	}

	if (stage_changed)
	{
			//A terminal on no folio strip comes into the strip its
			//label asks for ("X1:5" -> the strip "X1"), the same way
			//"Assign free terminals" does it
		if (!strip && m_element->diagram() && m_element->diagram()->project())
		{
			QETProject *project = m_element->diagram()->project();
			const QString name_ = TerminalStripAutoImport::stripNameForLabel(real_terminal->label());
			for (auto candidate : project->terminalStrip()) {
				if (candidate && candidate->name() == name_) {
					strip = candidate;
					break;
				}
			}
			if (!strip && !name_.isEmpty())
			{
				const auto place_ = RealTerminal::placeOf(m_element.data());
				auto new_strip = QSharedPointer<TerminalStrip>::create(project);
				new_strip->setName(name_);
				new_strip->setInstallation(place_.first);
				new_strip->setLocation(place_.second);
				new AddTerminalStripCommand(new_strip.data(), project, macro);
				strip = new_strip.data();
			}
			if (strip) {
				new AddTerminalToStripCommand(real_terminal, strip, macro);
			}
		}

		if (strip)
		{
				//The block of the terminal before this one in the
				//strip, when its stages continue into this one
			const auto reals_ = strip->realTerminals();
			const int index_ = reals_.indexOf(real_terminal);
			const auto previous_ = index_ > 0 ? reals_.at(index_ - 1) : nullptr;
			const auto block_ = previous_ ? previous_->physicalTerminal() : nullptr;
			if (block_ && block_ != real_terminal->physicalTerminal()
				&& previous_->element()
				&& previous_->element()->elementData().terminalForm() == ElementData::TFormMultiLevel)
			{
				bool taken_ = false;
				int min_ = 0, max_ = 0, count_ = 0;
				for (const auto &member : block_->realTerminals()) {
					const int stage_ = member->stage();
					if (stage_ == wanted_) {
						taken_ = true;
					}
					min_ = count_ ? qMin(min_, stage_) : stage_;
					max_ = count_ ? qMax(max_, stage_) : stage_;
					++count_;
				}
				const bool continues_ = count_ > 0
						&& (wanted_ == min_ - 1 || wanted_ == max_ + 1);
				if (!taken_ && continues_) {
					new GroupTerminalsCommand(strip, block_, {real_terminal}, macro);
				}
			}
		}
			//The stage of the terminal is kept either way, on a terminal
			//of a strip or on one which is on no strip yet
		new ChangeTerminalLevel(strip, real_terminal, wanted_, macro);
	}

	return macro;
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

	const auto real_terminal = terminalElement() ? terminalElement()->realTerminal() : nullptr;
	const int stage_index = ui->m_stage_cb->findData(real_terminal ? real_terminal->stage() : 0);
	ui->m_stage_cb->setCurrentIndex(stage_index > 0 ? stage_index : 0);
	updateStageControl();

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
						  ui->m_stage_cb, ui->m_section_cb, ui->m_width_cb,
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
						  ui->m_stage_cb, ui->m_section_cb, ui->m_width_cb,
						  ui->m_manufacturer_cb, ui->m_color_cb}) {
		disconnect(cb, QOverload<int>::of(&QComboBox::activated), this, &TerminalPropertiesWidget::apply);
		if (cb->isEditable()) {
			disconnect(cb->lineEdit(), &QLineEdit::editingFinished, this, &TerminalPropertiesWidget::apply);
		}
	}
	disconnect(ui->m_led_cb, &QCheckBox::clicked, this, &TerminalPropertiesWidget::apply);
}
