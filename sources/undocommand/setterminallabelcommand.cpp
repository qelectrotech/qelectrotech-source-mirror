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
#include "setterminallabelcommand.h"

#include <qetgraphicsitem/element.h>

#include <algorithm>
#include <utility>

/**
	@brief SetTerminalLabelCommand::SetTerminalLabelCommand
	Give the terminals of @p element the labels of @p changes, one entry
	per terminal the properties changed.
	@param element the placed symbol the terminals belong to
	@param changes the terminals to change, with the label each had before
	and the one it should have after
	@param parent
*/
SetTerminalLabelCommand::SetTerminalLabelCommand(Element *element,
												 const QList<Change> &changes,
												 QUndoCommand *parent) :
	QUndoCommand(parent),
	m_element(element),
	m_changes(changes)
{
	setText(QObject::tr("Change the name or the drawing of a terminal"));
}

/**
	@brief SetTerminalLabelCommand::id
	Seven, so the steps of one rename typed into the properties panel
	merge into one (@see mergeWith())
*/
int SetTerminalLabelCommand::id() const
{
	return 7;
}

/**
	@brief SetTerminalLabelCommand::mergeWith
	Take the newest label of each terminal @p already has a step for, and
	keep the label it had before the first of the steps, so undo goes back
	to where the editing started
	@param other
	@return true when @p other is a step on the same symbol
*/
bool SetTerminalLabelCommand::mergeWith(const QUndoCommand *other)
{
	const auto *command = dynamic_cast<const SetTerminalLabelCommand *>(other);
	if (!command || command->m_element != m_element)
		return false;

	for (const Change &change : std::as_const(command->m_changes)) {
		auto found = std::find_if(m_changes.begin(), m_changes.end(),
								  [&change](const Change &known) {
			return known.terminal == change.terminal;
		});
		if (found == m_changes.end())
			m_changes << change;
		else
			found->new_label = change.new_label;
	}
	return true;
}

void SetTerminalLabelCommand::undo()
{
	for (const Change &change : std::as_const(m_changes))
		setLabel(change.terminal, change.old_label);
}

void SetTerminalLabelCommand::redo()
{
	for (const Change &change : std::as_const(m_changes))
		setLabel(change.terminal, change.new_label);
}

/**
	@brief SetTerminalLabelCommand::setLabel
	One terminal of the symbol: an empty label takes back what the
	definition says
*/
void SetTerminalLabelCommand::setLabel(const QUuid &terminal,
									   const std::optional<TerminalLabel> &label) const
{
	if (!m_element)
		return;
	if (label)
		m_element->setTerminalLabel(terminal, *label);
	else
		m_element->removeTerminalLabel(terminal);
}
