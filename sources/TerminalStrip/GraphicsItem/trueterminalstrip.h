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
#include <TerminalStrip/connectorpinplan.h>

class Element;
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

		/// The pin plan of a connector drawn as a device (a head with its
		/// pins): the rows of ConnectorPinPlan::ofDevice
	class DevicePinPlanStrip : public AbstractTerminalStripInterface
	{
		public:
			DevicePinPlanStrip(Element *head);

			QString installation() const override;
			QString location() const override;
			QString name() const override;
			QVector<QSharedPointer<AbstractPhysicalTerminalInterface>> physicalTerminal() const override;
				///The plan is worked out again at the next call: what is drawn changed
			void refresh();

		private:
			const ConnectorPinPlan::Plan &plan() const;

			QPointer<Element> m_head;
			mutable ConnectorPinPlan::Plan m_plan;
			mutable bool m_plan_valid { false };
			mutable bool m_working { false };
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
            QVector<AbstractBridgeInterface *> bridges() const override;
			QString xref() const override;
			ElementData::TerminalType type() const override;
			bool isLed() const override;
			QString cable() const override;
			QString cableWire() const override;
			QStringList cableCoreTargets(const QString &name, const QUuid &uuid,
										 int slot) const override;
			bool isShield() const override;
			QVector<QString> cableTarget() const override;
			QVector<Wire> wires() const override;
			bool hasWire() const override;
			QColor color() const override;
			QColor stripeColor() const override;
			qreal widthMm() const override;
			QString catalogEntry() const override;
			bool hasSeparatorAfter() const override;

		private:
			QSharedPointer<RealTerminal> m_real;
	};

		/// A row of a pin plan which is no terminal of the strip: a spare pin
		/// of a connector's device type, or a pin read from a symbol
	class PinPlanRowTerminal : public AbstractRealTerminalInterface
	{
		public:
			PinPlanRowTerminal(const ConnectorPinPlan::Row &row);
			QString label() const override;
			bool isBridged() const override;
			AbstractBridgeInterface* bridge() const override;
			QString xref() const override;
			QVector<Wire> wires() const override;
			bool hasWire() const override;

		private:
			ConnectorPinPlan::Row m_row;
	};

	class PinPlanRowPhysicalTerminal : public AbstractPhysicalTerminalInterface
	{
		public:
			PinPlanRowPhysicalTerminal(const ConnectorPinPlan::Row &row);
			QVector<QSharedPointer<AbstractRealTerminalInterface>> realTerminals() const override;

		private:
			ConnectorPinPlan::Row m_row;
	};

	class TrueBridge : public AbstractBridgeInterface
	{
		public:
			TrueBridge(QSharedPointer<TerminalStripBridge> bridge);
			QUuid uuid() const override;
			int track() const override;

		private:
			QSharedPointer<TerminalStripBridge> m_bridge;
	};
}

#endif // TRUETERMINALSTRIP_H
