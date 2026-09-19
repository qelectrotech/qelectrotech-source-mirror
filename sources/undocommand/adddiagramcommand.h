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
#ifndef ADDDIAGRAMCOMMAND_H
#define ADDDIAGRAMCOMMAND_H

#include <QUndoCommand>
#include <QPointer>

class QETProject;
class Diagram;

/**
	@brief The AddDiagramCommand class
	Undo/redo support for adding a new folio (diagram) to a project.
	Takes ownership of the (already constructed, not yet inserted) diagram:
	redo() inserts it into the project, undo() detaches it again without
	destroying it. The diagram is only actually destroyed when this command
	itself is destroyed while the diagram is detached (i.e. while undone).
*/
class AddDiagramCommand : public QUndoCommand
{
	public:
		AddDiagramCommand(QETProject *project, Diagram *diagram, int pos = -1, QUndoCommand *parent = nullptr);
		~AddDiagramCommand() override;

		void undo() override;
		void redo() override;

	private:
		QPointer<QETProject> m_project;
		Diagram *m_diagram;
		int m_position;
		/// true while the diagram is detached from the project (not currently in its diagram list)
		bool m_owns_diagram = true;
};

#endif // ADDDIAGRAMCOMMAND_H
