/*
	Copyright 2026 Trovo Tech Solutions
	This file is part of a custom feature set built on QElectroTech.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "wirespecdialog.h"
#include "corecoloreditor.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <QLabel>
#include <QScrollArea>

WireSpecDialog::WireSpecDialog(QWidget *parent) :
	QDialog(parent)
{
	buildUi();
}

void WireSpecDialog::buildUi()
{
	setWindowTitle(tr("Wire / cable"));

	m_wire_id          = new QLineEdit(this);
	m_manufacturer     = new QLineEdit(this);
	m_mfr_part_no      = new QLineEdit(this);
	m_supplier         = new QLineEdit(this);
	m_supplier_part_no = new QLineEdit(this);

	m_cross_section = new QDoubleSpinBox(this);
	m_cross_section->setRange(0.0, 1000.0);
	m_cross_section->setDecimals(2);
	m_cross_section->setSuffix(QStringLiteral(" mm²"));

	m_outer_dia = new QDoubleSpinBox(this);
	m_outer_dia->setRange(0.0, 1000.0);
	m_outer_dia->setDecimals(2);
	m_outer_dia->setSuffix(QStringLiteral(" mm"));

	m_insulation_dia = new QDoubleSpinBox(this);
	m_insulation_dia->setRange(0.0, 1000.0);
	m_insulation_dia->setDecimals(2);
	m_insulation_dia->setSuffix(QStringLiteral(" mm"));

	m_num_cores = new QSpinBox(this);
	m_num_cores->setRange(1, 256);

	// Multi-core cable builder: one colour picker per core, kept in sync with
	// the core count. Capped height with a scroll area for high core counts.
	m_core_colors = new CoreColorEditor(this);
	auto *core_scroll = new QScrollArea(this);
	core_scroll->setWidgetResizable(true);
	core_scroll->setFrameShape(QFrame::NoFrame);
	core_scroll->setWidget(m_core_colors);
	core_scroll->setMinimumHeight(40);
	core_scroll->setMaximumHeight(150);
	connect(m_num_cores, QOverload<int>::of(&QSpinBox::valueChanged),
			m_core_colors, &CoreColorEditor::setCoreCount);

	m_shield = new QCheckBox(tr("Shielded"), this);
	m_shield_type = new QComboBox(this);
	m_shield_type->addItems({tr("Braid"), tr("Foil"), tr("Both")});

	m_voltage = new QSpinBox(this);
	m_voltage->setRange(0, 100000);
	m_voltage->setSuffix(QStringLiteral(" V"));

	m_temp = new QSpinBox(this);
	m_temp->setRange(-100, 1000);
	m_temp->setSuffix(QStringLiteral(" °C"));

	m_flexible = new QCheckBox(tr("Flexible"), this);

	m_color_primary = new WireColorComboBox(this);

	m_notes = new QPlainTextEdit(this);
	m_notes->setMaximumHeight(60);

	auto *form = new QFormLayout;
	form->addRow(tr("Wire ID *"),          m_wire_id);
	form->addRow(tr("Manufacturer"),       m_manufacturer);
	form->addRow(tr("Mfr part no."),       m_mfr_part_no);
	form->addRow(tr("Supplier"),           m_supplier);
	form->addRow(tr("Supplier part no."),  m_supplier_part_no);
	form->addRow(tr("Cross-section"),      m_cross_section);
	form->addRow(tr("Outer diameter"),     m_outer_dia);
	form->addRow(tr("Insulation diam."),   m_insulation_dia);
	form->addRow(tr("Number of cores"),    m_num_cores);
	form->addRow(tr("Core colours"),       core_scroll);
	form->addRow(m_shield);
	form->addRow(tr("Shield type"),        m_shield_type);
	form->addRow(tr("Voltage rating"),     m_voltage);
	form->addRow(tr("Temp. rating"),       m_temp);
	form->addRow(m_flexible);
	form->addRow(tr("Primary colour"),     m_color_primary);
	form->addRow(tr("Notes"),              m_notes);

	auto *buttons = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	// Shield type only enabled when shielded.
	connect(m_shield, &QCheckBox::toggled, m_shield_type, &QWidget::setEnabled);
	m_shield_type->setEnabled(false);

	// Colourful header strip.
	auto *header = new QLabel(tr("Wire / cable specification"), this);
	header->setStyleSheet(QStringLiteral(
		"QLabel { background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
		" stop:0 #0066cc, stop:1 #00a651); color: white; font-weight: bold;"
		" padding: 6px 10px; border-radius: 4px; }"));

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(header);
	layout->addLayout(form);
	layout->addWidget(buttons);
}

void WireSpecDialog::setWireSpec(const WireSpec &spec, bool editing)
{
	m_wire_id->setText(spec.wireId);
	m_wire_id->setReadOnly(editing); // wire_id is the primary key
	m_manufacturer->setText(spec.manufacturerName);
	m_mfr_part_no->setText(spec.manufacturerPartNo);
	m_supplier->setText(spec.supplierName);
	m_supplier_part_no->setText(spec.supplierPartNo);
	m_cross_section->setValue(spec.crossSectionMm2);
	m_outer_dia->setValue(spec.outerDiaMm);
	m_insulation_dia->setValue(spec.insulationDiaMm);
	m_num_cores->setValue(spec.numCores);
	m_core_colors->setColors(spec.coreColors);
	m_shield->setChecked(spec.hasShield);
	m_shield_type->setEnabled(spec.hasShield);
	if (!spec.shieldType.isEmpty()) {
		const int idx = m_shield_type->findText(spec.shieldType);
		if (idx >= 0)
			m_shield_type->setCurrentIndex(idx);
	}
	m_voltage->setValue(spec.voltageRatingV);
	m_temp->setValue(spec.tempRatingC);
	m_flexible->setChecked(spec.isFlexible);
	m_color_primary->setColorName(spec.colorPrimary);
	m_notes->setPlainText(spec.notes);

	setWindowTitle(editing ? tr("Edit wire / cable") : tr("New wire / cable"));
}

WireSpec WireSpecDialog::wireSpec() const
{
	WireSpec s;
	s.wireId             = m_wire_id->text().trimmed();
	s.manufacturerName   = m_manufacturer->text().trimmed();
	s.manufacturerPartNo = m_mfr_part_no->text().trimmed();
	s.supplierName       = m_supplier->text().trimmed();
	s.supplierPartNo     = m_supplier_part_no->text().trimmed();
	s.crossSectionMm2    = m_cross_section->value();
	s.outerDiaMm         = m_outer_dia->value();
	s.insulationDiaMm    = m_insulation_dia->value();
	s.numCores           = m_num_cores->value();
	s.coreColors         = m_core_colors->colors();

	s.hasShield      = m_shield->isChecked();
	s.shieldType     = s.hasShield ? m_shield_type->currentText() : QString();
	s.voltageRatingV = m_voltage->value();
	s.tempRatingC    = m_temp->value();
	s.isFlexible     = m_flexible->isChecked();
	s.colorPrimary   = m_color_primary->colorName();
	s.notes          = m_notes->toPlainText().trimmed();
	return s;
}
