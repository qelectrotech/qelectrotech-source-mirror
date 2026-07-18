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
#include "contactgroupselectiondialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QItemSelectionModel>

ContactGroupSelectionDialog::ContactGroupSelectionDialog(
	const QVector<ElementData::SlaveContactGroup> &groups,
	const QSet<int> &usedGroupIndices,
	const ElementData &slaveData,
	QWidget *parent)
	: QDialog(parent)
{
	m_used_indices = usedGroupIndices;
	setWindowTitle(tr("Sélectionner un groupe de contacts"));

	auto *main_layout = new QVBoxLayout(this);

	auto *info_label = new QLabel(
		tr("Sélectionnez le groupe de contacts à assigner à cet élément esclave :"));
	main_layout->addWidget(info_label);

	// Determine max terminal count for dynamic columns
	int max_terminals = 0;
	for (const auto &g : groups) {
		if (g.terminalCount > max_terminals)
			max_terminals = g.terminalCount;
	}

	// Build column headers: #, Type, Sous-type, Contacts, Bornes, T1, T2, ...
	QStringList headers;
	headers << tr("#")
			<< tr("Type")
			<< tr("Sous-type")
			<< tr("Contacts")
			<< tr("Bornes");
	for (int t = 0; t < max_terminals; ++t) {
		headers << tr("T%1").arg(t + 1);
	}

	m_table = new QTableWidget(groups.size(), headers.size(), this);
	m_table->setHorizontalHeaderLabels(headers);
	m_table->setSelectionBehavior(QTableWidget::SelectRows);
	m_table->setSelectionMode(QTableWidget::SingleSelection);
	m_table->setEditTriggers(QTableWidget::NoEditTriggers);
	m_table->verticalHeader()->setVisible(false);
	m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setStretchLastSection(false);
	m_table->verticalHeader()->setDefaultSectionSize(24);

	// Populate table rows
	for (int row = 0; row < groups.size(); ++row) {
		const auto &g = groups.at(row);

		auto *num_item = new QTableWidgetItem(QString::number(row + 1));
		num_item->setTextAlignment(Qt::AlignCenter);
		m_table->setItem(row, 0, num_item);

		m_table->setItem(row, 1, new QTableWidgetItem(typeToString(g.type)));
		m_table->setItem(row, 2, new QTableWidgetItem(subtypeToString(g.subtype)));

		auto *ctc_item = new QTableWidgetItem(QString::number(g.contactCount));
		ctc_item->setTextAlignment(Qt::AlignCenter);
		m_table->setItem(row, 3, ctc_item);

		auto *term_item = new QTableWidgetItem(QString::number(g.terminalCount));
		term_item->setTextAlignment(Qt::AlignCenter);
		m_table->setItem(row, 4, term_item);

		// Fill T1..TN label columns
		for (int t = 0; t < max_terminals; ++t) {
			int col = 5 + t;
			QString label;
			if (t < g.labels.size()) {
				label = g.labels.at(t);
			} else if (t < g.terminalCount) {
				label = tr("T%1").arg(t + 1);
			}
			auto *label_item = new QTableWidgetItem(label);
			label_item->setTextAlignment(Qt::AlignCenter);
			m_table->setItem(row, col, label_item);
		}
	}

	// Mark used group rows as disabled (grayed out)
	QFont disabled_font;
	disabled_font.setStrikeOut(true);
	QColor disabled_color(Qt::gray);

	for (int row = 0; row < groups.size(); ++row) {
		bool disabled = false;
		QString reason;

		if (m_used_indices.contains(row)) {
			disabled = true;
			reason = tr("(déjà assigné)");
		} else {
			const auto &g = groups.at(row);
			if (g.type != slaveData.m_slave_state) {
				disabled = true;
				reason = tr("(état ne correspond pas)");
			} else if (g.subtype != slaveData.m_slave_type) {
				disabled = true;
				reason = tr("(sous-type ne correspond pas)");
			} else if (g.contactCount != slaveData.m_contact_count) {
				disabled = true;
				reason = tr("(nombre de contacts ne correspond pas)");
			}
		}

		if (disabled) {
			m_disabled_rows.insert(row);
			for (int col = 0; col < m_table->columnCount(); ++col) {
				auto *item = m_table->item(row, col);
				if (item) {
					item->setForeground(disabled_color);
					item->setFont(disabled_font);
					item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
					item->setToolTip(reason);
				}
			}
			// Show circled "?" as a widget in the first column
			auto *num_item = m_table->item(row, 0);
			if (num_item) {
				num_item->setToolTip(reason);
				auto *question_label = new QLabel(this);
				question_label->setText("?");
				question_label->setAlignment(Qt::AlignCenter);
				question_label->setStyleSheet(
					"QLabel {"
					"  color: #1a73e8;"
					"  font-weight: bold;"
					"  font-size: 8px;"
					"  border: 1.5px solid #1a73e8;"
					"  border-radius: 7px;"
					"  min-width: 12px; max-width: 12px;"
					"  min-height: 12px; max-height: 12px;"
					"}");
				question_label->setToolTip(reason);
				m_table->setCellWidget(row, 0, question_label);
			}
		}
	}

	// Select first available (non-disabled) row
	int first_available = 0;
	for (int row = 0; row < groups.size(); ++row) {
		if (!m_disabled_rows.contains(row)) {
			first_available = row;
			break;
		}
	}
	m_table->selectRow(first_available);
	main_layout->addWidget(m_table);

	// Calculate width based on column count: base + ~60px per column
	int table_width = 50 + headers.size() * 65;
	int table_height = 60 + groups.size() * 26;
	m_table->setMinimumWidth(table_width);
	m_table->setMinimumHeight(table_height);

	// Buttons
	auto *button_layout = new QHBoxLayout();
	button_layout->addStretch();

	m_ok_button = new QPushButton(tr("OK"), this);
	button_layout->addWidget(m_ok_button);

	auto *cancel_button = new QPushButton(tr("Annuler"), this);
	button_layout->addWidget(cancel_button);

	main_layout->addLayout(button_layout);

	// Connections
	connect(m_ok_button, &QPushButton::clicked, this, [this]() {
		auto *item = m_table->currentItem();
		if (item && !m_disabled_rows.contains(item->row())) {
			m_selected_index = item->row();
			accept();
		}
	});
	connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);
	connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
		if (!m_disabled_rows.contains(row)) {
			m_selected_index = row;
			accept();
		}
	});
}

int ContactGroupSelectionDialog::selectedIndex() const
{
	return m_selected_index;
}

QString ContactGroupSelectionDialog::typeToString(ElementData::SlaveState type)
{
	switch (type) {
		case ElementData::NO:    return tr("Normalement ouvert");
		case ElementData::NC:    return tr("Normalement fermé");
		case ElementData::SW:    return tr("Inverseur");
		case ElementData::Other: return tr("Autre");
		default: return tr("Inconnu");
	}
}

QString ContactGroupSelectionDialog::subtypeToString(ElementData::SlaveType subtype)
{
	switch (subtype) {
		case ElementData::SSimple:    return tr("Simple");
		case ElementData::Power:      return tr("Puissance");
		case ElementData::DelayOn:    return tr("Temporisé travail");
		case ElementData::DelayOff:   return tr("Temporisé repos");
		case ElementData::delayOnOff: return tr("Temporisé travail & repos");
		default: return tr("Inconnu");
	}
}
