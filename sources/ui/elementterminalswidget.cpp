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
#include "elementterminalswidget.h"

#include "terminallabeldialog.h"

#include <diagram.h>
#include <qetgraphicsitem/element.h>
#include <qetgraphicsitem/terminal.h>
#include <undocommand/setterminallabelcommand.h>

#include <QCheckBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>

#include <optional>
#include <utility>

/**
	@brief ElementTerminalsWidget::ElementTerminalsWidget
	One row per terminal of @p element, showing what the symbol gives it
	(@see ElementTerminalsWidget::updateUi())
*/
ElementTerminalsWidget::ElementTerminalsWidget(Element *element, QWidget *parent) :
	AbstractElementPropertiesEditorWidget(parent)
{
	QLabel *help = new QLabel(tr("What a row changes belongs to this copy of the symbol: "
								 "the definition keeps its own values, and so do the other "
								 "copies of the symbol."), this);
	help->setWordWrap(true);

	m_empty_lb = new QLabel(tr("This symbol has no terminal."), this);
	m_empty_lb->setWordWrap(true);
	m_empty_lb->hide();

	m_table = new QTableWidget(0, 4, this);
	m_table->setHorizontalHeaderLabels({tr("Show"), tr("Name"),
										tr("Text properties"), tr("Reset")});
	m_table->verticalHeader()->hide();
	m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	m_table->setSelectionMode(QAbstractItemView::NoSelection);
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(help);
	layout->addWidget(m_empty_lb);
	layout->addWidget(m_table);
	setLayout(layout);

	setElement(element);
}

void ElementTerminalsWidget::setElement(Element *element)
{
	m_element = element;
	updateUi();
}

QString ElementTerminalsWidget::title() const
{
	return tr("Terminals");
}

/**
	@brief ElementTerminalsWidget::updateUi
	Fill the table with the terminals of the symbol, each row showing the
	values that terminal has -- what the definition gave it, or what this
	copy of the symbol was given afterwards
*/
void ElementTerminalsWidget::updateUi()
{
	m_table->setRowCount(0);
	m_rows.clear();
	if (!m_element) {
		m_empty_lb->hide();
		m_table->show();
		return;
	}

	const QList<Terminal *> terminals = m_element->terminals();
	m_empty_lb->setVisible(terminals.isEmpty());
	m_table->setVisible(!terminals.isEmpty());
	m_table->setRowCount(static_cast<int>(terminals.size()));
	for (int row = 0 ; row < terminals.size() ; ++row)
		buildRow(row, terminals.at(row));
}

/**
	@brief ElementTerminalsWidget::buildRow
	Make the row @p row of @p terminal: the checkbox which shows the name,
	the line where it is written, the button to how it is drawn and the
	button which takes back what the definition says
*/
void ElementTerminalsWidget::buildRow(int row, Terminal *terminal)
{
	Row entry;
	entry.terminal = terminal;
	entry.label = terminal->label();
	entry.show = new QCheckBox(m_table);
	entry.name = new QLineEdit(m_table);
	entry.font = new QPushButton(m_table);
	entry.font->setToolTip(tr("Set where the name sits, how big and how turned it is, "
							  "how aligned, whether it is framed and in which colour"));
	entry.reset = new QPushButton(m_table);
	entry.reset->setText(tr("Reset"));
	entry.reset->setToolTip(tr("Take back the values the definition of the symbol gives"));

	m_table->setCellWidget(row, 0, entry.show);
	m_table->setCellWidget(row, 1, entry.name);
	m_table->setCellWidget(row, 2, entry.font);
	m_table->setCellWidget(row, 3, entry.reset);
	m_rows << entry;
	showRowValues(row);

	connect(entry.show, &QCheckBox::toggled, this, [this, row]() {showChanged(row);});
	connect(entry.name, &QLineEdit::textChanged, this, [this, row]() {nameChanged(row);});
	connect(entry.font, &QPushButton::clicked, this, [this, row]() {fontClicked(row);});
	connect(entry.reset, &QPushButton::clicked, this, [this, row]() {resetClicked(row);});
}

