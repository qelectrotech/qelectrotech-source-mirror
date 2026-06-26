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

#include <QTabWidget>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>

WireSpecDialog::WireSpecDialog(QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Wire / cable"));
	resize(560, 540);

	// Colourful header strip.
	auto *header = new QLabel(tr("Wire / cable reference"), this);
	header->setStyleSheet(QStringLiteral(
		"QLabel { background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
		" stop:0 #0066cc, stop:1 #00a651); color: white; font-weight: bold;"
		" padding: 6px 10px; border-radius: 4px; }"));

	auto *tabs = new QTabWidget(this);
	tabs->addTab(buildGeneralTab(), tr("General"));
	tabs->addTab(buildCoresTab(),   tr("Cable cores"));

	auto *buttons = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(header);
	layout->addWidget(tabs, 1);
	layout->addWidget(buttons);
}

QWidget *WireSpecDialog::buildGeneralTab()
{
	auto *tab = new QWidget(this);
	auto *outer = new QVBoxLayout(tab);

	// --- Identification ---
	m_wire_id      = new QLineEdit(tab);
	m_family       = new QLineEdit(tab);
	m_family->setPlaceholderText(tr("e.g. CAN_Cable, H07V-K"));
	m_manufacturer = new QLineEdit(tab);
	m_mfr_part_no  = new QLineEdit(tab);

	auto *id_box = new QGroupBox(tr("Identification"), tab);
	auto *id_form = new QFormLayout(id_box);
	id_form->addRow(tr("Wire ID *"),     m_wire_id);
	id_form->addRow(tr("Family / type"), m_family);
	id_form->addRow(tr("Manufacturer"),  m_manufacturer);
	id_form->addRow(tr("Mfr part no."),  m_mfr_part_no);

	// --- Supplier ---
	m_supplier         = new QLineEdit(tab);
	m_supplier_part_no = new QLineEdit(tab);

	auto *sup_box = new QGroupBox(tr("Supplier"), tab);
	auto *sup_form = new QFormLayout(sup_box);
	sup_form->addRow(tr("Supplier name"), m_supplier);
	sup_form->addRow(tr("Stock number"),  m_supplier_part_no);

	// --- Characteristics ---
	m_cross_section = new QDoubleSpinBox(tab);
	m_cross_section->setRange(0.0, 1000.0);
	m_cross_section->setDecimals(2);
	m_cross_section->setSuffix(QStringLiteral(" mm²"));

	m_outer_dia = new QDoubleSpinBox(tab);
	m_outer_dia->setRange(0.0, 1000.0);
	m_outer_dia->setDecimals(2);
	m_outer_dia->setSuffix(QStringLiteral(" mm"));

	m_insulation_dia = new QDoubleSpinBox(tab);
	m_insulation_dia->setRange(0.0, 1000.0);
	m_insulation_dia->setDecimals(2);
	m_insulation_dia->setSuffix(QStringLiteral(" mm"));

	m_color_primary = new WireColorComboBox(tab);

	m_shield = new QCheckBox(tr("Shielded"), tab);
	m_shield_type = new QComboBox(tab);
	m_shield_type->addItems({tr("Braid"), tr("Foil"), tr("Both")});
	m_shield_type->setEnabled(false);
	connect(m_shield, &QCheckBox::toggled, m_shield_type, &QWidget::setEnabled);

	m_voltage = new QSpinBox(tab);
	m_voltage->setRange(0, 100000);
	m_voltage->setSuffix(QStringLiteral(" V"));

	m_temp = new QSpinBox(tab);
	m_temp->setRange(-100, 1000);
	m_temp->setSuffix(QStringLiteral(" °C"));

	m_flexible = new QCheckBox(tr("Flexible"), tab);

	auto *ch_box = new QGroupBox(tr("Characteristics"), tab);
	auto *ch_form = new QFormLayout(ch_box);
	ch_form->addRow(tr("Cross-sectional area"), m_cross_section);
	ch_form->addRow(tr("Cable outer diameter"), m_outer_dia);
	ch_form->addRow(tr("Insulation diameter"),  m_insulation_dia);
	ch_form->addRow(tr("Primary colour"),       m_color_primary);
	ch_form->addRow(m_shield);
	ch_form->addRow(tr("Shield type"),          m_shield_type);
	ch_form->addRow(tr("Voltage rating"),       m_voltage);
	ch_form->addRow(tr("Temperature rating"),   m_temp);
	ch_form->addRow(m_flexible);

	// --- Notes ---
	m_notes = new QPlainTextEdit(tab);
	m_notes->setMaximumHeight(60);
	auto *notes_box = new QGroupBox(tr("Notes"), tab);
	auto *notes_lay = new QVBoxLayout(notes_box);
	notes_lay->addWidget(m_notes);

	outer->addWidget(id_box);
	outer->addWidget(sup_box);
	outer->addWidget(ch_box);
	outer->addWidget(notes_box);
	outer->addStretch(1);
	return tab;
}

