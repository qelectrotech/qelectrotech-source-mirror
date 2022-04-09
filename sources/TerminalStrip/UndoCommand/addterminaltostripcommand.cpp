/*
        Copyright 2006-2021 The QElectroTech Team
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
    QUndoCommand(parent),
    m_terminal(terminal),
	m_new_strip(strip)
{
	auto t_label = terminal->label();
    auto ts_name = strip->name();

    auto str_1 = t_label.isEmpty() ? QObject::tr("Ajouter une borne") :
									 QObject::tr("Ajouter la borne %1").arg(t_label);

    auto str_2 = ts_name.isEmpty() ? QObject::tr("à un groupe de bornes") :
									 QObject::tr("au groupe de bornes %1").arg(ts_name);

    setText(str_1 + " " + str_2);
}


AddTerminalToStripCommand::~AddTerminalToStripCommand()
{}

/**
 * @brief AddTerminalToStripCommand::undo
 * Reimplemented from QUndoCommand
 */
void AddTerminalToStripCommand::undo()
{
    if (!m_terminal ||
        !m_new_strip) {
        return;
    }
    m_new_strip->removeTerminal(m_terminal);
}

/**
 * @brief AddTerminalToStripCommand::redo
 * Reimplemented from QUndoCommand
 */
void AddTerminalToStripCommand::redo()
{
    if (!m_terminal ||
        !m_new_strip) {
        return;
    }
    m_new_strip->addTerminal(m_terminal);
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
	QUndoCommand(parent),
	m_terminals(terminal->realTerminals()),
	m_strip(strip)
{	
	QString t_label;
	for (const auto &real_t : m_terminals) {
		if (!t_label.isEmpty())
			t_label.append(", ");
		t_label.append(real_t->label());
	}

	auto strip_name = strip->name();

	auto str_1 = t_label.isEmpty() ? QObject::tr("Enlever une borne") :
									 QObject::tr("Enlever la borne %1").arg(t_label);

	auto str_2 = strip_name.isEmpty() ? QObject::tr("d'un groupe de bornes") :
										QObject::tr("du groupe de bornes %1").arg(strip_name);
	setText(str_1 + " " + str_2);
}

void RemoveTerminalFromStripCommand::undo()
{
	if (m_strip)
	{
		for (const auto &real_t : m_terminals) {
			m_strip->addTerminal(real_t);
		}
		auto phy_t = m_terminals.first()->physicalTerminal();
		if (phy_t) {
			m_strip->groupTerminals(phy_t, m_terminals);
		}
	}
}

void RemoveTerminalFromStripCommand::redo()
{
	if (m_strip)
	{
		for (const auto & real_t : m_terminals) {
			m_strip->removeTerminal(real_t);
		}
	}
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
	QUndoCommand (parent),
	m_terminal(terminal),
	m_old_strip(old_strip),
	m_new_strip(new_strip)
{
	QString t_label;
	for (auto real_t : terminal->realTerminals()) {
		if (!t_label.isEmpty())
			t_label.append(", ");
		t_label.append(real_t->label());
	}

	auto strip_name = old_strip->name();
	auto new_strip_name = new_strip->name();

	auto str_1 = t_label.isEmpty() ? QObject::tr("Déplacer une borne") :
									 QObject::tr("Déplacer la borne %1").arg(t_label);

	auto str_2 = strip_name.isEmpty() ? QObject::tr(" d'un groupe de bornes") :
										QObject::tr(" du groupe de bornes %1").arg(strip_name);

	auto str_3 = new_strip_name.isEmpty() ? QObject::tr("vers un groupe de bornes") :
											QObject::tr("vers le groupe de bornes %1").arg(new_strip_name);
	setText(str_1 + " " + str_2 + " " + str_3);
}

void MoveTerminalCommand::undo()
{
	if (m_terminal)
	{
		if (m_new_strip) {
			m_new_strip->removeTerminal(m_terminal);
		}
		if (m_old_strip) {
			m_old_strip->addTerminal(m_terminal);
		}
	}
}

void MoveTerminalCommand::redo()
{
	if (m_terminal)
	{
		if (m_old_strip) {
			m_old_strip->removeTerminal(m_terminal);
		}
		if (m_new_strip) {
			m_new_strip->addTerminal(m_terminal);
		}
	}
}
