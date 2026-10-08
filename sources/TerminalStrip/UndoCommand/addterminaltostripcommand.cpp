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
#include "addterminaltostripcommand.h"
#include "../../qetgraphicsitem/terminalelement.h"
#include "../realterminal.h"
#include "../physicalterminal.h"

/**
 * @brief AddTerminalToStripCommand::AddTerminalToStripCommand
 * Add \p terminal to \p strip
 * @param terminal : terminal to add to strip
 * @param strip : terminal strip where terminal must be added
 * @param parent : parent undo command
 */
AddTerminalToStripCommand::AddTerminalToStripCommand(QSharedPointer<RealTerminal> terminal, TerminalStrip *strip, QUndoCommand *parent) :
	QUndoCommand{parent},
	m_terminal{terminal},
	m_new_strip{strip}
{
	const auto t_label = terminal->label();
	const auto ts_name = strip->name();

	QString text;
	if (ts_name.isEmpty()) {
		text = t_label.isEmpty()
			? QObject::tr("Ajouter une borne à un groupe de bornes")
			: QObject::tr("Ajouter la borne %1 à un groupe de bornes").arg(t_label);
	} else {
		text = t_label.isEmpty()
			? QObject::tr("Ajouter une borne au groupe de bornes %1").arg(ts_name)
			: QObject::tr("Ajouter la borne %1 au groupe de bornes %2").arg(t_label, ts_name);
	}
	setText(text);
}

AddTerminalToStripCommand::AddTerminalToStripCommand(QVector<QSharedPointer<RealTerminal>> terminals, TerminalStrip *strip, QUndoCommand *parent) :
	QUndoCommand{parent},
	m_terminal{terminals},
	m_new_strip{strip}
{
	const auto ts_name = strip->name();
	const auto count = m_terminal.size();

	setText(ts_name.isEmpty()
		? QObject::tr("Ajouter %n borne(s) à un groupe de bornes", "", count)
		: QObject::tr("Ajouter %n borne(s) au groupe de bornes %1", "", count).arg(ts_name));
}


AddTerminalToStripCommand::~AddTerminalToStripCommand()
{}

/**
 * @brief AddTerminalToStripCommand::undo
 * Reimplemented from QUndoCommand
 */
void AddTerminalToStripCommand::undo()
{
	if (m_new_strip) {
		m_new_strip->removeTerminals(m_terminal);
	}
}

/**
 * @brief AddTerminalToStripCommand::redo
 * Reimplemented from QUndoCommand
 */
void AddTerminalToStripCommand::redo()
{
	if (m_new_strip) {
		m_new_strip->addTerminals(m_terminal);
	}
}

/**
 * @brief RemoveTerminalFromStripCommand::RemoveTerminalFromStripCommand
 * @param terminal
 * @param strip
 * @param parent
 */
RemoveTerminalFromStripCommand::RemoveTerminalFromStripCommand(QSharedPointer<PhysicalTerminal> terminal,
															   TerminalStrip *strip,
															   QUndoCommand *parent) :
	QUndoCommand{parent},
	m_strip{strip}
{
	m_terminals.append(terminal->realTerminals());
	setCommandTitle();
}

RemoveTerminalFromStripCommand::RemoveTerminalFromStripCommand(const QVector<QSharedPointer<PhysicalTerminal> > &phy_t_vector,
															   TerminalStrip *strip,
															   QUndoCommand *parent):
	QUndoCommand{parent},
	m_strip{strip}
{
	for (const auto &phy_t : phy_t_vector) {
		m_terminals.append(phy_t->realTerminals());
	}
	setCommandTitle();
}

void RemoveTerminalFromStripCommand::undo()
{
	if (m_strip) {
		m_strip->addAndGroupTerminals(m_terminals);
	}
}

void RemoveTerminalFromStripCommand::redo()
{
	if (m_strip)
	{
		QVector<QSharedPointer<RealTerminal>> real_t;
		for (const auto &real_t_vector : std::as_const(m_terminals)) {
			real_t.append(real_t_vector);
		}

		m_strip->removeTerminals(real_t);
	}
}

