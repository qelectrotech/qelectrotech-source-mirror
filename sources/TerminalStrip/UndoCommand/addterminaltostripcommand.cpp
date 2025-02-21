/*
	Copyright 2006-2025 The QElectroTech Team
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

	const auto str_1 = t_label.isEmpty() ? QObject::tr("Ajouter une borne") :
									 QObject::tr("Ajouter la borne %1").arg(t_label);

	const auto str_2 = ts_name.isEmpty() ? QObject::tr("à un groupe de bornes") :
									 QObject::tr("au groupe de bornes %1").arg(ts_name);

	setText(str_1 + " " + str_2);
}

AddTerminalToStripCommand::AddTerminalToStripCommand(QVector<QSharedPointer<RealTerminal>> terminals, TerminalStrip *strip, QUndoCommand *parent) :
	QUndoCommand{parent},
	m_terminal{terminals},
	m_new_strip{strip}
{
	const auto ts_name = strip->name();

	const auto str_1 = m_terminal.size() > 1 ? QObject::tr("Ajouter %1 bornes").arg(m_terminal.size()) :
											   QObject::tr("Ajouter une borne");

	const auto str_2 = ts_name.isEmpty() ? QObject::tr("à un groupe de bornes") :
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

	const auto str_1 = m_terminals.size()>1 ? QObject::tr("Enlever %1 bornes").arg(m_terminals.size()):
											  QObject::tr("Enlever une borne");

	const auto str_2 = strip_name.isEmpty() ? QObject::tr("d'un groupe de bornes") :
											  QObject::tr("du groupe de bornes %1").arg(strip_name);
	setText(str_1 + " " + str_2);
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

MoveTerminalCommand::MoveTerminalCommand(QVector<QSharedPointer<PhysicalTerminal>> terminals, TerminalStrip *old_strip,
										 TerminalStrip *new_strip, QUndoCommand *parent) :
	QUndoCommand {parent},
	m_terminal {terminals},
	m_old_strip {old_strip},
	m_new_strip {new_strip}

{
	const auto strip_name = old_strip->name();
	const auto new_strip_name = new_strip->name();

	const auto str_1 = m_terminal.size() > 1 ? QObject::tr("Déplacer des bornes") :
											   QObject::tr("Déplacer une borne");

	const auto str_2 = strip_name.isEmpty() ? QObject::tr(" d'un groupe de bornes") :
											  QObject::tr(" du groupe de bornes %1").arg(strip_name);

	const auto str_3 = new_strip_name.isEmpty() ? QObject::tr("vers un groupe de bornes") :
												  QObject::tr("vers le groupe de bornes %1").arg(new_strip_name);

	setText(str_1 + " " + str_2 + " " + str_3);
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
