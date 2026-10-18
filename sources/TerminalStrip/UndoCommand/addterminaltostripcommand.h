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
#ifndef ADDTERMINALTOSTRIPCOMMAND_H
#define ADDTERMINALTOSTRIPCOMMAND_H

#include <QUndoCommand>
#include <QPointer>
#include <QVector>
#include <QSharedPointer>
#include "../../properties/elementdata.h"

class TerminalElement;
class TerminalStrip;
class RealTerminal;
class PhysicalTerminal;

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
		AddTerminalToStripCommand(QSharedPointer<RealTerminal> terminal, TerminalStrip *strip, QUndoCommand *parent = nullptr);
		AddTerminalToStripCommand(QVector<QSharedPointer<RealTerminal>> terminals, TerminalStrip *strip, QUndoCommand *parent = nullptr);
		~AddTerminalToStripCommand() override;

		void undo() override;
		void redo() override;

	private:
		QVector<QSharedPointer<RealTerminal>> m_terminal;
		QPointer<TerminalStrip> m_new_strip;
};

/**
 * @brief The RemoveTerminalFromStripCommand class
 * Remove a terminal from a terminal strip.
 * The removed terminal become free.
 */
class RemoveTerminalFromStripCommand : public QUndoCommand
{
	public:
		RemoveTerminalFromStripCommand (QSharedPointer<PhysicalTerminal> terminal, TerminalStrip *strip, QUndoCommand *parent = nullptr);
		RemoveTerminalFromStripCommand (const QVector<QSharedPointer<PhysicalTerminal>> &phy_t_vector, TerminalStrip *strip, QUndoCommand *parent = nullptr);
		~RemoveTerminalFromStripCommand() override {}

		void undo() override;
		void redo() override;

	private:
		void setCommandTitle();

	private:
		QVector<QVector<QSharedPointer<RealTerminal>>> m_terminals;
		QPointer<TerminalStrip> m_strip;
};

class MoveTerminalCommand : public QUndoCommand
{
	public:
		MoveTerminalCommand (QSharedPointer<PhysicalTerminal> terminal, TerminalStrip *old_strip,
							 TerminalStrip *new_strip, QUndoCommand *parent = nullptr);
		MoveTerminalCommand (QVector<QSharedPointer<PhysicalTerminal>> terminals, TerminalStrip *old_strip,
							 TerminalStrip *new_strip, QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;

	private:
		const QVector<QSharedPointer<PhysicalTerminal>> m_terminal;
		QPointer<TerminalStrip> m_old_strip, m_new_strip;

};

/**
 * @brief The AddSpareTerminalsCommand class
 * Insert spare terminals (on no folio) in a terminal strip,
 * at a position of the strip.
 */
class AddSpareTerminalsCommand : public QUndoCommand
{
	public:
		AddSpareTerminalsCommand(TerminalStrip *strip, int pos,
								 const QVector<QSharedPointer<RealTerminal>> &spare_terminals,
								 QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;

	private:
		QPointer<TerminalStrip> m_strip;
		int m_pos = -1;
		QVector<QSharedPointer<RealTerminal>> m_spare_terminals;
		QVector<QSharedPointer<PhysicalTerminal>> m_physical_terminals;
};

/**
 * @brief The ChangeSpareTerminalDataCommand class
 * Change the data (label, type, section...) of a spare terminal
 */
class ChangeSpareTerminalDataCommand : public QUndoCommand
{
	public:
		ChangeSpareTerminalDataCommand(const QSharedPointer<RealTerminal> &spare_terminal,
									   const ElementData &new_data,
									   QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;

	private:
		QSharedPointer<RealTerminal> m_spare_terminal;
		ElementData m_old_data, m_new_data;
};

#endif // ADDTERMINALTOSTRIPCOMMAND_H
