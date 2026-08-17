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
#include "shortcutsconfigpage.h"

#include "../../qeticons.h"
#include "../../shortcutmanager.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>
#include <utility>

namespace {
/// Key a shortcut for conflict detection: two actions collide only when they
/// share a sequence *within the same category*. See FINDINGS / upstream #757.
QString conflictKey(const QString &category, const QString &sequence)
{
	return category + QLatin1Char('\x1f') + sequence;
}
}

/**
	@brief ShortcutsConfigPage::ShortcutsConfigPage
	@param parent
*/
ShortcutsConfigPage::ShortcutsConfigPage(QWidget *parent) :
	ConfigPage(parent)
{
	auto *vlayout = new QVBoxLayout();

	QLabel *title_label = new QLabel(this->title());
	vlayout->addWidget(title_label);

	QFrame *horiz_line = new QFrame();
	horiz_line->setFrameShape(QFrame::HLine);
	vlayout->addWidget(horiz_line);

	m_filter_edit = new QLineEdit(this);
	m_filter_edit->setPlaceholderText(tr("Filtrer les raccourcis…"));
	connect(m_filter_edit, &QLineEdit::textChanged, this, &ShortcutsConfigPage::filterRows);
	vlayout->addWidget(m_filter_edit);

	m_table = new QTableWidget(0, 4, this);
	m_table->setHorizontalHeaderLabels({tr("Catégorie"), tr("Action"), tr("Raccourci"), QString()});
	m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	m_table->verticalHeader()->setVisible(false);
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table->setSelectionMode(QAbstractItemView::NoSelection);
	vlayout->addWidget(m_table);

	auto *reset_all_button = new QPushButton(tr("Tout réinitialiser"), this);
	connect(reset_all_button, &QPushButton::clicked, this, &ShortcutsConfigPage::resetAllRows);

	auto *bottom_layout = new QHBoxLayout();
	bottom_layout->addStretch();
	bottom_layout->addWidget(reset_all_button);
	vlayout->addLayout(bottom_layout);

	setLayout(vlayout);

	populateTable();
}

ShortcutsConfigPage::~ShortcutsConfigPage()
{
}

/**
	@brief ShortcutsConfigPage::populateTable
	Fill the table with one row per shortcut known to ShortcutManager, sorted
	by category then action name.
*/
void ShortcutsConfigPage::populateTable()
{
	QList<ShortcutManager::ShortcutInfo> shortcuts = ShortcutManager::instance().allShortcuts();
	std::sort(shortcuts.begin(), shortcuts.end(),
		  [](const ShortcutManager::ShortcutInfo &a, const ShortcutManager::ShortcutInfo &b) {
			if (a.category != b.category) {
				return a.category < b.category;
			}
			return a.description < b.description;
		  });

	m_table->setRowCount(shortcuts.size());
	m_rows.clear();
	m_rows.reserve(shortcuts.size());

	for (int row = 0; row < shortcuts.size(); ++row) {
		const ShortcutManager::ShortcutInfo &info = shortcuts.at(row);

		auto *category_item = new QTableWidgetItem(info.category);
		category_item->setFlags(category_item->flags() & ~Qt::ItemIsEditable);
		m_table->setItem(row, 0, category_item);

		auto *description_item = new QTableWidgetItem(info.description);
		description_item->setFlags(description_item->flags() & ~Qt::ItemIsEditable);
		m_table->setItem(row, 1, description_item);

		auto *edit = new QKeySequenceEdit(info.current_sequence, m_table);
		connect(edit, &QKeySequenceEdit::editingFinished, this, &ShortcutsConfigPage::checkConflicts);
		m_table->setCellWidget(row, 2, edit);

		auto *reset_button = new QToolButton(m_table);
		reset_button->setIcon(QET::Icons::EditUndo);
		reset_button->setToolTip(tr("Réinitialiser ce raccourci"));
		reset_button->setAutoRaise(true);
		connect(reset_button, &QToolButton::clicked, this, [this, row]() { resetRow(row); });
		m_table->setCellWidget(row, 3, reset_button);

		m_rows << Row{info.id, info.category, info.default_sequence, edit};
	}

	checkConflicts();
}

