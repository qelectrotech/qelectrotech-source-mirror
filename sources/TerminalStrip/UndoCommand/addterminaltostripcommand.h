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
#ifndef ADDTERMINALTOSTRIPCOMMAND_H
#define ADDTERMINALTOSTRIPCOMMAND_H

#include <QUndoCommand>
#include <QPointer>

class TerminalElement;
class TerminalStrip;

/**
 * @brief The AddTerminalToStripCommand class
 * Add a terminal element to a terminal strip
 * Two cases are handled :
 * Add free terminal to strip,
 * Move terminal from strip to another strip
 */
class AddTerminalToStripCommand : public QUndoCommand
{
    public:
        AddTerminalToStripCommand(TerminalElement *terminal, TerminalStrip *strip, QUndoCommand *parent = nullptr);
        AddTerminalToStripCommand(TerminalElement *terminal, TerminalStrip *old_strip,
								  TerminalStrip *new_strip, QUndoCommand *parent = nullptr);
        ~AddTerminalToStripCommand() override;

        void undo() override;
        void redo() override;

    private:
        enum Operation{
            none,
            add,
            move,
        };

        QPointer<TerminalElement> m_terminal;
        QPointer<TerminalStrip> m_old_strip;
        QPointer<TerminalStrip> m_new_strip;
        Operation m_operation = Operation::none;


};

/**
 * @brief The RemoveTerminalFromStripCommand class
 * Remove a terminal from a terminal strip.
 * The removed terminal become free.
 */
class RemoveTerminalFromStripCommand : public QUndoCommand
{
	public:
		RemoveTerminalFromStripCommand (TerminalElement *terminal, TerminalStrip *strip, QUndoCommand *parent = nullptr);
		~RemoveTerminalFromStripCommand() override {}

		void undo() override;
		void redo() override;

	private:
		QPointer<TerminalElement> m_terminal;
		QPointer<TerminalStrip> m_strip;
};

#endif // ADDTERMINALTOSTRIPCOMMAND_H