/**
	@brief ElementTerminalsWidget::showRowValues
	Show what the row @p row holds in its widgets.
	The widgets say nothing back while it does: a terminal which takes its
	name from the master shows the name the link gives it, and that is not
	the name this row writes back
*/
void ElementTerminalsWidget::showRowValues(int row)
{
	const Row &entry = m_rows.at(row);
	const bool from_master = entry.terminal && entry.terminal->useMasterLabel();

	const QSignalBlocker show_blocker(entry.show);
	entry.show->setChecked(entry.label.m_show_name);

	const QSignalBlocker name_blocker(entry.name);
	entry.name->setText(from_master ? entry.terminal->name() : entry.label.m_name);
	entry.name->setEnabled(!from_master);
	entry.name->setToolTip(from_master
						   ? tr("The name comes from the master this terminal is linked to")
						   : QString());

	const QSignalBlocker font_blocker(entry.font);
	entry.font->setText(entry.label.m_label_font.family());
}

/**
	@brief ElementTerminalsWidget::nameChanged
	The name written in the row @p row is the one this copy of the symbol
	gives its terminal; in the properties panel it is applied at once
*/
void ElementTerminalsWidget::nameChanged(int row)
{
	if (row < 0 || row >= m_rows.size() || !m_rows.at(row).name->isEnabled())
		return;
	m_rows[row].label.m_name = m_rows.at(row).name->text();
	if (m_live_edit)
		apply();
}

/**
	@brief ElementTerminalsWidget::showChanged
	Whether the row @p row shows the name of its terminal at all
*/
void ElementTerminalsWidget::showChanged(int row)
{
	if (row < 0 || row >= m_rows.size())
		return;
	m_rows[row].label.m_show_name = m_rows.at(row).show->isChecked();
	if (m_live_edit)
		apply();
}

/**
	@brief ElementTerminalsWidget::fontClicked
	Ask how the name of the row @p row is drawn; what the dialog hands
	back becomes what the row shows, applied at once in the properties
	panel
*/
void ElementTerminalsWidget::fontClicked(int row)
{
	if (row < 0 || row >= m_rows.size())
		return;
	TerminalLabelDialog dialog(m_rows.at(row).label, this);
	if (dialog.exec() != QDialog::Accepted)
		return;

	m_rows[row].label = dialog.label();
	showRowValues(row);
	if (m_live_edit)
		apply();
}

/**
	@brief ElementTerminalsWidget::resetClicked
	Take back what the definition says about the terminal of the row
	@p row: the row shows the values of the definition again, and the
	next apply() takes them off this copy of the symbol
*/
void ElementTerminalsWidget::resetClicked(int row)
{
	if (row < 0 || row >= m_rows.size() || !m_rows.at(row).terminal)
		return;
	m_rows[row].label = m_rows.at(row).terminal->definitionLabel();
	showRowValues(row);
	if (m_live_edit)
		apply();
}

/**
	@brief ElementTerminalsWidget::apply
	Apply what the rows show by pushing one undo command with it, on the
	stack of the folio the symbol is on
*/
void ElementTerminalsWidget::apply()
{
	if (!m_element || !m_element->diagram())
		return;
	if (QUndoCommand *undo = associatedUndo())
		m_element->diagram()->undoStack().push(undo);
}

/**
	@brief ElementTerminalsWidget::associatedUndo
	@return a command with every row which shows something other than what
	the symbol has -- the terminal's own values kept as they are, a row
	showing the definition again taking its override off -- or nullptr
	when no row changed
*/
QUndoCommand *ElementTerminalsWidget::associatedUndo() const
{
	if (!m_element)
		return nullptr;
	Element *element = m_element.data();
	const QHash<QUuid, TerminalLabel> labels = element->terminalLabels();

	QList<SetTerminalLabelCommand::Change> changes;
	for (const Row &entry : std::as_const(m_rows)) {
		if (!entry.terminal)
			continue;
		const QUuid uuid = entry.terminal->stableUuid();
		const auto found = labels.constFind(uuid);
		const std::optional<TerminalLabel> was = found == labels.constEnd()
				? std::nullopt
				: std::optional<TerminalLabel>(found.value());
			//A row showing the definition again takes the values of the
			//definition off the symbol, rather than writing them over them
		const std::optional<TerminalLabel> want =
				entry.label == entry.terminal->definitionLabel()
				? std::nullopt
				: std::optional<TerminalLabel>(entry.label);
		if (was != want)
			changes.append({uuid, was, want});
	}
	if (changes.isEmpty())
		return nullptr;
	return new SetTerminalLabelCommand(element, changes);
}

/**
	@brief ElementTerminalsWidget::setLiveEdit
	@param live_edit true to apply every change at once, as the properties
	panel does, false to wait for the dialog's Apply
	@return always true: this editor can be live
*/
bool ElementTerminalsWidget::setLiveEdit(bool live_edit)
{
	m_live_edit = live_edit;
	return true;
}
