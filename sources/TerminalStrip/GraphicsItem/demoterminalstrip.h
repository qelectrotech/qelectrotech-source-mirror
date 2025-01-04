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
#ifndef DEMOTERMINALSTRIP_H
#define DEMOTERMINALSTRIP_H

#include "terminalstripdrawer.h"

namespace TerminalStripDrawer {

class DemoTerminalStrip : public AbstractTerminalStripInterface
{
	public:
		DemoTerminalStrip();

		QString installation() const override {
			return QStringLiteral("=INST");
		}
		QString location() const override {
			return QStringLiteral("+LOC" );
		}
		QString name() const override {
			return QStringLiteral("X1");
		}
		QVector<QSharedPointer<AbstractPhysicalTerminalInterface>> physicalTerminal() const override;

	private:
		void build();

	private:
		QVector<QSharedPointer<AbstractPhysicalTerminalInterface>> m_physical_terminal;
};

} //End namespace TerminalStripDrawer

#endif // DEMOTERMINALSTRIP_H
