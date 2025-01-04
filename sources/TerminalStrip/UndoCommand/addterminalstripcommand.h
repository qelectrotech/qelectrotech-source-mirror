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
#ifndef ADDTERMINALSTRIPCOMMAND_H
#define ADDTERMINALSTRIPCOMMAND_H

#include <QUndoCommand>
#include <QPointer>
#include <QVector>

class TerminalStrip;
class QETProject;
class Element;

class AddTerminalStripCommand : public QUndoCommand
{
	public:
		AddTerminalStripCommand(TerminalStrip *strip, QETProject *project, QUndoCommand *parent = nullptr);
		~AddTerminalStripCommand() override;

		void undo() override;
		void redo() override;

	private:
		QPointer<TerminalStrip> m_strip;
		QPointer<QETProject> m_project;
};

class RemoveTerminalStripCommand : public QUndoCommand
{
	public:
		RemoveTerminalStripCommand(TerminalStrip *strip, QETProject *project, QUndoCommand *parent = nullptr);
		~RemoveTerminalStripCommand() override;

		void undo() override;
		void redo() override;

	private:
		QPointer<TerminalStrip> m_strip;
		QPointer<QETProject> m_project;
		QVector<QPointer<Element>> m_elements;
};

#endif // ADDTERMINALSTRIPCOMMAND_H
