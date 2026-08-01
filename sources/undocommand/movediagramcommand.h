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
#ifndef MOVEDIAGRAMCOMMAND_H
#define MOVEDIAGRAMCOMMAND_H

#include <QUndoCommand>
#include <QPointer>

class ProjectView;
class Diagram;

/**
	@brief The MoveDiagramCommand class
	Undo/redo support for reordering a folio (diagram) within a project's
	tabs. Wraps ProjectView::setDiagramPosition(), which performs the tab
	move and the matching (synchronous) project diagram-list reorder in one
	step.
*/
class MoveDiagramCommand : public QUndoCommand
{
	public:
		MoveDiagramCommand(ProjectView *project_view, Diagram *diagram, int new_position, QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;

	private:
		QPointer<ProjectView> m_project_view;
		Diagram *m_diagram;
		int m_old_position;
		int m_new_position;
};

#endif // MOVEDIAGRAMCOMMAND_H
