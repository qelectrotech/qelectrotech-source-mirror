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
#include "sortterminalstripcommand.h"
#include "../terminalstrip.h"
#include "../physicalterminal.h"
#include "../realterminal.h"
#include "../../utils/qetutils.h"

SortTerminalStripCommand::SortTerminalStripCommand(TerminalStrip *strip, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_strip(strip)
{
	setText(QObject::tr("Sort terminal block %1").arg(m_strip->name()));
	m_old_order = m_strip->physicalTerminal();
	m_new_order = m_strip->physicalTerminal();
	sort();
}

void SortTerminalStripCommand::undo()
{
	if (m_strip) {
		m_strip->setOrderTo(m_old_order);
	}
}

void SortTerminalStripCommand::redo()
{
	if (m_strip) {
		m_strip->setOrderTo(m_new_order);
	}
}

void SortTerminalStripCommand::sort()
{
	auto label_of = [](const QSharedPointer<PhysicalTerminal> &t) -> QString
	{
		return t->realTerminalCount() ? t->realTerminals().constLast()->label()
										: QString();
	};

	std::stable_sort(m_new_order.begin(), m_new_order.end(),
			[&label_of](const QSharedPointer<PhysicalTerminal> &arg1,
						const QSharedPointer<PhysicalTerminal> &arg2)
	{
		return QETUtils::naturalLessThan(label_of(arg1), label_of(arg2));
	});
}
