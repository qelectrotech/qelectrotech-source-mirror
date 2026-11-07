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
#include <QListWidget>
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
	m_mode_cb = new QComboBox(this);
	m_mode_cb->addItem(tr("Draw conductors from their own style"), static_cast<int>(PotentialTypes::Mode::Off));
	m_mode_cb->addItem(tr("Draw conductors from their potential type, chosen on each conductor"),
					   static_cast<int>(PotentialTypes::Mode::PerWire));
	m_mode_cb->addItem(tr("Draw conductors from their potential type, set on the source terminal"),
					   static_cast<int>(PotentialTypes::Mode::AtSource));
	m_mode_cb->setToolTip(tr("When a type is used, a conductor whose potential has a type is "
							 "drawn in that type's colour, line style and width. The style "
							 "stored on the conductor is kept, and used again when this is "
							 "off or the conductor has no type."));

	QLabel *help = new QLabel(tr("Choose a conductor's type in its properties. The type applies "
								 "to every conductor on the same potential. With the source "
								 "terminal, set the type on a terminal of a symbol (the output of "
								 "a power supply, for example) in the symbol's "
								 "properties: it reaches the conductors through terminal blocks "
								 "and folio reports, not through other devices. A type chosen "
								 "on a conductor still wins."), this);
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

		//Pairs of types joined on purpose (0V and PE at a power supply):
		//a potential holding only such a pair is not reported by the check
	QLabel *bond_help = new QLabel(tr("Types joined on purpose, for example 0V and PE at a "
									  "power supply. A potential with only these types is "
									  "not reported as mixed."), this);
	bond_help->setWordWrap(true);
	m_bond_a_cb = new QComboBox(this);
	m_bond_b_cb = new QComboBox(this);
	QPushButton *bond_add_pb = new QPushButton(tr("Join"), this);
	QPushButton *bond_remove_pb = new QPushButton(tr("Remove"), this);
	QHBoxLayout *bond_row = new QHBoxLayout;
	bond_row->addWidget(m_bond_a_cb);
	bond_row->addWidget(new QLabel(tr("and"), this));
	bond_row->addWidget(m_bond_b_cb);
	bond_row->addWidget(bond_add_pb);
	bond_row->addStretch();
	bond_row->addWidget(bond_remove_pb);
	m_bond_list = new QListWidget(this);
	m_bond_list->setMaximumHeight(80);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(m_mode_cb);
	layout->addWidget(help);
	layout->addWidget(m_table);
	layout->addLayout(buttons);
	layout->addWidget(bond_help);
	layout->addLayout(bond_row);
	layout->addWidget(m_bond_list);
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
	connect(m_table->model(), &QAbstractItemModel::rowsInserted, this, &PotentialTypesWidget::refreshBondChoices);
	connect(m_table->model(), &QAbstractItemModel::rowsRemoved, this, &PotentialTypesWidget::refreshBondChoices);
	connect(m_table->model(), &QAbstractItemModel::dataChanged, this, &PotentialTypesWidget::refreshBondChoices);
	connect(bond_add_pb, &QPushButton::clicked, this, [this]() {
		const PotentialBond bond{m_bond_a_cb->currentText(), m_bond_b_cb->currentText()};
		if (bond.a.isEmpty() || bond.b.isEmpty() || bond.a == bond.b)
			return;
		for (int i = 0; i < m_bond_list->count(); ++i) {
			const QStringList pair = m_bond_list->item(i)->data(Qt::UserRole).toStringList();
			if (bond == PotentialBond{pair.value(0), pair.value(1)})
				return;
		}
		addBondItem(bond);
	});
	connect(bond_remove_pb, &QPushButton::clicked, this, [this]() {
		delete m_bond_list->takeItem(m_bond_list->currentRow());
	});
}

void PotentialTypesWidget::setPotentialTypes(const PotentialTypes &types)
{
		//Every mode is offered (one this QET does not know reads as Off)
	m_mode_cb->setCurrentIndex(qMax(0, m_mode_cb->findData(static_cast<int>(types.mode()))));
	setTypes(types.types());
	setBonds(types.bonds());
}

void PotentialTypesWidget::setBonds(const QList<PotentialBond> &bonds)
{
	m_bond_list->clear();
	for (const PotentialBond &bond : bonds)
		addBondItem(bond);
}

void PotentialTypesWidget::addBondItem(const PotentialBond &bond)
{
	QListWidgetItem *item = new QListWidgetItem(tr("%1 and %2").arg(bond.a, bond.b), m_bond_list);
	item->setData(Qt::UserRole, QStringList{bond.a, bond.b});
}

	//The two lists offer the names in the table as they stand now
void PotentialTypesWidget::refreshBondChoices()
{
	QStringList names;
	for (int row = 0; row < m_table->rowCount(); ++row) {
		const QTableWidgetItem *item = m_table->item(row, NameColumn);
		if (item && !item->text().trimmed().isEmpty())
			names << item->text().trimmed();
	}
	for (QComboBox *cb : {m_bond_a_cb, m_bond_b_cb}) {
		const QString current = cb->currentText();
		cb->clear();
		cb->addItems(names);
		cb->setCurrentText(current);
	}
	if (m_bond_b_cb->count() > 1 && m_bond_a_cb->currentText() == m_bond_b_cb->currentText())
		m_bond_b_cb->setCurrentIndex((m_bond_b_cb->currentIndex() + 1) % m_bond_b_cb->count());
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
	result.setMode(static_cast<PotentialTypes::Mode>(m_mode_cb->currentData().toInt()));
	result.setTypes(types);
	QList<PotentialBond> bonds;
	for (int i = 0; i < m_bond_list->count(); ++i) {
		const QStringList pair = m_bond_list->item(i)->data(Qt::UserRole).toStringList();
		bonds << PotentialBond{pair.value(0), pair.value(1)};
	}
	result.setBonds(bonds);
	return result;
}
