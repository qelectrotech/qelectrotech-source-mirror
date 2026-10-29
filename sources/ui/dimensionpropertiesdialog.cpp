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

	ui->m_text_rotation_cb->addItem(tr("Auto (parallel to line)"), -1);
	ui->m_text_rotation_cb->addItem(QStringLiteral("0° (") + tr("Horizontal") + QStringLiteral(")"), 0);
	ui->m_text_rotation_cb->addItem(QStringLiteral("90°"), 90);
	ui->m_text_rotation_cb->addItem(QStringLiteral("180°"), 180);
	ui->m_text_rotation_cb->addItem(QStringLiteral("270°"), 270);

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

	if (m_segments.size() <= 1) {
		ui->m_select_all_cb->setVisible(false);
	}

	connect(ui->m_select_all_cb, &QCheckBox::clicked,
	        this, &DimensionPropertiesDialog::onSelectAllToggled);
	connect(ui->m_segments_list, &QListWidget::itemSelectionChanged,
	        this, &DimensionPropertiesDialog::onSelectionChanged);
	connect(ui->m_segments_list, &QListWidget::currentRowChanged,
	        this, [this](int row) {
		if (m_updating_selection) return;
		if (row >= 0 && row < m_segments.size()) {
			DimensionItem *newActive = m_segments.at(row);
			if (newActive != m_active_item) {
				m_active_item = newActive;
				loadItemProperties(m_active_item);
			}
		}
	});

	setupPropertyConnections();

	connect(ui->m_buttonBox, &QDialogButtonBox::accepted, this, &DimensionPropertiesDialog::accept);
	connect(ui->m_buttonBox, &QDialogButtonBox::rejected, this, &DimensionPropertiesDialog::reject);

	QPushButton *applyBtn = ui->m_buttonBox->button(QDialogButtonBox::Apply);
	if (applyBtn) {
		connect(applyBtn, &QPushButton::clicked, this, &DimensionPropertiesDialog::onApplyClicked);
	}

	int initialIdx = m_segments.indexOf(item);
	int startRow = initialIdx >= 0 ? initialIdx : 0;
	if (startRow >= 0 && startRow < ui->m_segments_list->count()) {
		ui->m_segments_list->setCurrentRow(startRow);
		ui->m_segments_list->item(startRow)->setSelected(true);
	}
}

DimensionPropertiesDialog::~DimensionPropertiesDialog()
{
	delete ui;
}

QList<DimensionItem *> DimensionPropertiesDialog::selectedSegments() const
{
	QList<DimensionItem *> list;
	for (QListWidgetItem *item : ui->m_segments_list->selectedItems()) {
		int row = ui->m_segments_list->row(item);
		if (row >= 0 && row < m_segments.size()) {
			list.append(m_segments.at(row));
		}
	}
	if (list.isEmpty() && m_active_item) {
		list.append(m_active_item);
	}
	return list;
}

void DimensionPropertiesDialog::onSelectAllToggled(bool checked)
{
	if (m_updating_selection) return;
	m_updating_selection = true;

	if (checked) {
		ui->m_segments_list->selectAll();
	} else {
		ui->m_segments_list->clearSelection();
		if (m_active_item) {
			int idx = m_segments.indexOf(m_active_item);
			if (idx >= 0 && idx < ui->m_segments_list->count()) {
				ui->m_segments_list->item(idx)->setSelected(true);
			}
		}
	}

	m_updating_selection = false;
	onSelectionChanged();
}

void DimensionPropertiesDialog::onSelectionChanged()
{
	if (m_updating_selection) return;
	m_updating_selection = true;

	auto selected = ui->m_segments_list->selectedItems();
	if (selected.size() == m_segments.size() && !m_segments.isEmpty()) {
		ui->m_select_all_cb->setCheckState(Qt::Checked);
	} else if (selected.isEmpty()) {
		ui->m_select_all_cb->setCheckState(Qt::Unchecked);
	} else {
		ui->m_select_all_cb->setCheckState(Qt::PartiallyChecked);
	}

	int currentRow = ui->m_segments_list->currentRow();
	if (currentRow >= 0 && currentRow < m_segments.size()) {
		DimensionItem *newActive = m_segments.at(currentRow);
		if (newActive != m_active_item) {
			m_active_item = newActive;
			loadItemProperties(m_active_item);
		}
	} else if (!selected.isEmpty()) {
		int firstRow = ui->m_segments_list->row(selected.first());
		if (firstRow >= 0 && firstRow < m_segments.size()) {
			m_active_item = m_segments.at(firstRow);
			loadItemProperties(m_active_item);
		}
	}

	// Highlight all selected segments in diagram
	if (!m_segments.isEmpty() && m_segments.first() && m_segments.first()->diagram()) {
		Diagram *diag = m_segments.first()->diagram();
		diag->clearSelection();
		for (DimensionItem *seg : selectedSegments()) {
			if (seg) seg->setSelected(true);
		}
		diag->update();
	}

	m_updating_selection = false;
}

