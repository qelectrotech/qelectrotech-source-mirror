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
#include "shortcutbarconfigpage.h"

#include "../../ElementsCollection/elementslocation.h"
#include "../../qeticons.h"
#include "../../shortcutmanager.h"

#include <QAction>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

/**
	@brief ShortcutBarConfigPage::ShortcutBarConfigPage
	@param parent
*/
ShortcutBarConfigPage::ShortcutBarConfigPage(QWidget *parent) :
	ConfigPage(parent)
{
	const QStringList available = ShortcutBarSettings::availableIds();
	for (const ShortcutManager::ShortcutInfo &info :
	     ShortcutManager::instance().allShortcuts()) {
		if (available.contains(info.id)) {
			m_descriptions.insert(info.id, info.description);
		}
	}
	for (const ShortcutBarSettings::Context c : ShortcutBarSettings::contexts()) {
		m_pending.insert(c, ShortcutBarSettings::ids(c));
	}

	auto *explanation = new QLabel(
		tr("La barre de raccourcis s'ouvre à la position du curseur "
		   "(touche S par défaut). Elle montre les commandes choisies "
		   "ici selon ce qui est sélectionné, puis le sélecteur "
		   "d'éléments."), this);
	explanation->setWordWrap(true);

	m_context = new QComboBox(this);
	for (const ShortcutBarSettings::Context c : ShortcutBarSettings::contexts()) {
		m_context->addItem(ShortcutBarSettings::title(c), c);
	}

	m_available = new QListWidget(this);
	m_available->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_available->setSortingEnabled(true);
	m_chosen = new QListWidget(this);
	m_chosen->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_chosen->setDragDropMode(QAbstractItemView::InternalMove);

	auto *add = new QPushButton(tr("Ajouter →"), this);
	auto *remove = new QPushButton(tr("← Retirer"), this);
	auto *up = new QPushButton(tr("Monter"), this);
	auto *down = new QPushButton(tr("Descendre"), this);
	auto *reset = new QPushButton(tr("Valeurs par défaut"), this);

	auto *buttons = new QVBoxLayout();
	buttons->addStretch();
	buttons->addWidget(add);
	buttons->addWidget(remove);
	buttons->addSpacing(12);
	buttons->addWidget(up);
	buttons->addWidget(down);
	buttons->addStretch();

	auto *grid = new QGridLayout();
	grid->addWidget(new QLabel(tr("Commandes disponibles"), this), 0, 0);
	grid->addWidget(new QLabel(tr("Dans la barre, dans l'ordre"), this), 0, 2);
	grid->addWidget(m_available, 1, 0);
	grid->addLayout(buttons, 1, 1);
	grid->addWidget(m_chosen, 1, 2);

	auto *context_row = new QHBoxLayout();
	context_row->addWidget(new QLabel(tr("Contexte :"), this));
	context_row->addWidget(m_context, 1);
	context_row->addWidget(reset);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(explanation);
	layout->addLayout(context_row);
	layout->addLayout(grid);

	connect(m_context, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() {
		storeContext();
		showContext();
	});
	connect(add, &QPushButton::clicked, this, &ShortcutBarConfigPage::addSelected);
	connect(remove, &QPushButton::clicked, this, &ShortcutBarConfigPage::removeSelected);
	connect(up, &QPushButton::clicked, this, [this]() { moveSelected(-1); });
	connect(down, &QPushButton::clicked, this, [this]() { moveSelected(1); });
	connect(reset, &QPushButton::clicked, this, &ShortcutBarConfigPage::resetContext);
	connect(m_available, &QListWidget::itemDoubleClicked, this, &ShortcutBarConfigPage::addSelected);
	connect(m_chosen, &QListWidget::itemDoubleClicked, this, &ShortcutBarConfigPage::removeSelected);

	showContext();
}

/**
	@brief ShortcutBarConfigPage::applyConf
	Save every context's list.
*/
void ShortcutBarConfigPage::applyConf()
{
	storeContext();
	for (const ShortcutBarSettings::Context c : ShortcutBarSettings::contexts()) {
		ShortcutBarSettings::setIds(c, m_pending.value(c));
	}
}

QString ShortcutBarConfigPage::title() const
{
	return tr("Barre de raccourcis", "configuration page title");
}

QIcon ShortcutBarConfigPage::icon() const
{
	return QET::Icons::ConfigureShortcuts;
}

/**
	@brief ShortcutBarConfigPage::showContext
	Fill both lists for the context chosen in the combo box.
*/
void ShortcutBarConfigPage::showContext()
{
	m_shown = static_cast<ShortcutBarSettings::Context>(
		m_context->currentData().toInt());
	const QStringList chosen = m_pending.value(m_shown);

	m_available->clear();
	m_chosen->clear();
	for (const QString &id : chosen) {
		appendItem(m_chosen, id);
	}
	for (auto it = m_descriptions.cbegin(); it != m_descriptions.cend(); ++it) {
		if (!chosen.contains(it.key())) {
			appendItem(m_available, it.key());
		}
	}
}

/**
	@brief ShortcutBarConfigPage::storeContext
	Keep the shown context's list, in the order on screen.
*/
void ShortcutBarConfigPage::storeContext()
{
	QStringList ids;
	for (int i = 0 ; i < m_chosen->count() ; ++i) {
		ids << m_chosen->item(i)->data(Qt::UserRole).toString();
	}
	m_pending.insert(m_shown, ids);
}

void ShortcutBarConfigPage::addSelected()
{
	for (QListWidgetItem *item : m_available->selectedItems()) {
		m_chosen->addItem(m_available->takeItem(m_available->row(item)));
	}
}

void ShortcutBarConfigPage::removeSelected()
{
	for (QListWidgetItem *item : m_chosen->selectedItems()) {
		QListWidgetItem *taken = m_chosen->takeItem(m_chosen->row(item));
			//A removed element is not a command to list; it is pinned
			//again from the bar's own customising window
		if (ShortcutBarSettings::isElement(taken->data(Qt::UserRole).toString())) {
			delete taken;
		} else {
			m_available->addItem(taken);
		}
	}
}

/**
	@brief ShortcutBarConfigPage::moveSelected
	Move the selected command @a step rows, keeping it selected.
*/
void ShortcutBarConfigPage::moveSelected(int step)
{
	const int row = m_chosen->currentRow();
	const int target = row + step;
	if (row < 0 || target < 0 || target >= m_chosen->count()) {
		return;
	}
	QListWidgetItem *item = m_chosen->takeItem(row);
	m_chosen->insertItem(target, item);
	m_chosen->setCurrentRow(target);
}

void ShortcutBarConfigPage::resetContext()
{
	m_pending.insert(m_shown, ShortcutBarSettings::defaultIds(m_shown));
	showContext();
}

/**
	@brief ShortcutBarConfigPage::appendItem
	Add @a id to @a list with the command's text and icon, or a pinned
	element's name and icon. An id no live action carries (a command from a
	build without it) is still listed, by its id, so saving does not
	silently drop it.
*/
void ShortcutBarConfigPage::appendItem(QListWidget *list, const QString &id)
{
	QString text = m_descriptions.value(id, id);
	QIcon icon;
	if (ShortcutBarSettings::isElement(id)) {
		const ElementsLocation location(id);
		if (location.exist()) {
			text = location.name();
			icon = location.icon();
		}
	} else if (QAction *action = ShortcutManager::instance().action(id, nullptr)) {
		icon = action->icon();
	}
	auto *item = new QListWidgetItem(icon, text);
	item->setData(Qt::UserRole, id);
	list->addItem(item);
}
