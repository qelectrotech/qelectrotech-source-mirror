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
#include "spacemouseconfigpage.h"

#include "../../qeticons.h"
#include "../../shortcutmanager.h"
#include "../../spacemouse/spacemousebuttonmap.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>

namespace {
	constexpr int COL_BUTTON = 0;
	constexpr int COL_ACTION = 1;
	constexpr int COL_REMOVE = 2;

		//The combo box's first entry: no action bound, i.e. this row does
		//nothing. Real action ids are never empty (ShortcutManager assigns
		//them from source, not user input), so an empty string can never
		//collide with a real one.
	const QString UNBOUND_LABEL = QObject::tr("(Aucune action)", "spacemouse config: unbound button");
}

/**
	@brief SpaceMouseConfigPage::SpaceMouseConfigPage
	@param parent
*/
SpaceMouseConfigPage::SpaceMouseConfigPage(QWidget *parent) :
	ConfigPage(parent)
{
	auto *vlayout = new QVBoxLayout();

	QLabel *title_label = new QLabel(this->title());
	vlayout->addWidget(title_label);

	QFrame *horiz_line = new QFrame();
	horiz_line->setFrameShape(QFrame::HLine);
	vlayout->addWidget(horiz_line);

	auto *intro_label = new QLabel(
		tr("Associez un numéro de bouton de votre souris 3D (SpaceMouse, SpacePilot...) "
		   "à une action de QElectroTech. Le numéro de bouton dépend de votre appareil "
		   "et de son pilote -- reportez-vous à sa documentation, ou essayez successivement "
		   "les valeurs à partir de 0.",
		   "spacemouse config page intro"));
	intro_label->setWordWrap(true);
	vlayout->addWidget(intro_label);

	m_table = new QTableWidget(0, 3, this);
	m_table->setHorizontalHeaderLabels({tr("N° bouton"), tr("Action"), QString()});
	m_table->horizontalHeader()->setSectionResizeMode(COL_BUTTON, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(COL_ACTION, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(COL_REMOVE, QHeaderView::ResizeToContents);
	m_table->verticalHeader()->setVisible(false);
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table->setSelectionMode(QAbstractItemView::NoSelection);
	vlayout->addWidget(m_table);

	auto *add_button = new QPushButton(QET::Icons::Add, tr("Ajouter une association"), this);
	connect(add_button, &QPushButton::clicked, this, &SpaceMouseConfigPage::addRow);

	auto *bottom_layout = new QHBoxLayout();
	bottom_layout->addWidget(add_button);
	bottom_layout->addStretch();
	vlayout->addLayout(bottom_layout);

	setLayout(vlayout);

	populateTable();
}

SpaceMouseConfigPage::~SpaceMouseConfigPage()
{
}

/**
	@brief SpaceMouseConfigPage::populateTable
	Fill the table with one row per binding currently persisted in
	SpaceMouseButtonMap.
*/
void SpaceMouseConfigPage::populateTable()
{
	const QMap<int, QString> bindings = SpaceMouseButtonMap::allBindings();
	for (auto it = bindings.constBegin(); it != bindings.constEnd(); ++it) {
		appendRow(it.key(), it.value());
	}
}

/**
	@brief SpaceMouseConfigPage::appendRow
	Add one row bound to \a button / \a action_id (either or both may be
	the "unset" value -- 0 and an empty string respectively -- for a fresh
	row added via the "+" button).
	@param button
	@param action_id
*/
void SpaceMouseConfigPage::appendRow(int button, const QString &action_id)
{
	const int row = m_table->rowCount();
	m_table->setRowCount(row + 1);

	auto *button_spin = new QSpinBox(m_table);
	button_spin->setRange(0, 999);
	button_spin->setValue(button);
	m_table->setCellWidget(row, COL_BUTTON, button_spin);

	auto *action_combo = new QComboBox(m_table);
	action_combo->addItem(UNBOUND_LABEL, QString());

	QList<ShortcutManager::ShortcutInfo> shortcuts = ShortcutManager::instance().allShortcuts();
	std::sort(shortcuts.begin(), shortcuts.end(),
		  [](const ShortcutManager::ShortcutInfo &a, const ShortcutManager::ShortcutInfo &b) {
			if (a.category != b.category) {
				return a.category < b.category;
			}
			return a.description < b.description;
		  });
	for (const ShortcutManager::ShortcutInfo &info : shortcuts) {
		action_combo->addItem(
			QStringLiteral("%1 — %2").arg(info.category, info.description), info.id);
	}

	int index_for_current = action_combo->findData(action_id);
	action_combo->setCurrentIndex(index_for_current >= 0 ? index_for_current : 0);
	m_table->setCellWidget(row, COL_ACTION, action_combo);

	auto *remove_button = new QToolButton(m_table);
	remove_button->setIcon(QET::Icons::EditTableDeleteRow);
	remove_button->setToolTip(tr("Supprimer cette association"));
	connect(remove_button, &QToolButton::clicked, this, &SpaceMouseConfigPage::removeSelectedRow);
	m_table->setCellWidget(row, COL_REMOVE, remove_button);
}

/**
	@brief SpaceMouseConfigPage::addRow
	Add a fresh, unbound row for the user to fill in.
*/
void SpaceMouseConfigPage::addRow()
{
	appendRow(0, QString());
}

/**
	@brief SpaceMouseConfigPage::removeSelectedRow
	Remove whichever row owns the remove button that was clicked. Found by
	looking the sender up in the button column rather than tracked
	per-button state, so nothing has to be kept in sync as rows are added
	and removed around it.
*/
void SpaceMouseConfigPage::removeSelectedRow()
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
	@brief SpaceMouseConfigPage::applyConf
	Persist exactly what the table currently shows: every previously saved
	binding is cleared first, then every row still in the table (after any
	adds/edits/removals) is written back. Simpler and less error-prone than
	tracking which individual rows changed, and cheap -- this runs once,
	when the user validates the surrounding ConfigDialog, not on every
	keystroke.
*/
void SpaceMouseConfigPage::applyConf()
{
	const QMap<int, QString> previous = SpaceMouseButtonMap::allBindings();
	for (auto it = previous.constBegin(); it != previous.constEnd(); ++it) {
		SpaceMouseButtonMap::setActionId(it.key(), QString());
	}

	for (int row = 0; row < m_table->rowCount(); ++row)
	{
		auto *button_spin = qobject_cast<QSpinBox *>(m_table->cellWidget(row, COL_BUTTON));
		auto *action_combo = qobject_cast<QComboBox *>(m_table->cellWidget(row, COL_ACTION));
		if (!button_spin || !action_combo) {
			continue;
		}
		SpaceMouseButtonMap::setActionId(button_spin->value(), action_combo->currentData().toString());
	}
}

QString SpaceMouseConfigPage::title() const
{
	return tr("Souris 3D", "configuration page title");
}

QIcon SpaceMouseConfigPage::icon() const
{
	return QET::Icons::ConfigureToolbars;
}