void DimensionPropertiesDialog::setupPropertyConnections()
{
	// Prefix
	connect(ui->m_prefix_cb, &QComboBox::currentTextChanged, this, [this](const QString &text) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].prefix = text;
		}
	});

	// Auto value
	connect(ui->m_auto_cb, &QCheckBox::toggled, this, [this](bool checked) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].isAutoValue = checked;
		}
		updateValueDisplay();
	});

	// Custom text
	connect(ui->m_value_le, &QLineEdit::textEdited, this, [this](const QString &text) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].customText = text;
		}
	});

	// Suffix
	connect(ui->m_suffix_cb, &QComboBox::currentTextChanged, this, [this](const QString &text) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].suffix = text;
		}
	});

	// Text distance
	connect(ui->m_text_dist_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].textDistance = val;
		}
	});

	// Text center
	connect(ui->m_center_cb, &QCheckBox::toggled, this, [this](bool checked) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].textCenter = checked;
		}
	});

	// Unit
	connect(ui->m_unit_cb, &QComboBox::currentTextChanged, this, [this](const QString &text) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].unit = text;
		}
	});

	// Show unit
	connect(ui->m_show_unit_cb, &QCheckBox::toggled, this, [this](bool checked) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].showUnit = checked;
		}
	});

	// Dual unit
	connect(ui->m_dual_unit_cb, &QCheckBox::toggled, this, [this](bool checked) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].showDualUnit = checked;
		}
	});

	// Precision
	connect(ui->m_precision_cb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].precision = idx;
		}
		updateValueDisplay();
	});

	// Suppress zero
	connect(ui->m_suppress_zero_cb, &QCheckBox::toggled, this, [this](bool checked) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].suppressZero = checked;
		}
		updateValueDisplay();
	});

	// Text rotation
	connect(ui->m_text_rotation_cb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
		if (m_updating) return;
		int rot = ui->m_text_rotation_cb->itemData(idx).toInt();
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].textRotation = rot;
			m_current_props[seg].isTextHorizontal = (rot == 0);
		}
	});

	// Text in line (mid line)
	connect(ui->m_mid_line_cb, &QCheckBox::toggled, this, [this](bool checked) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].isTextInLine = checked;
		}
	});

	// Line width
	connect(ui->m_line_width_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].lineWidth = val;
		}
	});

	// Extension overshoot
	connect(ui->m_overshoot_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].extensionOvershoot = val;
		}
	});

	// Extension gap
	connect(ui->m_gap_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].extensionGap = val;
		}
	});

	// Offset
	connect(ui->m_offset_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].offset = val;
		}
	});

	// Termination style
	connect(ui->m_terminations_cb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].terminationStyle = idx;
		}
	});

	// Arrow size
	connect(ui->m_arrow_size_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].arrowSize = val;
		}
	});

	// Framed
	connect(ui->m_framed_cb, &QCheckBox::toggled, this, [this](bool checked) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].isFramed = checked;
		}
	});

	// Comma
	connect(ui->m_comma_cb, &QCheckBox::toggled, this, [this](bool checked) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].useComma = checked;
		}
		updateValueDisplay();
	});

	// Mask background
	connect(ui->m_mask_cb, &QCheckBox::toggled, this, [this](bool checked) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].maskBackground = checked;
		}
	});

	// Font size
	connect(ui->m_font_size_sb, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
		if (m_updating) return;
		for (DimensionItem *seg : selectedSegments()) {
			m_current_props[seg].fontSize = val;
		}
	});
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
	int rotIdx = ui->m_text_rotation_cb->findData(p.textRotation);
	if (rotIdx < 0) {
		if (p.isTextHorizontal || p.textRotation == 0) {
			rotIdx = ui->m_text_rotation_cb->findData(0);
		} else {
			rotIdx = 0; // Default to Automatisch (-1)
		}
	}
	ui->m_text_rotation_cb->setCurrentIndex(rotIdx >= 0 ? rotIdx : 0);
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

void DimensionPropertiesDialog::apply()
{
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
