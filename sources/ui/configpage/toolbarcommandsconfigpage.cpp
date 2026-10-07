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
#include "toolbarcommandsconfigpage.h"

#include "../../qeticons.h"
#include "../../shortcutmanager.h"

#include <QAction>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

ToolbarCommandsConfigPage::ToolbarCommandsConfigPage(QWidget *parent) :
	ConfigPage(parent)
{
	const QStringList available = DiagramToolbarSettings::availableCommandIds();
	for (const ShortcutManager::ShortcutInfo &info :
	     ShortcutManager::instance().allShortcuts()) {
		if (available.contains(info.id)) {
			m_descriptions.insert(info.id, info.description);
		}
	}
	m_custom = DiagramToolbarSettings::customToolbars();
	for (const DiagramToolbarSettings::Toolbar &toolbar : DiagramToolbarSettings::toolbars()) {
		m_pending.insert(toolbar.name, DiagramToolbarSettings::ids(toolbar.name));
	}

	auto *explanation = new QLabel(
		tr("Choose the commands of each toolbar of the diagram editor. Drag "
		   "a command from the list on the left to the bar, or double-click "
		   "it; Delete removes it."), this);
	explanation->setWordWrap(true);

	m_toolbar = new QComboBox(this);
	m_toolbar->setObjectName(QStringLiteral("toolbarCombo"));
	auto *new_toolbar = new QPushButton(tr("New bar…"), this);
	new_toolbar->setObjectName(QStringLiteral("newToolbarButton"));
	m_rename = new QPushButton(tr("Rename…"), this);
	m_delete = new QPushButton(tr("Delete the bar"), this);
	m_delete->setObjectName(QStringLiteral("deleteToolbarButton"));
	auto *reset = new QPushButton(tr("Defaults"), this);
	reset->setObjectName(QStringLiteral("resetButton"));

	m_filter = new QLineEdit(this);
	m_filter->setPlaceholderText(tr("Search for a command"));
	m_filter->setClearButtonEnabled(true);

	m_available = new QListWidget(this);
	m_available->setObjectName(QStringLiteral("availableList"));
	m_available->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_available->setSortingEnabled(true);
	m_available->setDragDropMode(QAbstractItemView::DragOnly);
	m_available->setDefaultDropAction(Qt::MoveAction);
	m_chosen = new QListWidget(this);
	m_chosen->setObjectName(QStringLiteral("chosenList"));
	m_chosen->setSelectionMode(QAbstractItemView::ExtendedSelection);
		//Takes commands dragged from the list on the left, and reorders
	m_chosen->setDragDropMode(QAbstractItemView::DragDrop);
	m_chosen->setDefaultDropAction(Qt::MoveAction);

	auto *add = new QPushButton(tr("Add →"), this);
	auto *separator = new QPushButton(tr("Separator"), this);
	separator->setObjectName(QStringLiteral("separatorButton"));
	auto *remove = new QPushButton(tr("← Remove"), this);
	auto *up = new QPushButton(tr("Move up"), this);
	auto *down = new QPushButton(tr("Move down"), this);

	auto *buttons = new QVBoxLayout();
	buttons->addStretch();
	buttons->addWidget(add);
	buttons->addWidget(separator);
	buttons->addWidget(remove);
	buttons->addSpacing(12);
	buttons->addWidget(up);
	buttons->addWidget(down);
	buttons->addStretch();

	auto *grid = new QGridLayout();
	grid->addWidget(new QLabel(tr("Available commands"), this), 0, 0);
	grid->addWidget(new QLabel(tr("In the bar, in order"), this), 0, 2);
	grid->addWidget(m_filter, 1, 0);
	grid->addWidget(m_available, 2, 0);
	grid->addLayout(buttons, 2, 1);
	grid->addWidget(m_chosen, 1, 2, 2, 1);

	auto *toolbar_row = new QHBoxLayout();
	toolbar_row->addWidget(new QLabel(tr("Toolbar:"), this));
	toolbar_row->addWidget(m_toolbar, 1);
	toolbar_row->addWidget(new_toolbar);
	toolbar_row->addWidget(m_rename);
	toolbar_row->addWidget(m_delete);
	toolbar_row->addWidget(reset);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(explanation);
	layout->addLayout(toolbar_row);
	layout->addLayout(grid);

	connect(m_toolbar, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() {
		storeToolbar();
		showToolbar();
	});
	connect(m_filter, &QLineEdit::textChanged, this, &ToolbarCommandsConfigPage::fillAvailable);
	connect(add, &QPushButton::clicked, this, &ToolbarCommandsConfigPage::addSelected);
	connect(separator, &QPushButton::clicked, this, &ToolbarCommandsConfigPage::addSeparator);
	connect(remove, &QPushButton::clicked, this, &ToolbarCommandsConfigPage::removeSelected);
	connect(up, &QPushButton::clicked, this, [this]() { moveSelected(-1); });
	connect(down, &QPushButton::clicked, this, [this]() { moveSelected(1); });
	connect(reset, &QPushButton::clicked, this, &ToolbarCommandsConfigPage::resetToolbar);
	connect(new_toolbar, &QPushButton::clicked, this, &ToolbarCommandsConfigPage::newToolbar);
	connect(m_rename, &QPushButton::clicked, this, &ToolbarCommandsConfigPage::renameToolbar);
	connect(m_delete, &QPushButton::clicked, this, &ToolbarCommandsConfigPage::deleteToolbar);
	connect(m_available, &QListWidget::itemDoubleClicked, this, &ToolbarCommandsConfigPage::addSelected);
	connect(m_chosen, &QListWidget::itemDoubleClicked, this, &ToolbarCommandsConfigPage::removeSelected);
		//A command dropped from the left is no longer available. Later,
		//not now: a dropped row is inserted before its data is set.
	connect(m_chosen->model(), &QAbstractItemModel::rowsInserted, this, [this]() {
		if (m_chosen->property("filling").toBool()) return;
		QTimer::singleShot(0, this, [this]() {
			storeToolbar();
			fillAvailable();
		});
	});

	auto *delete_key = new QAction(m_chosen);
	delete_key->setShortcut(QKeySequence::Delete);
	delete_key->setShortcutContext(Qt::WidgetShortcut);
	m_chosen->addAction(delete_key);
	connect(delete_key, &QAction::triggered, this, &ToolbarCommandsConfigPage::removeSelected);

	fillToolbarCombo(DiagramToolbarSettings::builtInNames().constFirst());
}

