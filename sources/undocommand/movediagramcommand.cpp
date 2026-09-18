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
#include "movediagramcommand.h"

#include "../projectview.h"
#include "../qetproject.h"

/**
	@brief MoveDiagramCommand::MoveDiagramCommand
	@param project_view : project view the diagram's tab belongs to
	@param diagram : the diagram to move
	@param new_position : target position (may be out of range, e.g. for a
	x10/x100 move near either end -- ProjectView::setDiagramPosition() clamps
	it the same way the underlying tab bar always has)
	@param parent
*/
MoveDiagramCommand::MoveDiagramCommand(ProjectView *project_view, Diagram *diagram, int new_position, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_project_view(project_view),
	m_diagram(diagram),
	m_old_position(project_view ? project_view->project()->folioIndex(diagram) : -1),
	m_new_position(new_position)
{
	setText(QObject::tr("Déplacer un folio", "undo command text"));
}

void MoveDiagramCommand::redo()
{
	if (!m_project_view) {
		return;
	}
	m_project_view->setDiagramPosition(m_diagram, m_new_position);
}

void MoveDiagramCommand::undo()
{
	if (!m_project_view) {
		return;
	}
	m_project_view->setDiagramPosition(m_diagram, m_old_position);
}
