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
#ifndef TRUETERMINALSTRIP_H
#define TRUETERMINALSTRIP_H

#include "terminalstripdrawer.h"

class TerminalStrip;
class PhysicalTerminal;
class RealTerminal;
class TerminalStripBridge;

namespace TerminalStripDrawer
{
	class TrueTerminalStrip : public AbstractTerminalStripInterface
	{
		public:
			TrueTerminalStrip(TerminalStrip *strip);

			QString installation() const override;
			QString location() const override;
			QString name() const override;
			QVector<QSharedPointer<AbstractPhysicalTerminalInterface>> physicalTerminal() const override;

		private:
			QPointer<TerminalStrip> m_strip;
	};

	class TruePhysicalTerminal : public AbstractPhysicalTerminalInterface
	{
		public:
			TruePhysicalTerminal(QSharedPointer<PhysicalTerminal> physical);
			QVector<QSharedPointer<AbstractRealTerminalInterface>> realTerminals() const override;

		private:
			QSharedPointer<PhysicalTerminal> m_physical;
	};

    class TrueRealTerminal : public AbstractRealTerminalInterface
    {
        public:
            TrueRealTerminal(QSharedPointer<RealTerminal> real);
            QString label() const override;
            bool isBridged() const override;
            AbstractBridgeInterface* bridge() const override;
			QString xref() const override;

		private:
			QSharedPointer<RealTerminal> m_real;
	};

	class TrueBridge : public AbstractBridgeInterface
	{
		public:
			TrueBridge(QSharedPointer<TerminalStripBridge> bridge);
			QUuid uuid() const override;

		private:
			QSharedPointer<TerminalStripBridge> m_bridge;
	};
}

#endif // TRUETERMINALSTRIP_H
