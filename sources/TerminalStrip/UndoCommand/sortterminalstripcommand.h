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
#ifndef SORTTERMINALSTRIPCOMMAND_H
#define SORTTERMINALSTRIPCOMMAND_H

#include <QUndoCommand>
#include <QPointer>
#include <QVector>

class TerminalStrip;
class PhysicalTerminal;

/**
 * @brief The SortTerminalStripCommand class
 * An undo command use to sort the terminals element who
 * compose a terminal strip
 */
class SortTerminalStripCommand : public QUndoCommand
{
	public:
		SortTerminalStripCommand(TerminalStrip *strip, QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;

	private:
		void sort();

	private:
		QPointer<TerminalStrip> m_strip;
		QVector<QSharedPointer<PhysicalTerminal>> m_old_order,
												  m_new_order;
};

#endif // SORTTERMINALSTRIPCOMMAND_H
