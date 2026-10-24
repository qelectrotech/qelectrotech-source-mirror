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
#ifndef BRIDGEUNDOCOMMANDS_H
#define BRIDGEUNDOCOMMANDS_H

#include "bridge.h"

#include <QColor>
#include <QHash>
#include <QLineF>
#include <QList>
#include <QPointer>
#include <QUndoCommand>

class BridgePart;
class Diagram;
class QETProject;

/**
	@brief What the window of a bridge changed: its colour, and how many
	terminals it may hold. Both halves are one step, so one undo puts
	back the bridge exactly as it was.
*/
class ChangeBridgePropertiesCommand : public QUndoCommand
{
	public:
		ChangeBridgePropertiesCommand(Bridge *bridge,
									  const QColor &color,
									  int max_connections,
									  qreal line_width,
									  QUndoCommand *parent = nullptr);
		void undo() override;
		void redo() override;

	private:
		QPointer<Bridge> m_bridge;
		QColor m_color_before;
		QColor m_color_after;
		int m_max_before = 0;
		int m_max_after = 0;
		qreal m_width_before = 1.0;
		qreal m_width_after = 1.0;
};

/**
	@brief A line of a bridge moved: its ends, and with them which
	terminals the bridge lands on.

	The geometry and the terminals it crosses belong together: a line
	lengthened over another terminal gives the bridge that terminal, and
	shortened it drops it again. What the bridge carries as information
	follows with them. One step either way, so one undo puts the line and
	its connections back exactly as they were.
*/
class ChangeBridgeGeometryCommand : public QUndoCommand
{
	public:
		ChangeBridgeGeometryCommand(Bridge *bridge,
									BridgePart *part,
									const QLineF &line_before,
									const QLineF &line_after,
									const QList<BridgeTerminal> &terminals_before,
									const QList<BridgeTerminal> &terminals_after,
									const BridgeInformation &information_before,
									const BridgeInformation &information_after,
									QUndoCommand *parent = nullptr);
		void undo() override;
		void redo() override;

	private:
		void apply(const QLineF &line,
				   const QList<BridgeTerminal> &terminals,
				   const BridgeInformation &information);

	private:
		QPointer<Bridge> m_bridge;
		QPointer<BridgePart> m_part;
		QUuid m_part_uuid;
		QLineF m_line_before;
		QLineF m_line_after;
		QList<BridgeTerminal> m_terminals_before;
		QList<BridgeTerminal> m_terminals_after;
		BridgeInformation m_information_before;
		BridgeInformation m_information_after;
};

/**
	@brief Which terminals a bridge lands on changed: a terminal moved
	onto one of its lines joins it, one moved off leaves it. The bridge
	keeps its colour and its limit through it all.
*/
class ChangeBridgeTerminalsCommand : public QUndoCommand
{
	public:
		ChangeBridgeTerminalsCommand(Bridge *bridge,
									 const QList<BridgeTerminal> &before,
									 const QList<BridgeTerminal> &after,
									 const BridgeInformation &information_before,
									 const BridgeInformation &information_after,
									 QUndoCommand *parent = nullptr);
		void undo() override;
		void redo() override;

	private:
		QPointer<Bridge> m_bridge;
		QList<BridgeTerminal> m_terminals_before;
		QList<BridgeTerminal> m_terminals_after;
		BridgeInformation m_information_before;
		BridgeInformation m_information_after;
};

/**
	@brief Take the drawn lines of a bridge off the drawing.

	Only the lines which were selected go: a bridge with other lines
	left goes on with them. When the last line of a bridge goes, the
	bridge goes with it -- what draws nothing is no bridge of this
	project -- and one undo puts both back where they stood.
*/
class RemoveBridgeCommand : public QUndoCommand
{
	public:
		RemoveBridgeCommand(const QList<BridgePart *> &parts);
		~RemoveBridgeCommand() override = default;

		void undo() override;
		void redo() override;

	private:
		QETProject *project() const;

	private:
		struct Entry
		{
				///The line as it is drawn on the folio
			QPointer<BridgePart> part;
				///The folio it stands on
			QPointer<Diagram> folio;
				///Where the bridge holds that section
			BridgePartData data;
				///The bridge the section belongs to
			QPointer<Bridge> bridge;
		};

		QList<Entry> m_entries;
			///What each bridge lands on before and after the lines go:
			///what only those lines crossed goes with them
		QHash<Bridge *, QList<BridgeTerminal>> m_terminals_before;
		QHash<Bridge *, QList<BridgeTerminal>> m_terminals_after;
			///Every bridge which loses its last line here: it goes with it
		QList<QPointer<Bridge>> m_bridges;
};

#endif // BRIDGEUNDOCOMMANDS_H
