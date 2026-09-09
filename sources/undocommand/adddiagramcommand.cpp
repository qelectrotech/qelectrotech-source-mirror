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
#include "adddiagramcommand.h"

#include "../qetproject.h"
#include "../diagram.h"

/**
	@brief AddDiagramCommand::AddDiagramCommand
	@param project : project the diagram is added to
	@param diagram : the already constructed diagram to add (not yet part of any project's diagram list)
	@param pos : position to insert the diagram at, -1 to append at the end
	@param parent
*/
AddDiagramCommand::AddDiagramCommand(QETProject *project, Diagram *diagram, int pos, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_project(project),
	m_diagram(diagram),
	m_position(pos)
{
	setText(QObject::tr("Ajouter un folio", "undo command text"));
}

/**
	@brief AddDiagramCommand::~AddDiagramCommand
	Destroy the diagram only if it is currently detached from the project
	(i.e. this add was undone) -- otherwise the project owns it and will
	destroy it itself.
*/
AddDiagramCommand::~AddDiagramCommand()
{
	if (m_owns_diagram && m_diagram) {
		m_diagram->deleteLater();
	}
}

void AddDiagramCommand::redo()
{
	if (!m_project || !m_diagram) {
		return;
	}
	m_project->addDiagram(m_diagram, m_position);
	m_owns_diagram = false;
}

void AddDiagramCommand::undo()
{
	if (!m_project || !m_diagram) {
		return;
	}
	m_project->detachDiagram(m_diagram);
	m_owns_diagram = true;
}