/**
	@brief ToolbarCommandsConfigPage::applyConf
	Save the user's own toolbars and every toolbar's contents, then
	rebuild the toolbars of the open diagram editors.
*/
void ToolbarCommandsConfigPage::applyConf()
{
	storeToolbar();
	DiagramToolbarSettings::setCustomToolbars(m_custom);
	for (const DiagramToolbarSettings::Toolbar &toolbar : DiagramToolbarSettings::toolbars()) {
		DiagramToolbarSettings::setIds(toolbar.name, m_pending.value(toolbar.name));
	}
	DiagramToolbarSettings::applyToAll();
}

QString ToolbarCommandsConfigPage::title() const
{
	return tr("Toolbar contents", "configuration page title: what each toolbar holds");
}

QIcon ToolbarCommandsConfigPage::icon() const
{
	return QET::Icons::ConfigureToolbars;
}

/**
	@brief ToolbarCommandsConfigPage::fillToolbarCombo
	List the built-in toolbars and the user's own, and show @a current.
*/
void ToolbarCommandsConfigPage::fillToolbarCombo(const QString &current)
{
	const QSignalBlocker blocker(m_toolbar);
	m_toolbar->clear();
	for (const QString &name : DiagramToolbarSettings::builtInNames()) {
		m_toolbar->addItem(DiagramToolbarSettings::builtInTitle(name), name);
	}
	for (const DiagramToolbarSettings::Toolbar &toolbar : std::as_const(m_custom)) {
		m_toolbar->addItem(toolbar.title, toolbar.name);
	}
	const int index = m_toolbar->findData(current);
	m_toolbar->setCurrentIndex(index < 0 ? 0 : index);
	showToolbar();
}

