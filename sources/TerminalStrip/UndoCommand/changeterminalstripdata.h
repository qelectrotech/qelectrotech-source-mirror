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
#ifndef CHANGETERMINALSTRIPDATA_H
#define CHANGETERMINALSTRIPDATA_H

#include <QUndoCommand>
#include "../terminalstripdata.h"
#include "../terminalstrip.h"

/**
 * @brief The ChangeTerminalStripData class
 */
class ChangeTerminalStripData : public QUndoCommand
{
	public:
		ChangeTerminalStripData(TerminalStrip *strip, const TerminalStripData &data, QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;

	private:
		QPointer<TerminalStrip> m_strip;
		TerminalStripData m_old_data, m_new_data;
};

#endif // CHANGETERMINALSTRIPDATA_H
