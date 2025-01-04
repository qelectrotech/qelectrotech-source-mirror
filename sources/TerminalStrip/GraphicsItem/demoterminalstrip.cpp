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
			DemoRealTerminal(const QString &label, const QUuid &bridge) :
				m_label { label },
				m_bridge { bridge }
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

		private:
			QString m_label;
			QUuid m_bridge;
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
														lvl_1)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
								 new DemoRealTerminal( QStringLiteral("0vdc"),
														lvl_2)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
								 new DemoRealTerminal( QStringLiteral("signal"),
														lvl_3)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
								 new DemoRealTerminal( QStringLiteral("teach"),
														lvl_4)};
		m_physical_terminal << QSharedPointer<AbstractPhysicalTerminalInterface> {
							   new DemoPhysicalTerminal {real_terminals_vector}};

		real_terminals_vector.clear();
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
								 new DemoRealTerminal( QStringLiteral("24vdc"),
														lvl_1)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
								 new DemoRealTerminal( QStringLiteral("0vdc"),
														lvl_2)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
								 new DemoRealTerminal( QStringLiteral("signal"),
														lvl_3)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
								 new DemoRealTerminal( QStringLiteral("teach"),
														lvl_4)};
		m_physical_terminal << QSharedPointer<AbstractPhysicalTerminalInterface> {
							   new DemoPhysicalTerminal {real_terminals_vector}};

		real_terminals_vector.clear();
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
								 new DemoRealTerminal( QStringLiteral("24vdc"),
														lvl_1)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
								 new DemoRealTerminal( QStringLiteral("0vdc"),
														lvl_2)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
								 new DemoRealTerminal( QStringLiteral("signal"),
														lvl_3)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
								 new DemoRealTerminal( QStringLiteral("teach"),
														lvl_4)};
		m_physical_terminal << QSharedPointer<AbstractPhysicalTerminalInterface> {
							   new DemoPhysicalTerminal {real_terminals_vector}};
	}

}
