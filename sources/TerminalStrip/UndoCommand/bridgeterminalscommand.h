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
#ifndef BRIDGETERMINALSCOMMAND_H
#define BRIDGETERMINALSCOMMAND_H

#include <QUndoCommand>
#include <QVector>
#include <QUuid>
#include <QPointer>
#include <QMultiMap>

class TerminalStrip;

/**
 * @brief The BridgeTerminalsCommand class
 * UndoCommand use to create bridge betwen terminals
 * of a terminals strip
 */
class BridgeTerminalsCommand : public QUndoCommand
{
	public:
		BridgeTerminalsCommand(TerminalStrip *strip, QVector<QUuid> real_terminal_uuid, QUndoCommand *parent = nullptr);
		~BridgeTerminalsCommand() override {}

		void undo() override;
		void redo() override;

	private:
		QPointer<TerminalStrip> m_strip;
		QVector<QUuid> m_uuid_vector;
};


/**
 * @brief The UnBridgeTerminalsCommand class
 * UndoCommand use to remove bridge betwen terminals
 * of a terminals strip
 */
class UnBridgeTerminalsCommand : public QUndoCommand
{
	public:
		UnBridgeTerminalsCommand(TerminalStrip *strip, QVector<QUuid> real_terminal_uuid, QUndoCommand *parent = nullptr);
		~UnBridgeTerminalsCommand() override{}

		void undo() override;
		void redo() override;

	private:
		QPointer<TerminalStrip> m_strip;
		QMultiMap<QUuid, QUuid> m_bridge_terminal_map; ///Key is bridge uuid, value is real terminal uuid

};

#endif // BRIDGETERMINALSCOMMAND_H
