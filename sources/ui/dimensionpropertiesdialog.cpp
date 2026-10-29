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
#include "dimensionpropertiesdialog.h"
#include "ui_dimensionpropertiesdialog.h"

#include "../diagram.h"
#include "../qet.h"

#include <QPushButton>
#include <QUndoCommand>

class ChangeDimensionPropertiesCommand : public QUndoCommand
{
public:
	ChangeDimensionPropertiesCommand(DimensionItem *item,
	                                 const DimensionProperties &oldProps,
	                                 const DimensionProperties &newProps,
	                                 QUndoCommand *parent = nullptr) :
		QUndoCommand(parent),
		m_item(item),
		m_old_props(oldProps),
		m_new_props(newProps)
	{
		setText(QObject::tr("Edit dimension properties"));
	}

	void undo() override {
		if (m_item) {
			m_item->setProperties(m_old_props);
		}
	}

	void redo() override {
		if (m_item) {
			m_item->setProperties(m_new_props);
		}
	}

private:
	QPointer<DimensionItem> m_item;
	DimensionProperties m_old_props;
	DimensionProperties m_new_props;
};

DimensionPropertiesDialog::DimensionPropertiesDialog(DimensionItem *item, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DimensionPropertiesDialog),
	m_primary_item(item)
{
	ui->setupUi(this);
	QET::trackDialogGeometry(this);

	ui->m_prefix_cb->addItems({QString(), QStringLiteral("⌀"), QStringLiteral("R"), QStringLiteral("M"), QStringLiteral("L="), QStringLiteral("B="), QStringLiteral("H=")});
	ui->m_suffix_cb->addItems({QString(), QStringLiteral(" mm"), QStringLiteral(" cm"), QStringLiteral(" m"), QStringLiteral("°")});
	ui->m_unit_cb->addItems({QStringLiteral("mm"), QStringLiteral("cm"), QStringLiteral("m"), QStringLiteral("in")});
	ui->m_precision_cb->addItems({QStringLiteral("0"), QStringLiteral("0,0"), QStringLiteral("0,00"), QStringLiteral("0,000")});
	ui->m_terminations_cb->addItem(tr("Arrows"), DimensionItem::Arrows);
	ui->m_terminations_cb->addItem(tr("Architectural Slashes"), DimensionItem::Slashes);

	if (item) {
		m_segments = item->findChainSegments();
	}
	if (m_segments.isEmpty() && item) {
		m_segments.append(item);
	}

	for (int i = 0; i < m_segments.size(); ++i) {
		DimensionItem *it = m_segments.at(i);
		ui->m_segments_list->addItem(QString::number(i + 1));
		m_original_props[it] = it->properties();
		m_current_props[it] = it->properties();
	}

	connect(ui->m_segments_list, &QListWidget::currentRowChanged,
	        this, &DimensionPropertiesDialog::onSegmentSelected);
	connect(ui->m_auto_cb, &QCheckBox::toggled,
	        this, &DimensionPropertiesDialog::onAutoToggled);
	connect(ui->m_precision_cb, QOverload<int>::of(&QComboBox::currentIndexChanged),
	        this, &DimensionPropertiesDialog::onPrecisionChanged);
	connect(ui->m_comma_cb, &QCheckBox::toggled,
	        this, &DimensionPropertiesDialog::onCommaToggled);
	connect(ui->m_suppress_zero_cb, &QCheckBox::toggled,
	        this, &DimensionPropertiesDialog::updateValueDisplay);

	QPushButton *applyBtn = ui->m_buttonBox->button(QDialogButtonBox::Apply);
	if (applyBtn) {
		connect(applyBtn, &QPushButton::clicked, this, &DimensionPropertiesDialog::onApplyClicked);
	}

	int initialIdx = m_segments.indexOf(item);
	ui->m_segments_list->setCurrentRow(initialIdx >= 0 ? initialIdx : 0);
}

DimensionPropertiesDialog::~DimensionPropertiesDialog()
{
	delete ui;
}

void DimensionPropertiesDialog::onSegmentSelected(int row)
{
	if (row < 0 || row >= m_segments.size()) return;

	if (m_active_item) {
		saveItemProperties(m_active_item);
	}

	m_active_item = m_segments.at(row);
	loadItemProperties(m_active_item);

	if (m_active_item && m_active_item->diagram()) {
		m_active_item->diagram()->clearSelection();
		m_active_item->setSelected(true);
		m_active_item->update();
	}
}

void DimensionPropertiesDialog::onAutoToggled(bool checked)
{
	Q_UNUSED(checked)
	if (m_updating) return;
	updateValueDisplay();
}

void DimensionPropertiesDialog::onPrecisionChanged()
{
	if (m_updating) return;
	updateValueDisplay();
}

void DimensionPropertiesDialog::onCommaToggled()
{
	if (m_updating) return;
	updateValueDisplay();
}

void DimensionPropertiesDialog::onApplyClicked()
{
	apply();
}

void DimensionPropertiesDialog::updateValueDisplay()
{
	if (!m_active_item) return;

	if (ui->m_auto_cb->isChecked()) {
		int prec = ui->m_precision_cb->currentIndex();
		if (prec < 0) prec = 1;
		QString valStr = QString::number(m_active_item->dimensionValueMM(), 'f', prec);
		if (ui->m_comma_cb->isChecked()) {
			valStr.replace(QLatin1Char('.'), QLatin1Char(','));
		}
		if (ui->m_suppress_zero_cb->isChecked() && prec > 0) {
			QString zeroEnding = ui->m_comma_cb->isChecked() ? QStringLiteral(",0") : QStringLiteral(".0");
			if (valStr.endsWith(zeroEnding)) {
				valStr.chop(2);
			}
		}
		ui->m_value_le->setText(valStr);
		ui->m_value_le->setEnabled(false);
	} else {
		ui->m_value_le->setEnabled(true);
	}
}

