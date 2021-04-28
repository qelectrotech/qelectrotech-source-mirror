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

/**
 * @brief AddTerminalToStripCommand::AddTerminalToStripCommand
 * Add \p terminal to \p strip
 * @param terminal : terminal to add to strip
 * @param strip : terminal strip where terminal must be added
 * @param parent : parent undo command
 */
AddTerminalToStripCommand::AddTerminalToStripCommand(TerminalElement *terminal, TerminalStrip *strip, QUndoCommand *parent) :
    QUndoCommand(parent),
    m_terminal(terminal),
    m_new_strip(strip),
    m_operation(Operation::add)
{
    auto t_label = terminal->actualLabel();
    auto ts_name = strip->name();

    auto str_1 = t_label.isEmpty() ? QObject::tr("Ajouter une borne") :
									 QObject::tr("Ajouter la borne %1").arg(t_label);

    auto str_2 = ts_name.isEmpty() ? QObject::tr("à un groupe de bornes") :
									 QObject::tr("au groupe de bornes %1").arg(ts_name);

    setText(str_1 + " " + str_2);
}

/**
 * @brief AddTerminalToStripCommand::AddTerminalToStripCommand
 * Move \p terminal from \p old_strip to \p new_strip
 * @param terminal : terminal to move
 * @param old_strip : terminal where start the move
 * @param new_strip : terminal where finish the move
 * @param parent : parent undo command
 */
AddTerminalToStripCommand::AddTerminalToStripCommand(TerminalElement *terminal, TerminalStrip *old_strip,
                                                     TerminalStrip *new_strip, QUndoCommand *parent) :
    QUndoCommand(parent),
    m_terminal(terminal),
    m_old_strip(old_strip),
    m_new_strip(new_strip),
    m_operation(Operation::move)
{
    auto t_label = terminal->actualLabel();
    auto old_ts_name = old_strip->name();
    auto new_ts_name = new_strip->name();

    auto str_1 = t_label.isEmpty() ? QObject::tr("Déplacer une borne") :
									 QObject::tr("Déplacer la borne %1").arg(t_label);

    auto str_2 = old_ts_name.isEmpty() ? QObject::tr("d'un groupe de bornes") :
                                         QObject::tr("du groupe de bornes %1").arg(old_ts_name);

    auto str_3 = new_ts_name.isEmpty() ? QObject::tr("à un autre groupe de bornes") :
                                         QObject::tr("au groupe de bornes %1").arg(new_ts_name);

	setText(str_1 + " " + str_2 + " " + str_3);
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

    if ( m_operation == Operation::move &&
         m_old_strip) {
        m_old_strip->addTerminal(m_terminal);
    }
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

    if (m_operation == Operation::move &&
        m_old_strip) {
        m_old_strip->removeTerminal(m_terminal);
    }

    m_new_strip->addTerminal(m_terminal);
}
