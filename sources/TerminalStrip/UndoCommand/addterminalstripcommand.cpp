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
#include "addterminalstripcommand.h"
#include "../../qetproject.h"
#include "../terminalstrip.h"
#include "../qetgraphicsitem/element.h"
#include "../realterminal.h"

#include <QObject>

/**
 * @brief AddTerminalStripCommand::AddTerminalStripCommand
 * @param strip
 * @param parent
 */
AddTerminalStripCommand::AddTerminalStripCommand(TerminalStrip *strip,
												 QETProject    *project,
												 QUndoCommand  *parent) :
	QUndoCommand(parent),
	m_strip(strip),
	m_project(project)
{
	setText(QObject::tr("Ajouter un groupe de bornes"));
}

AddTerminalStripCommand::~AddTerminalStripCommand()
{}

void AddTerminalStripCommand::undo() {
		if (m_project && m_strip) {
				m_project->removeTerminalStrip(m_strip);
		}
}

void AddTerminalStripCommand::redo() {
	if (m_project && m_strip) {
			m_project->addTerminalStrip(m_strip);
	}
}

RemoveTerminalStripCommand::RemoveTerminalStripCommand(TerminalStrip *strip,
													   QETProject    *project,
													   QUndoCommand  *parent) :
	QUndoCommand(parent),
	m_strip(strip),
	m_project(project)
{
	for (const auto &real_t : strip->realTerminals())
	{
		if (real_t->element())
			m_elements.append(real_t->element());
	}
	setText(QObject::tr("Supprimer un groupe de bornes"));
}

RemoveTerminalStripCommand::~RemoveTerminalStripCommand()
{}

void RemoveTerminalStripCommand::undo()
{
	if (m_project && m_strip) {
		for (auto elmt : m_elements) {
			m_strip->addTerminal(elmt);
		}
		m_project->addTerminalStrip(m_strip);
	}
}

void RemoveTerminalStripCommand::redo()
{
	if (m_project && m_strip) {
		for (auto elmt : m_elements) {
			m_strip->removeTerminal(elmt);
		}
		m_project->removeTerminalStrip(m_strip);
	}
}