/**
	@brief ShortcutsConfigPage::filterRows
	Hide every row whose category or action name doesn't contain \a filter_text.
*/
void ShortcutsConfigPage::filterRows(const QString &filter_text)
{
	const QString needle = filter_text.trimmed();
	for (int row = 0; row < m_table->rowCount(); ++row) {
		const bool matches = needle.isEmpty()
				|| m_table->item(row, 0)->text().contains(needle, Qt::CaseInsensitive)
				|| m_table->item(row, 1)->text().contains(needle, Qt::CaseInsensitive);
		m_table->setRowHidden(row, !matches);
	}
}

/**
	@brief ShortcutsConfigPage::checkConflicts
	Highlight every row whose currently-edited sequence is shared, non-empty,
	with another row, and explain the conflict in the shortcut editor's tooltip.
*/
void ShortcutsConfigPage::checkConflicts()
{
	// Scope the comparison to the category, not the whole registry.
	// QElectroTech deliberately registers one key per editor window --
	// Undo/Redo/New/Open/Save and Ctrl+Shift+S each exist three times, once
	// for the diagram, element and titleblock editors -- and those are not
	// collisions: they act on different windows. Comparing globally flagged
	// 60 of the 95 shipped defaults as conflicts, which made the indicator
	// meaningless. The category is the registry's existing per-window
	// grouping, so it is the scope key.
	QHash<QString, QList<int>> sequence_to_rows;
	for (int row = 0; row < m_rows.size(); ++row) {
		const QString sequence_text = m_rows.at(row).edit->keySequence().toString();
		if (!sequence_text.isEmpty()) {
			sequence_to_rows[conflictKey(m_rows.at(row).category, sequence_text)] << row;
		}
	}

	for (int row = 0; row < m_rows.size(); ++row) {
		const Row &current_row = m_rows.at(row);
		const QString sequence_text = current_row.edit->keySequence().toString();
		const QList<int> &conflicting_rows =
				sequence_to_rows.value(conflictKey(current_row.category, sequence_text));
		const bool conflicted = !sequence_text.isEmpty() && conflicting_rows.size() > 1;

		QTableWidgetItem *description_item = m_table->item(row, 1);
		if (conflicted) {
			QStringList other_descriptions;
			for (int other_row : conflicting_rows) {
				if (other_row != row) {
					other_descriptions << m_table->item(other_row, 1)->text();
				}
			}
			description_item->setBackground(QColor(255, 205, 205));
			current_row.edit->setToolTip(
				tr("Ce raccourci est aussi utilisé par : %1").arg(other_descriptions.join(QStringLiteral(", "))));
		} else {
			description_item->setBackground(Qt::NoBrush);
			current_row.edit->setToolTip(QString());
		}
	}
}

/**
	@brief ShortcutsConfigPage::resetRow
	Reset the shortcut editor at \a row_index to its default sequence.
*/
void ShortcutsConfigPage::resetRow(int row_index)
{
	if (row_index < 0 || row_index >= m_rows.size()) {
		return;
	}
	m_rows.at(row_index).edit->setKeySequence(m_rows.at(row_index).default_sequence);
	checkConflicts();
}

void ShortcutsConfigPage::resetAllRows()
{
	for (const Row &row : std::as_const(m_rows)) {
		row.edit->setKeySequence(row.default_sequence);
	}
	checkConflicts();
}

/**
	@brief ShortcutsConfigPage::applyConf
	Persist every row's shortcut edit through ShortcutManager, which also
	applies it immediately to every currently live QAction sharing that id.
*/
void ShortcutsConfigPage::applyConf()
{
	for (const Row &row : std::as_const(m_rows)) {
		ShortcutManager::instance().setSequence(row.id, row.edit->keySequence());
	}
}

QString ShortcutsConfigPage::title() const
{
	return tr("Raccourcis", "configuration page title");
}

QIcon ShortcutsConfigPage::icon() const
{
	return QET::Icons::ConfigureToolbars;
}