void RemoveTerminalFromStripCommand::setCommandTitle()
{
	const auto strip_name = m_strip->name();
	const auto count = m_terminals.size();

	setText(strip_name.isEmpty()
		? QObject::tr("Enlever %n borne(s) d'un groupe de bornes", "", count)
		: QObject::tr("Enlever %n borne(s) du groupe de bornes %1", "", count).arg(strip_name));
}

/**
 * @brief MoveTerminalCommand::MoveTerminalCommand
 * @param terminal
 * @param old_strip
 * @param new_strip
 * @param parent
 */
MoveTerminalCommand::MoveTerminalCommand(QSharedPointer<PhysicalTerminal> terminal, TerminalStrip *old_strip,
										 TerminalStrip *new_strip, QUndoCommand *parent) :
	QUndoCommand {parent},
	m_terminal {terminal},
	m_old_strip {old_strip},
	m_new_strip {new_strip}
{
	QString t_label;
	for (auto real_t : terminal->realTerminals()) {
		if (!t_label.isEmpty())
			t_label.append(", ");
		t_label.append(real_t->label());
	}
	const auto strip_name = old_strip->name();
	const auto new_strip_name = new_strip->name();

	QString text;
	if (t_label.isEmpty()) {
		if (strip_name.isEmpty() && new_strip_name.isEmpty())
			text = QObject::tr("Déplacer une borne d'un groupe de bornes vers un autre groupe de bornes");
		else if (strip_name.isEmpty())
			text = QObject::tr("Déplacer une borne d'un groupe de bornes vers le groupe de bornes %1").arg(new_strip_name);
		else if (new_strip_name.isEmpty())
			text = QObject::tr("Déplacer une borne du groupe de bornes %1 vers un autre groupe de bornes").arg(strip_name);
		else
			text = QObject::tr("Déplacer une borne du groupe de bornes %1 vers le groupe de bornes %2").arg(strip_name, new_strip_name);
	} else {
		if (strip_name.isEmpty() && new_strip_name.isEmpty())
			text = QObject::tr("Déplacer la borne %1 d'un groupe de bornes vers un autre groupe de bornes").arg(t_label);
		else if (strip_name.isEmpty())
			text = QObject::tr("Déplacer la borne %1 d'un groupe de bornes vers le groupe de bornes %2").arg(t_label, new_strip_name);
		else if (new_strip_name.isEmpty())
			text = QObject::tr("Déplacer la borne %1 du groupe de bornes %2 vers un autre groupe de bornes").arg(t_label, strip_name);
		else
			text = QObject::tr("Déplacer la borne %1 du groupe de bornes %2 vers le groupe de bornes %3").arg(t_label, strip_name, new_strip_name);
	}
	setText(text);
}

MoveTerminalCommand::MoveTerminalCommand(QVector<QSharedPointer<PhysicalTerminal>> terminals, TerminalStrip *old_strip,
										 TerminalStrip *new_strip, QUndoCommand *parent) :
	QUndoCommand {parent},
	m_terminal {terminals},
	m_old_strip {old_strip},
	m_new_strip {new_strip}

{
	const auto strip_name = old_strip->name();
	const auto new_strip_name = new_strip->name();
	const auto count = m_terminal.size();

	QString text;
	if (strip_name.isEmpty() && new_strip_name.isEmpty())
		text = QObject::tr("Déplacer %n borne(s) d'un groupe de bornes vers un autre groupe de bornes", "", count);
	else if (strip_name.isEmpty())
		text = QObject::tr("Déplacer %n borne(s) d'un groupe de bornes vers le groupe de bornes %1", "", count).arg(new_strip_name);
	else if (new_strip_name.isEmpty())
		text = QObject::tr("Déplacer %n borne(s) du groupe de bornes %1 vers un autre groupe de bornes", "", count).arg(strip_name);
	else
		text = QObject::tr("Déplacer %n borne(s) du groupe de bornes %1 vers le groupe de bornes %2", "", count).arg(strip_name, new_strip_name);
	setText(text);
}

void MoveTerminalCommand::undo()
{
	if (m_new_strip) {
		m_new_strip->removeTerminals(m_terminal);
	}
	if (m_old_strip) {
		m_old_strip->addTerminals(m_terminal);
	}
}

void MoveTerminalCommand::redo()
{
	if (m_old_strip) {
		m_old_strip->removeTerminals(m_terminal);
	}
	if (m_new_strip) {
		m_new_strip->addTerminals(m_terminal);
	}
}
