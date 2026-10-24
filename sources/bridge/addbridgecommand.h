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
#ifndef ADDBRIDGECOMMAND_H
#define ADDBRIDGECOMMAND_H

#include "bridge.h"

#include <QList>
#include <QPointer>
#include <QUndoCommand>

class BridgePart;
class Diagram;
class QETProject;

/**
	@brief Puts a drawn bridge line on the folio, or takes it back off.

	Both halves are one single step, because a bridge line without its
	bridge is of no use to anybody: undoing has to take the line away
	and leave nothing behind, and redoing has to put both back exactly
	as they were. The same step gives a bridge the terminals the line
	landed on, so one undo takes those away again as well.
*/
class AddBridgeCommand : public QUndoCommand
{
	public:
		AddBridgeCommand(Bridge *bridge,
						 BridgePart *part,
						 Diagram *diagram,
						 bool own_bridge,
						 const QList<BridgeTerminal> &terminals = QList<BridgeTerminal>());
		~AddBridgeCommand() override = default;

		void undo() override;
		void redo() override;

	private:
		QETProject *project() const;

	private:
			///The bridge the line belongs to, a pointer: the command which
			///brought it into being is the one which takes it away again,
			///and QPointer goes null rather than following freed memory
		QPointer<Bridge> m_bridge;
		///The line itself, owned by the folio once it is drawn
		QPointer<BridgePart> m_part;
		QPointer<Diagram> m_diagram;
		///Which section of the bridge this line is
		BridgePartData m_part_data;
		///The terminals this line gives to the bridge: redo adds them
		///again, undo takes them away
		QList<BridgeTerminal> m_terminals;
		///True when this command brought the bridge into existence, so it
		///is the one which has to take it away again
		bool m_own_bridge = false;
};

#endif // ADDBRIDGECOMMAND_H