/**
	@brief ToolbarCommandsConfigPage::showToolbar
	Fill both lists for the toolbar chosen in the combo box.
*/
void ToolbarCommandsConfigPage::showToolbar()
{
	m_shown = m_toolbar->currentData().toString();
	const bool custom = !DiagramToolbarSettings::builtInNames().contains(m_shown);
	m_rename->setEnabled(custom);
	m_delete->setEnabled(custom);

	m_chosen->setProperty("filling", true);
	m_chosen->clear();
	for (const QString &id : m_pending.value(m_shown)) {
		m_chosen->addItem(makeItem(id));
	}
	m_chosen->setProperty("filling", false);
	fillAvailable();
}

/**
	@brief ToolbarCommandsConfigPage::storeToolbar
	Keep the shown toolbar's list, in the order on screen.
*/
void ToolbarCommandsConfigPage::storeToolbar()
{
	if (m_shown.isEmpty()) {
		return;
	}
	QStringList ids;
	for (int i = 0 ; i < m_chosen->count() ; ++i) {
		ids << m_chosen->item(i)->data(Qt::UserRole).toString();
	}
	m_pending.insert(m_shown, ids);
}

/**
	@brief ToolbarCommandsConfigPage::fillAvailable
	List what can still go on the shown toolbar and matches the search: a
	command not already on it, and a widget button on no toolbar at all
	(a widget can only be in one place).
*/
void ToolbarCommandsConfigPage::fillAvailable()
{
	QStringList on_shown;
	for (int i = 0 ; i < m_chosen->count() ; ++i) {
		on_shown << m_chosen->item(i)->data(Qt::UserRole).toString();
	}
	QStringList on_any = on_shown;
	for (auto it = m_pending.cbegin() ; it != m_pending.cend() ; ++it) {
		if (it.key() != m_shown) on_any << it.value();
	}

	QStringList ids;
	for (auto it = m_descriptions.cbegin() ; it != m_descriptions.cend() ; ++it) {
		if (!on_shown.contains(it.key())) ids << it.key();
	}
	for (const QString &id : DiagramToolbarSettings::widgetIds()) {
		if (!on_any.contains(id)) ids << id;
	}

	m_available->clear();
	const QString filter = m_filter->text().trimmed();
	for (const QString &id : std::as_const(ids)) {
		QListWidgetItem *item = makeItem(id);
		if (filter.isEmpty() || item->text().contains(filter, Qt::CaseInsensitive)) {
			m_available->addItem(item);
		} else {
			delete item;
		}
	}
}

void ToolbarCommandsConfigPage::addSelected()
{
	const QList<QListWidgetItem *> selected = m_available->selectedItems();
	if (selected.isEmpty()) {
		return;
	}
	m_chosen->setProperty("filling", true);
	for (QListWidgetItem *item : selected) {
		m_chosen->addItem(m_available->takeItem(m_available->row(item)));
	}
	m_chosen->setProperty("filling", false);
	storeToolbar();
	fillAvailable();
}

/**
	@brief ToolbarCommandsConfigPage::addSeparator
	Insert a separator after the selected row, or at the end.
*/
void ToolbarCommandsConfigPage::addSeparator()
{
	const int row = m_chosen->currentRow();
	m_chosen->setProperty("filling", true);
	m_chosen->insertItem(row < 0 ? m_chosen->count() : row + 1,
			     makeItem(DiagramToolbarSettings::separatorId()));
	m_chosen->setProperty("filling", false);
	storeToolbar();
}

void ToolbarCommandsConfigPage::removeSelected()
{
	for (QListWidgetItem *item : m_chosen->selectedItems()) {
		delete m_chosen->takeItem(m_chosen->row(item));
	}
	storeToolbar();
	fillAvailable();
}