QWidget *WireSpecDialog::buildCoresTab()
{
	auto *tab = new QWidget(this);
	auto *layout = new QVBoxLayout(tab);

	// Toolbar: Add / Remove core + live count.
	m_cores = new CoreColorEditor(tab);

	auto *bar = new QHBoxLayout;
	auto *add_btn = new QPushButton(tr("Add core"), tab);
	auto *del_btn = new QPushButton(tr("Remove core"), tab);
	m_core_count_lbl = new QLabel(tab);
	connect(add_btn, &QPushButton::clicked, m_cores, &CoreColorEditor::addCore);
	connect(del_btn, &QPushButton::clicked, m_cores, &CoreColorEditor::removeSelectedCore);
	connect(m_cores, &CoreColorEditor::coresChanged, this, &WireSpecDialog::updateCoreCountLabel);
	bar->addWidget(add_btn);
	bar->addWidget(del_btn);
	bar->addStretch(1);
	bar->addWidget(m_core_count_lbl);

	// Cores list in a scroll area so many cores stay usable.
	auto *scroll = new QScrollArea(tab);
	scroll->setWidgetResizable(true);
	scroll->setWidget(m_cores);

	auto *hint = new QLabel(
		tr("Each core: Colour 1 = base, Colours 2–3 = optional tracers."), tab);
	hint->setStyleSheet(QStringLiteral("color: #666;"));

	layout->addLayout(bar);
	layout->addWidget(scroll, 1);
	layout->addWidget(hint);
	updateCoreCountLabel();
	return tab;
}

void WireSpecDialog::updateCoreCountLabel()
{
	if (m_core_count_lbl && m_cores)
		m_core_count_lbl->setText(tr("Cores: %1").arg(m_cores->coreCount()));
}

void WireSpecDialog::setWireSpec(const WireSpec &spec, bool editing)
{
	m_wire_id->setText(spec.wireId);
	m_wire_id->setReadOnly(editing); // wire_id is the primary key
	m_family->setText(spec.familyName);
	m_manufacturer->setText(spec.manufacturerName);
	m_mfr_part_no->setText(spec.manufacturerPartNo);
	m_supplier->setText(spec.supplierName);
	m_supplier_part_no->setText(spec.supplierPartNo);
	m_cross_section->setValue(spec.crossSectionMm2);
	m_outer_dia->setValue(spec.outerDiaMm);
	m_insulation_dia->setValue(spec.insulationDiaMm);
	m_color_primary->setColorName(spec.colorPrimary);
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
	m_notes->setPlainText(spec.notes);

	// Cores: fall back to numCores empty cores if none stored yet.
	QVector<QStringList> cores = spec.coreColors;
	if (cores.isEmpty())
		cores = QVector<QStringList>(qMax(1, spec.numCores));
	m_cores->setCores(cores, spec.coreSections);
	updateCoreCountLabel();

	setWindowTitle(editing ? tr("Edit wire / cable") : tr("New wire / cable"));
}

WireSpec WireSpecDialog::wireSpec() const
{
	WireSpec s;
	s.wireId             = m_wire_id->text().trimmed();
	s.familyName         = m_family->text().trimmed();
	s.manufacturerName   = m_manufacturer->text().trimmed();
	s.manufacturerPartNo = m_mfr_part_no->text().trimmed();
	s.supplierName       = m_supplier->text().trimmed();
	s.supplierPartNo     = m_supplier_part_no->text().trimmed();
	s.crossSectionMm2    = m_cross_section->value();
	s.outerDiaMm         = m_outer_dia->value();
	s.insulationDiaMm    = m_insulation_dia->value();
	s.coreColors         = m_cores->colors();
	s.coreSections       = m_cores->sections();
	s.numCores           = m_cores->coreCount();
	s.hasShield          = m_shield->isChecked();
	s.shieldType         = s.hasShield ? m_shield_type->currentText() : QString();
	s.voltageRatingV     = m_voltage->value();
	s.tempRatingC        = m_temp->value();
	s.isFlexible         = m_flexible->isChecked();
	s.colorPrimary       = m_color_primary->colorName();
	s.notes              = m_notes->toPlainText().trimmed();
	return s;
}
