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
#ifndef GROUPTERMINALSCOMMAND_H
#define GROUPTERMINALSCOMMAND_H

#include <QUndoCommand>
#include <QPointer>
#include <QVector>
#include "../terminalstrip.h"

/**
 * @brief The GroupTerminalsCommand class
 * Class used to group (make level terminal) together
 */
class GroupTerminalsCommand : public QUndoCommand
{
	public:
		GroupTerminalsCommand(TerminalStrip *strip,
							  const QSharedPointer<PhysicalTerminal> &receiver_,
							  const QVector<QSharedPointer<RealTerminal>> &to_group,
							  QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;

	private:
		QPointer<TerminalStrip> m_terminal_strip;
		QSharedPointer<PhysicalTerminal> m_receiver;
		QVector<QSharedPointer<RealTerminal>> m_to_group;
};

/**
 * @brief The UnGroupTerminalsCommand class
 * This class is used to ungroup terminal (remove level of multi level terminal)
 */
class UnGroupTerminalsCommand : public QUndoCommand
{
	public:
		UnGroupTerminalsCommand(TerminalStrip *strip,
								const QVector<QSharedPointer<RealTerminal>> &to_ungroup,
								QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;

	private:
		void setUp(const QVector<QSharedPointer<RealTerminal>> &to_ungroup);

	private:
		QPointer<TerminalStrip> m_terminal_strip;
		QHash <QSharedPointer<PhysicalTerminal>, QVector<QSharedPointer<RealTerminal>>> m_physical_real_H;
		QVector<QPair<QSharedPointer<RealTerminal>, int>> m_real_t_level;
};

#endif // GROUPTERMINALSCOMMAND_H
