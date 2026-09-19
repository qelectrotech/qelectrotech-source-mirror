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
#include "removediagramcommand.h"

#include "../qetproject.h"
#include "../diagram.h"

/**
	@brief RemoveDiagramCommand::RemoveDiagramCommand
	@param project : project the diagram is removed from
	@param diagram : the diagram to remove, currently part of \p project
	@param parent
*/
RemoveDiagramCommand::RemoveDiagramCommand(QETProject *project, Diagram *diagram, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_project(project),
	m_diagram(diagram),
	m_position(project ? project->folioIndex(diagram) : -1)
{
	setText(QObject::tr("Supprimer un folio", "undo command text"));
}

/**
	@brief RemoveDiagramCommand::~RemoveDiagramCommand
	Destroy the diagram only if it is currently detached from the project
	(i.e. the removal is currently in effect) -- otherwise the project owns
	it and will destroy it itself.
*/
RemoveDiagramCommand::~RemoveDiagramCommand()
{
	if (m_owns_diagram && m_diagram) {
		m_diagram->deleteLater();
	}
}

void RemoveDiagramCommand::redo()
{
	if (!m_project || !m_diagram) {
		return;
	}
	m_project->detachDiagram(m_diagram);
	m_owns_diagram = true;
}

void RemoveDiagramCommand::undo()
{
	if (!m_project || !m_diagram) {
		return;
	}
	m_project->addDiagram(m_diagram, m_position);
	m_owns_diagram = false;
}
