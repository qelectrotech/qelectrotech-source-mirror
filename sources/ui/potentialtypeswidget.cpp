// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "potentialtypeswidget.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
	enum Column {NameColumn, ColorColumn, StyleColumn, WidthColumn};

	void paintColorButton(QPushButton *button, const QColor &color)
	{
		QPixmap swatch(40, 14);
		swatch.fill(color);
		button->setIcon(QIcon(swatch));
		button->setIconSize(swatch.size());
		button->setProperty("color", color);
	}
}

PotentialTypesWidget::PotentialTypesWidget(QWidget *parent) :
	QWidget(parent)
{
	m_enabled_cb = new QCheckBox(tr("Draw conductors from their potential type"), this);
	m_enabled_cb->setToolTip(tr("When on, a conductor whose potential has a type is drawn in "
								"that type's colour, line style and width. The style stored "
								"on the conductor is kept, and used again when this is off "
								"or the conductor has no type."));

	QLabel *help = new QLabel(tr("Choose a conductor's type in its properties. The type applies "
								 "to every conductor on the same potential."), this);
	help->setWordWrap(true);

	m_table = new QTableWidget(0, 4, this);
	m_table->setHorizontalHeaderLabels({tr("Name"), tr("Colour"), tr("Line style"), tr("Width")});
	m_table->horizontalHeader()->setSectionResizeMode(NameColumn, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(StyleColumn, QHeaderView::ResizeToContents);
	m_table->verticalHeader()->hide();
		//Tall enough for the default list, no taller: the buttons below
		//must stay in view, not be pushed down by the empty rows
	m_table->setMinimumHeight(190);
	m_table->setMaximumHeight(300);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::SingleSelection);

	QPushButton *add_pb = new QPushButton(tr("Add"), this);
	QPushButton *remove_pb = new QPushButton(tr("Remove"), this);
	QPushButton *reset_pb = new QPushButton(tr("Restore the default types"), this);
	QHBoxLayout *buttons = new QHBoxLayout;
	buttons->addWidget(add_pb);
	buttons->addWidget(remove_pb);
	buttons->addStretch();
	buttons->addWidget(reset_pb);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(m_enabled_cb);
	layout->addWidget(help);
	layout->addWidget(m_table);
	layout->addLayout(buttons);
	layout->addStretch(1);

	connect(add_pb, &QPushButton::clicked, this, [this]() {
		PotentialType type;
		type.name = tr("New type");
		addRow(type);
		m_table->setCurrentCell(m_table->rowCount() - 1, NameColumn);
		m_table->editItem(m_table->item(m_table->rowCount() - 1, NameColumn));
	});
	connect(remove_pb, &QPushButton::clicked, this, [this]() {
		const int row = m_table->currentRow();
		if (row >= 0)
			m_table->removeRow(row);
	});
	connect(reset_pb, &QPushButton::clicked, this, [this]() {
		setTypes(PotentialTypes::defaultTypes());
	});
}

void PotentialTypesWidget::setPotentialTypes(const PotentialTypes &types)
{
	m_enabled_cb->setChecked(types.isEnabled());
	setTypes(types.types());
}

void PotentialTypesWidget::setTypes(const QList<PotentialType> &types)
{
	m_table->setRowCount(0);
	for (const PotentialType &type : types)
		addRow(type);
}

void PotentialTypesWidget::addRow(const PotentialType &type)
{
	const int row = m_table->rowCount();
	m_table->insertRow(row);
	m_table->setItem(row, NameColumn, new QTableWidgetItem(type.name));

	QPushButton *color_pb = new QPushButton(m_table);
	paintColorButton(color_pb, type.color);
	connect(color_pb, &QPushButton::clicked, this, [this, color_pb]() {chooseColor(color_pb);});
	m_table->setCellWidget(row, ColorColumn, color_pb);

	QComboBox *style_cb = new QComboBox(m_table);
	style_cb->addItem(tr("Solid line", "conductor style: solid line"), static_cast<int>(Qt::SolidLine));
	style_cb->addItem(tr("Dashed line", "conductor style: dashed line"), static_cast<int>(Qt::DashLine));
	style_cb->addItem(tr("Dots and dashes", "conductor style: dashed and dotted line"), static_cast<int>(Qt::DashDotLine));
	style_cb->addItem(tr("Dotted line", "conductor style: dotted line"), static_cast<int>(Qt::DotLine));
	const int style_index = style_cb->findData(static_cast<int>(type.style));
	style_cb->setCurrentIndex(style_index == -1 ? 0 : style_index);
	m_table->setCellWidget(row, StyleColumn, style_cb);

	QDoubleSpinBox *width_sb = new QDoubleSpinBox(m_table);
	width_sb->setRange(0.1, 10.0);
	width_sb->setSingleStep(0.5);
	width_sb->setDecimals(1);
	width_sb->setValue(type.width);
	m_table->setCellWidget(row, WidthColumn, width_sb);
}

void PotentialTypesWidget::chooseColor(QWidget *button)
{
	QPushButton *color_pb = static_cast<QPushButton *>(button);
	const QColor color = QColorDialog::getColor(color_pb->property("color").value<QColor>(),
												this, tr("Choose a colour"));
	if (color.isValid())
		paintColorButton(color_pb, color);
}

/**
	@return the types as edited. A row without a name is left out, and so
	is a second row of a name already listed: a conductor names its type,
	so two of the same name could not both be told apart.
*/
PotentialTypes PotentialTypesWidget::potentialTypes() const
{
	QList<PotentialType> types;
	for (int row = 0; row < m_table->rowCount(); ++row) {
		PotentialType type;
		type.name = m_table->item(row, NameColumn)->text().trimmed();
		if (type.name.isEmpty())
			continue;
		bool duplicate = false;
		for (const PotentialType &other : types)
			duplicate = duplicate || other.name == type.name;
		if (duplicate)
			continue;
		type.color = m_table->cellWidget(row, ColorColumn)->property("color").value<QColor>();
		type.style = static_cast<Qt::PenStyle>(
					static_cast<QComboBox *>(m_table->cellWidget(row, StyleColumn))
					->currentData().toInt());
		type.width = static_cast<QDoubleSpinBox *>(m_table->cellWidget(row, WidthColumn))->value();
		types << type;
	}

	PotentialTypes result;
	result.setEnabled(m_enabled_cb->isChecked());
	result.setTypes(types);
	return result;
}
