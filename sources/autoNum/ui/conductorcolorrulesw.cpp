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
#include "conductorcolorrulesw.h"

#include "../../conductorcolorrule.h"
#include "../../qeticons.h"
#include "../../qetproject.h"

#include <kcolorbutton.h>

#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
	constexpr int COL_TEXT    = 0;
	constexpr int COL_COLOR   = 1;
	constexpr int COL_BICOLOR = 2;
	constexpr int COL_COLOR_2 = 3;
	constexpr int COL_REMOVE  = 4;
}

/**
	@brief ConductorColorRulesW::ConductorColorRulesW
	@param project
	@param parent
*/
ConductorColorRulesW::ConductorColorRulesW(QETProject *project, QWidget *parent) :
	QWidget(parent),
	m_project(project)
{
	auto *vlayout = new QVBoxLayout();

	auto *intro_label = new QLabel(
		tr("Quand la numérotation automatique des conducteurs résout une étiquette identique au texte "
		   "d'une association ci-dessous, la couleur du conducteur -- et de tous les conducteurs du "
		   "même potentiel -- est appliquée automatiquement.",
		   "conductor color rules page intro"));
	intro_label->setWordWrap(true);
	vlayout->addWidget(intro_label);

	m_table = new QTableWidget(0, 5, this);
	m_table->setHorizontalHeaderLabels({
		tr("Texte", "column title"),
		tr("Couleur", "column title"),
		tr("Bicolore", "column title"),
		tr("Couleur 2", "column title"),
		QString()
	});
	m_table->horizontalHeader()->setSectionResizeMode(COL_TEXT, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(COL_COLOR, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(COL_BICOLOR, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(COL_COLOR_2, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(COL_REMOVE, QHeaderView::ResizeToContents);
	m_table->verticalHeader()->setVisible(false);
	m_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
	m_table->setSelectionMode(QAbstractItemView::NoSelection);
	vlayout->addWidget(m_table);

	auto *add_button = new QPushButton(QET::Icons::Add, tr("Ajouter une association"), this);
	connect(add_button, &QPushButton::clicked, this, &ConductorColorRulesW::addRow);

	auto *save_button = new QPushButton(tr("Enregistrer"), this);
	connect(save_button, &QPushButton::clicked, this, &ConductorColorRulesW::save);

	auto *bottom_layout = new QHBoxLayout();
	bottom_layout->addWidget(add_button);
	bottom_layout->addStretch();
	bottom_layout->addWidget(save_button);
	vlayout->addLayout(bottom_layout);

	setLayout(vlayout);

	populateTable();
}

/**
	@brief ConductorColorRulesW::populateTable
	Fill the table with one row per rule currently persisted in the
	project's conductorColorRules() map.
*/
void ConductorColorRulesW::populateTable()
{
	const QHash<QString, ConductorColorRule> rules = m_project->conductorColorRules();
	for (auto it = rules.constBegin(); it != rules.constEnd(); ++it) {
		appendRow(it.key(), it.value().color, it.value().m_bicolor, it.value().color_2);
	}
}

/**
	@brief ConductorColorRulesW::appendRow
	@param text
	@param color
	@param bicolor
	@param color_2
*/
void ConductorColorRulesW::appendRow(const QString &text, const QColor &color, bool bicolor, const QColor &color_2)
{
	const int row = m_table->rowCount();
	m_table->setRowCount(row + 1);

	auto *text_edit = new QLineEdit(text, m_table);
	m_table->setCellWidget(row, COL_TEXT, text_edit);

	auto *color_button = new KColorButton(m_table);
	color_button->setColor(color);
	m_table->setCellWidget(row, COL_COLOR, color_button);

	auto *color_2_button = new KColorButton(m_table);
	color_2_button->setColor(color_2);
	color_2_button->setEnabled(bicolor);
	m_table->setCellWidget(row, COL_COLOR_2, color_2_button);

	auto *bicolor_check = new QCheckBox(m_table);
	bicolor_check->setChecked(bicolor);
	connect(bicolor_check, &QCheckBox::toggled, color_2_button, &QWidget::setEnabled);
	auto *bicolor_cell = new QWidget(m_table);
	auto *bicolor_layout = new QHBoxLayout(bicolor_cell);
	bicolor_layout->addWidget(bicolor_check);
	bicolor_layout->setAlignment(Qt::AlignCenter);
	bicolor_layout->setContentsMargins(0, 0, 0, 0);
	m_table->setCellWidget(row, COL_BICOLOR, bicolor_cell);

	auto *remove_button = new QToolButton(m_table);
	remove_button->setIcon(QET::Icons::EditTableDeleteRow);
	remove_button->setToolTip(tr("Supprimer cette association"));
	connect(remove_button, &QToolButton::clicked, this, &ConductorColorRulesW::removeRow);
	m_table->setCellWidget(row, COL_REMOVE, remove_button);
}

/**
	@brief ConductorColorRulesW::addRow
	Add a fresh, blank row for the user to fill in.
*/
void ConductorColorRulesW::addRow()
{
	appendRow(QString(), QColor(Qt::black), false, QColor(Qt::black));
}

/**
	@brief ConductorColorRulesW::removeRow
	Remove whichever row owns the remove button that was clicked. Found by
	looking the sender up in the remove-button column rather than tracked
	per-button state, so nothing has to be kept in sync as rows are added
	and removed around it -- same approach as SpaceMouseConfigPage's own
	row removal.
*/
void ConductorColorRulesW::removeRow()
{
	auto *button = qobject_cast<QToolButton *>(sender());
	if (!button) {
		return;
	}
	for (int row = 0; row < m_table->rowCount(); ++row) {
		if (m_table->cellWidget(row, COL_REMOVE) == button) {
			m_table->removeRow(row);
			return;
		}
	}
}

/**
	@brief ConductorColorRulesW::save
	Persist exactly what the table currently shows to the project. A row
	with empty text is skipped -- an unnamed rule can never match a
	resolved label, so keeping it would be silent dead weight rather than
	a rule the user actually meant to configure.
*/
void ConductorColorRulesW::save()
{
	QHash<QString, ConductorColorRule> rules;

	for (int row = 0; row < m_table->rowCount(); ++row)
	{
		auto *text_edit    = qobject_cast<QLineEdit *>(m_table->cellWidget(row, COL_TEXT));
		auto *color_button = qobject_cast<KColorButton *>(m_table->cellWidget(row, COL_COLOR));
		auto *bicolor_cell = m_table->cellWidget(row, COL_BICOLOR);
		auto *bicolor_check = bicolor_cell ? bicolor_cell->findChild<QCheckBox *>() : nullptr;
		auto *color_2_button = qobject_cast<KColorButton *>(m_table->cellWidget(row, COL_COLOR_2));
		if (!text_edit || !color_button || !bicolor_check || !color_2_button) {
			continue;
		}

		const QString text = text_edit->text().trimmed();
		if (text.isEmpty()) {
			continue;
		}

		rules.insert(text, ConductorColorRule(
					color_button->color(),
					bicolor_check->isChecked(),
					color_2_button->color()));
	}

	m_project->setConductorColorRules(rules);
	m_project->setModified(true);
}
