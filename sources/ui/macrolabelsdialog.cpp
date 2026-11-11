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
#include "macrolabelsdialog.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

MacroLabelsDialog::MacroLabelsDialog(const QList<MacroLabels::Field> &fields, QWidget *parent) :
	QDialog(parent),
	m_fields(fields),
	m_table(new QTableWidget(int(fields.size()), 3, this))
{
	setWindowTitle(tr("Place the macro"));
	resize(520, 380);
	auto *layout = new QVBoxLayout(this);
	auto *help = new QLabel(tr("Enter the labels and texts of this macro. "
							   "Accept All takes the labels proposed."), this);
	help->setWordWrap(true);
	layout->addWidget(help);

	m_table->setObjectName(QStringLiteral("macroLabels"));
	m_table->setHorizontalHeaderLabels({tr("Kind"), tr("In the macro"), tr("To place")});
	m_table->verticalHeader()->setVisible(false);
	m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	for (int row = 0 ; row < fields.size() ; ++row) {
		const auto &field = fields.at(row);
		auto *kind = new QTableWidgetItem(field.kind == MacroLabels::Field::Label ? tr("Label") : tr("Text"));
		auto *saved = new QTableWidgetItem(field.saved);
		kind->setFlags(Qt::ItemIsEnabled);
		saved->setFlags(Qt::ItemIsEnabled);
		m_table->setItem(row, 0, kind);
		m_table->setItem(row, 1, saved);
		m_table->setItem(row, 2, new QTableWidgetItem(field.value));
	}
	layout->addWidget(m_table);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	auto *accept_all = buttons->addButton(tr("Accept All"), QDialogButtonBox::ActionRole);
	accept_all->setObjectName(QStringLiteral("macroAcceptAll"));
	connect(accept_all, &QPushButton::clicked, this, &MacroLabelsDialog::acceptAll);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);
}

/**
	@return the fields as edited
*/
QList<MacroLabels::Field> MacroLabelsDialog::fields() const
{
	QList<MacroLabels::Field> list = m_fields;
	for (int row = 0 ; row < list.size() ; ++row)
		list[row].value = m_table->item(row, 2)->text().trimmed();
	return list;
}

void MacroLabelsDialog::acceptAll()
{
	for (int row = 0 ; row < m_fields.size() ; ++row)
		m_table->item(row, 2)->setText(m_fields.at(row).value);
	accept();
}
