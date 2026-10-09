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
#include "demoterminalstrip.h"

namespace TerminalStripDrawer
{

/*========= DemoBridge =========*/
	class DemoBridge : public AbstractBridgeInterface
	{
		public:
			DemoBridge(const QUuid &uuid) :
				m_uuid { uuid } {}

			QUuid uuid() const override {
				return m_uuid;
			}

		private:
			const QUuid m_uuid;
	};

    class DemoRealTerminal : public AbstractRealTerminalInterface
    {
        public:
			DemoRealTerminal(const QString &label, const QString &xref, const QUuid &bridge,
							 ElementData::TerminalType type = ElementData::TTGeneric, bool led = false,
							 const QString &cable = QString(), const QString &wire = QString(), bool shield = false) :
				m_label { label },
				m_xref{ xref },
				m_bridge { bridge },
				m_type { type },
				m_led { led },
				m_cable { cable },
				m_wire { wire },
				m_shield { shield }
            {}

			QString label() const override {
				return m_label;
			}

			bool isBridged() const override {
				return true;
			}

			DemoBridge *bridge() const override {
				return new DemoBridge { m_bridge };
			}

			QString xref() const override {
				return m_xref;
			}

			ElementData::TerminalType type() const override {
				return m_type;
			}

			bool isLed() const override {
				return m_led;
			}

			QString cable() const override {
				return m_cable;
			}

			QString cableWire() const override {
				return m_wire;
			}

			bool isShield() const override {
				return m_shield;
			}

        private:
			QString m_label, m_xref;
            QUuid m_bridge;
			ElementData::TerminalType m_type;
			bool m_led;
			QString m_cable, m_wire;
			bool m_shield;
    };

	class DemoPhysicalTerminal : public AbstractPhysicalTerminalInterface
	{
		public:
			DemoPhysicalTerminal(QVector<QSharedPointer<AbstractRealTerminalInterface>> real_terminals) :
				m_real_terminals { real_terminals}
			{}

			QVector<QSharedPointer<AbstractRealTerminalInterface>> realTerminals() const override {
				return m_real_terminals;
			}

		private:
			QVector<QSharedPointer<AbstractRealTerminalInterface>> m_real_terminals;
	};



/*========= DemoTerminalStrip =========*/

	/**
	 * @brief DemoTerminalStrip::DemoTerminalStrip
	 */
	DemoTerminalStrip::DemoTerminalStrip()
	{
		build();
	}

	QVector<QSharedPointer<AbstractPhysicalTerminalInterface> > DemoTerminalStrip::physicalTerminal() const
	{
		return m_physical_terminal;
	}

	void DemoTerminalStrip::build()
	{
		QUuid lvl_1 = QUuid::createUuid();
		QUuid lvl_2 = QUuid::createUuid();
		QUuid lvl_3 = QUuid::createUuid();
		QUuid lvl_4 = QUuid::createUuid();

		QVector <QSharedPointer<AbstractRealTerminalInterface>> real_terminals_vector;

		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("24vdc"),
																									QStringLiteral("1_A1"),
																									lvl_1, ElementData::TTFuse, false, QStringLiteral("9W2"), QStringLiteral("1"), false)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("0vdc"),
																									QStringLiteral("1_A2"),
																									lvl_2, ElementData::TTSectional, false, QStringLiteral("9W2"), QStringLiteral("2"), false)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("signal"),
																									QStringLiteral("1_A3"),
																									lvl_3, ElementData::TTDiode, false, QStringLiteral("9W2"), QStringLiteral("3"), false)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("teach"),
																									QStringLiteral("1_A4"),
																									lvl_4, ElementData::TTGround, false, QStringLiteral("9W2"), QStringLiteral(""), true)};
		m_physical_terminal << QSharedPointer<AbstractPhysicalTerminalInterface> {
																				 new DemoPhysicalTerminal {real_terminals_vector}};

        real_terminals_vector.clear();
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("24vdc"),
																									QStringLiteral("2_A1"),
																									lvl_1, ElementData::TTGeneric, true)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("0vdc"),
																									QStringLiteral("2_A2"),
																									lvl_2, ElementData::TTFuse, true)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("signal"),
																									QStringLiteral("2_A3"),
																									lvl_3)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("teach"),
																									QStringLiteral("2_A4"),
																									lvl_4)};
		m_physical_terminal << QSharedPointer<AbstractPhysicalTerminalInterface> {
																				 new DemoPhysicalTerminal {real_terminals_vector}};


        real_terminals_vector.clear();
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("24vdc"),
																									QStringLiteral("3_A1"),
																									lvl_1)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("0vdc"),
																									QStringLiteral("3_A2"),
																									lvl_2)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("signal"),
																									QStringLiteral("3_A3"),
																									lvl_3)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("teach"),
																									QStringLiteral("3_A4"),
																									lvl_4)};
		m_physical_terminal << QSharedPointer<AbstractPhysicalTerminalInterface> {
																				 new DemoPhysicalTerminal {real_terminals_vector}};
    }
}