void DimensionPropertiesDialog::loadItemProperties(DimensionItem *item)
{
	if (!item) return;

	m_updating = true;
	const DimensionProperties &p = m_current_props[item];

	ui->m_prefix_cb->setEditText(p.prefix);
	ui->m_auto_cb->setChecked(p.isAutoValue);
	if (!p.isAutoValue) {
		ui->m_value_le->setText(p.customText);
		ui->m_value_le->setEnabled(true);
	}
	ui->m_suffix_cb->setEditText(p.suffix);

	ui->m_text_dist_sb->setValue(p.textDistance);
	ui->m_center_cb->setChecked(p.textCenter);
	int uIdx = ui->m_unit_cb->findText(p.unit);
	ui->m_unit_cb->setCurrentIndex(uIdx >= 0 ? uIdx : 0);
	ui->m_show_unit_cb->setChecked(p.showUnit);
	ui->m_dual_unit_cb->setChecked(p.showDualUnit);
	ui->m_precision_cb->setCurrentIndex(p.precision >= 0 && p.precision <= 3 ? p.precision : 1);
	ui->m_suppress_zero_cb->setChecked(p.suppressZero);
	ui->m_fraction_cb->setChecked(p.fractionFormat);
	ui->m_text_horiz_cb->setChecked(p.isTextHorizontal);
	ui->m_mid_line_cb->setChecked(p.isTextInLine);

	ui->m_line_width_sb->setValue(p.lineWidth);
	ui->m_overshoot_sb->setValue(p.extensionOvershoot);
	ui->m_gap_sb->setValue(p.extensionGap);
	ui->m_offset_sb->setValue(p.offset);
	ui->m_terminations_cb->setCurrentIndex(p.terminationStyle == 1 ? 1 : 0);
	ui->m_arrow_size_sb->setValue(p.arrowSize);

	ui->m_framed_cb->setChecked(p.isFramed);
	ui->m_comma_cb->setChecked(p.useComma);
	ui->m_mask_cb->setChecked(p.maskBackground);
	ui->m_font_size_sb->setValue(p.fontSize);

	m_updating = false;
	updateValueDisplay();
}

void DimensionPropertiesDialog::saveItemProperties(DimensionItem *item)
{
	if (!item) return;

	DimensionProperties p = m_current_props.value(item);
	p.prefix = ui->m_prefix_cb->currentText();
	p.isAutoValue = ui->m_auto_cb->isChecked();
	p.customText = ui->m_value_le->text();
	p.suffix = ui->m_suffix_cb->currentText();
	p.textDistance = ui->m_text_dist_sb->value();
	p.textCenter = ui->m_center_cb->isChecked();
	p.unit = ui->m_unit_cb->currentText();
	p.showUnit = ui->m_show_unit_cb->isChecked();
	p.showDualUnit = ui->m_dual_unit_cb->isChecked();
	p.precision = ui->m_precision_cb->currentIndex();
	p.suppressZero = ui->m_suppress_zero_cb->isChecked();
	p.fractionFormat = ui->m_fraction_cb->isChecked();
	p.isTextHorizontal = ui->m_text_horiz_cb->isChecked();
	p.isTextInLine = ui->m_mid_line_cb->isChecked();

	p.lineWidth = ui->m_line_width_sb->value();
	p.extensionOvershoot = ui->m_overshoot_sb->value();
	p.extensionGap = ui->m_gap_sb->value();
	p.offset = ui->m_offset_sb->value();
	p.terminationStyle = ui->m_terminations_cb->currentIndex();
	p.arrowSize = ui->m_arrow_size_sb->value();

	p.isFramed = ui->m_framed_cb->isChecked();
	p.useComma = ui->m_comma_cb->isChecked();
	p.maskBackground = ui->m_mask_cb->isChecked();
	p.fontSize = ui->m_font_size_sb->value();

	m_current_props[item] = p;
}

void DimensionPropertiesDialog::apply()
{
	if (m_active_item) {
		saveItemProperties(m_active_item);
	}

	for (DimensionItem *it : m_segments) {
		if (it && m_current_props.contains(it)) {
			it->setProperties(m_current_props[it]);
		}
	}

	if (m_primary_item && m_primary_item->diagram()) {
		m_primary_item->diagram()->update();
	}
	m_applied = true;
}

void DimensionPropertiesDialog::accept()
{
	if (m_active_item) {
		saveItemProperties(m_active_item);
	}

	apply();

	Diagram *diagram = m_primary_item ? m_primary_item->diagram() : nullptr;
	if (diagram) {
		QUndoCommand *parentCmd = new QUndoCommand(tr("Edit dimension properties"));
		bool hasChange = false;

		for (DimensionItem *it : m_segments) {
			if (it && m_original_props.contains(it) && m_current_props.contains(it)) {
				if (m_original_props[it] != m_current_props[it]) {
					new ChangeDimensionPropertiesCommand(it, m_original_props[it], m_current_props[it], parentCmd);
					hasChange = true;
				}
			}
		}

		if (hasChange) {
			diagram->undoStack().push(parentCmd);
		} else {
			delete parentCmd;
		}
	}

	QDialog::accept();
}

void DimensionPropertiesDialog::reject()
{
	if (m_applied) {
		for (DimensionItem *it : m_segments) {
			if (it && m_original_props.contains(it)) {
				it->setProperties(m_original_props[it]);
			}
		}
		if (m_primary_item && m_primary_item->diagram()) {
			m_primary_item->diagram()->update();
		}
	}
	QDialog::reject();
}

