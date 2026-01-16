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
#ifndef BRIDGETERMINALSCOMMAND_H
#define BRIDGETERMINALSCOMMAND_H

#include <QUndoCommand>
#include <QVector>
#include <QPointer>
#include <QMultiMap>

#include "../terminalstrip.h"

/**
 * @brief The BridgeTerminalsCommand class
 * UndoCommand use to create bridge between terminals
 * of a terminals strip
 */
class BridgeTerminalsCommand : public QUndoCommand
{
	public:
		BridgeTerminalsCommand(TerminalStrip *strip, QVector<QSharedPointer<RealTerminal>> real_terminal, QUndoCommand *parent = nullptr);
		~BridgeTerminalsCommand() override {}

		void undo() override;
		void redo() override;

	private:
		QPointer<TerminalStrip> m_strip;
		QVector<QSharedPointer<RealTerminal>> m_real_terminal_vector;
		QSharedPointer<TerminalStripBridge> m_bridge;
};


/**
 * @brief The UnBridgeTerminalsCommand class
 * UndoCommand use to remove bridge between terminals
 * of a terminals strip
 */
class UnBridgeTerminalsCommand : public QUndoCommand
{
	public:
		UnBridgeTerminalsCommand(TerminalStrip *strip, QVector<QSharedPointer<RealTerminal>> real_terminal, QUndoCommand *parent = nullptr);
		~UnBridgeTerminalsCommand() override{}

		void undo() override;
		void redo() override;

	private:
		QPointer<TerminalStrip> m_strip;
		QSharedPointer<TerminalStripBridge> m_bridge;
		QVector<QSharedPointer<RealTerminal>> m_terminals;
};

#endif // BRIDGETERMINALSCOMMAND_H
