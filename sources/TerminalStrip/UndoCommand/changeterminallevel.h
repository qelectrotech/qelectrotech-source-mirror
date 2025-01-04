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
#ifndef CHANGETERMINALLEVEL_H
#define CHANGETERMINALLEVEL_H

#include <QUndoCommand>
#include <QPointer>
#include "../terminalstrip.h"

class ChangeTerminalLevel : public QUndoCommand
{
	public:
		ChangeTerminalLevel(TerminalStrip *strip,
							const QWeakPointer<RealTerminal> &real_terminal,
							int level,
							QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;

	private:
		QPointer<TerminalStrip> m_strip;
		QWeakPointer<RealTerminal> m_real_terminal;
		int m_new_level, m_old_level;
};

#endif // CHANGETERMINALLEVEL_H