/**
	@brief ToolbarCommandsConfigPage::moveSelected
	Move the selected command @a step rows, keeping it selected.
*/
void ToolbarCommandsConfigPage::moveSelected(int step)
{
	const int row = m_chosen->currentRow();
	const int target = row + step;
	if (row < 0 || target < 0 || target >= m_chosen->count()) {
		return;
	}
	m_chosen->setProperty("filling", true);
	QListWidgetItem *item = m_chosen->takeItem(row);
	m_chosen->insertItem(target, item);
	m_chosen->setProperty("filling", false);
	m_chosen->setCurrentRow(target);
	storeToolbar();
}

/**
	@brief ToolbarCommandsConfigPage::resetToolbar
	Give the shown toolbar its defaults back; the user's own toolbar is
	emptied.
*/
void ToolbarCommandsConfigPage::resetToolbar()
{
	m_pending.insert(m_shown, DiagramToolbarSettings::defaultIds(m_shown));
		//A widget the defaults put here leaves the toolbar it was moved to
	for (const QString &id : DiagramToolbarSettings::defaultIds(m_shown)) {
		if (!DiagramToolbarSettings::isWidget(id)) continue;
		for (auto it = m_pending.begin() ; it != m_pending.end() ; ++it) {
			if (it.key() != m_shown) it.value().removeAll(id);
		}
	}
	showToolbar();
}

void ToolbarCommandsConfigPage::newToolbar()
{
	bool ok = false;
	const QString title = QInputDialog::getText(this, tr("New toolbar"),
						    tr("Toolbar name:"),
						    QLineEdit::Normal, QString(), &ok).trimmed();
	if (!ok || title.isEmpty()) {
		return;
	}
	storeToolbar();
	DiagramToolbarSettings::Toolbar toolbar;
	toolbar.name = DiagramToolbarSettings::newCustomName(m_custom);
	toolbar.title = title;
	toolbar.custom = true;
	m_custom << toolbar;
	m_pending.insert(toolbar.name, QStringList());
	fillToolbarCombo(toolbar.name);
}

void ToolbarCommandsConfigPage::renameToolbar()
{
	for (DiagramToolbarSettings::Toolbar &toolbar : m_custom) {
		if (toolbar.name != m_shown) continue;
		bool ok = false;
		const QString title = QInputDialog::getText(this, tr("Rename the toolbar"),
							    tr("Toolbar name:"),
							    QLineEdit::Normal, toolbar.title, &ok).trimmed();
		if (ok && !title.isEmpty()) {
			storeToolbar();
			toolbar.title = title;
			fillToolbarCombo(m_shown);
		}
		return;
	}
}

void ToolbarCommandsConfigPage::deleteToolbar()
{
	const QString name = m_shown;
	for (int i = 0 ; i < m_custom.size() ; ++i) {
		if (m_custom.at(i).name == name) {
			m_custom.removeAt(i);
			m_pending.remove(name);
			m_shown.clear();
			fillToolbarCombo(DiagramToolbarSettings::builtInNames().constFirst());
			return;
		}
	}
}

/**
	@brief ToolbarCommandsConfigPage::makeItem
	@return a list item for @a id with the command's text and icon. An id
	no live action carries (a command from a build without it, or a script
	since removed) is still listed, by its id, so saving does not silently
	drop it.
*/
QListWidgetItem *ToolbarCommandsConfigPage::makeItem(const QString &id) const
{
	QString text = m_descriptions.value(id, id);
	QString tip;
	QIcon icon;
	if (id == DiagramToolbarSettings::separatorId()) {
		text = tr("─── Separator ───");
	} else if (DiagramToolbarSettings::isWidget(id)) {
		text = DiagramToolbarSettings::widgetTitle(id);
	} else if (QAction *action = ShortcutManager::instance().action(id, nullptr)) {
		icon = action->icon();
		tip = action->statusTip();
	}
	auto *item = new QListWidgetItem(icon, text);
	item->setData(Qt::UserRole, id);
		//Tells apart commands with the same name ("Ajouter une ligne")
	item->setToolTip(tip);
	return item;
}
