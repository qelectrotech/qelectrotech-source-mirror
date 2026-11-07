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
#include "potentialsourceswidget.h"

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../diagram.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetproject.h"

#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QPixmap>
#include <QTableWidget>
#include <QVBoxLayout>

PotentialSourcesWidget::PotentialSourcesWidget(Element *element, QWidget *parent) :
	AbstractElementPropertiesEditorWidget(parent)
{
	QLabel *help = new QLabel(tr("A type set on a terminal reaches every conductor at its "
								 "potential that has no type of its own, through terminal "
								 "blocks and folio reports."), this);
	help->setWordWrap(true);

	m_table = new QTableWidget(0, 2, this);
	m_table->setHorizontalHeaderLabels({tr("Terminal"), tr("Potential type")});
	m_table->verticalHeader()->hide();
	m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setStretchLastSection(true);
	m_table->setSelectionMode(QAbstractItemView::NoSelection);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(help);
	layout->addWidget(m_table);

	setElement(element);
}

bool PotentialSourcesWidget::isWanted(const Element *element)
{
	if (!element || element->terminals().isEmpty() || !element->diagram())
		return false;
	if (element->elementData().m_type & ElementData::AllReport)
		return false;
	const QETProject *project = element->diagram()->project();
	return project && project->potentialTypes().mode() == PotentialTypes::Mode::AtSource;
}

void PotentialSourcesWidget::setElement(Element *element)
{
	m_element = element;
	updateUi();
}

QString PotentialSourcesWidget::title() const
{
	return tr("Potential sources");
}

/**
	@brief PotentialSourcesWidget::updateUi
	One row per terminal of the symbol, showing the type it has. A type the
	project no longer lists is shown as it is, so opening and closing the
	properties does not lose it.
*/
void PotentialSourcesWidget::updateUi()
{
	m_table->setRowCount(0);
	m_combos.clear();
	m_terminals.clear();
	if (!m_element)
		return;

	PotentialTypes types;
	if (m_element->diagram() && m_element->diagram()->project())
		types = m_element->diagram()->project()->potentialTypes();
	const QHash<QUuid, QString> sources = m_element->potentialSources();

	const QList<Terminal *> terminals = m_element->terminals();
	m_table->setRowCount(static_cast<int>(terminals.size()));
	for (int row = 0 ; row < terminals.size() ; ++row) {
		const Terminal *terminal = terminals.at(row);
		const QString name = terminal->name().isEmpty()
				? tr("Terminal %1").arg(row + 1) : terminal->name();
		QTableWidgetItem *item = new QTableWidgetItem(name);
		item->setFlags(Qt::ItemIsEnabled);
		m_table->setItem(row, 0, item);

		QComboBox *combo = new QComboBox(m_table);
		combo->addItem(tr("None"), QString());
		for (const PotentialType &type : types.types()) {
			QPixmap swatch(24, 12);
			swatch.fill(type.color);
			combo->addItem(QIcon(swatch), type.name, type.name);
		}
		const QString current = sources.value(terminal->stableUuid());
		int index = combo->findData(current);
		if (index < 0) {
			combo->addItem(current, current);
			index = combo->count() - 1;
		}
		combo->setCurrentIndex(index);
		if (m_live_edit)
			connect(combo, &QComboBox::activated, this, &PotentialSourcesWidget::apply);
		m_table->setCellWidget(row, 1, combo);
		m_combos << combo;
		m_terminals << terminal->stableUuid();
	}
}

bool PotentialSourcesWidget::setLiveEdit(bool live_edit)
{
	if (m_live_edit == live_edit)
		return true;
	m_live_edit = live_edit;
	for (QComboBox *combo : std::as_const(m_combos)) {
		if (m_live_edit)
			connect(combo, &QComboBox::activated, this, &PotentialSourcesWidget::apply);
		else
			disconnect(combo, &QComboBox::activated, this, &PotentialSourcesWidget::apply);
	}
	return true;
}

/**
	@return what the rows show, with the types the symbol has on terminals
	its definition no longer has kept as they are
*/
QHash<QUuid, QString> PotentialSourcesWidget::currentSources() const
{
	QHash<QUuid, QString> sources = m_element ? m_element->potentialSources() : QHash<QUuid, QString>();
	for (int row = 0 ; row < m_combos.size() ; ++row) {
		const QString type = m_combos.at(row)->currentData().toString();
		if (type.isEmpty())
			sources.remove(m_terminals.at(row));
		else
			sources.insert(m_terminals.at(row), type);
	}
	return sources;
}

QUndoCommand *PotentialSourcesWidget::associatedUndo() const
{
	if (!m_element)
		return nullptr;
	const QHash<QUuid, QString> sources = currentSources();
	if (sources == m_element->potentialSources())
		return nullptr;
	Element *element = m_element.data();
	const QVariantMap old_value = element->potentialSourcesMap();
	QVariantMap new_value;
	for (auto it = sources.constBegin() ; it != sources.constEnd() ; ++it)
		new_value.insert(it.key().toString(), it.value());
	QPropertyUndoCommand *undo = new QPropertyUndoCommand(element, "potentialSources", old_value, new_value);
	undo->setText(tr("Set the potential sources"));
	return undo;
}

void PotentialSourcesWidget::apply()
{
	if (!m_element || !m_element->diagram())
		return;
	if (QUndoCommand *undo = associatedUndo())
		m_element->diagram()->undoStack().push(undo);
}
