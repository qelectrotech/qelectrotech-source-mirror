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
#ifndef TERMINALSTRIP_H
#define TERMINALSTRIP_H

#include <QObject>
#include <QPointer>
#include <QColor>

#include "terminalstripdata.h"
#include "../properties/elementdata.h"

class Element;
class RealTerminal;
class QETProject;
class PhysicalTerminal;
class TerminalStripIndex;
class TerminalElement;
class TerminalStrip;
class TerminalStripBridge;

/**
 * @brief The TerminalStrip class
 * This class hold all the datas and configurations
 * of a terminal strip (but the not the visual aspect).
 * A terminal strip have some informations (name comment etc...)
 * and is composed by one or several PhysicalTerminal.
 */
class TerminalStrip : public QObject
{
	friend class TerminalStripModel;
	friend class RealTerminalData;

	Q_OBJECT

	public:
	signals:
		void orderChanged(); //Emitted when the order of the physical terminal is changed
		void bridgeChanged();
		void bridgeColorChanged(QSharedPointer<TerminalStripBridge> bridge);
	public:

		TerminalStrip(QETProject *project);

		TerminalStrip(const QString &installation,
					  const QString &location,
					  const QString &name,
					  QETProject *project);

		void setInstallation(const QString &installation);
		QString installation() const {return m_data.m_installation;}
		void setLocation(const QString &location);
		QString location() const {return m_data.m_location;}
		void setName(const QString &name);
		QString name() const {return m_data.m_name;}
		void setComment(const QString &comment);
		QString comment() const {return m_data.m_comment;}
		void setDescription(const QString &description);
		QString description() const {return m_data.m_description;}
		QUuid uuid() const {return m_data.m_uuid;}
		QETProject *project() const;

		TerminalStripData data() const;
		void setData(const TerminalStripData &data);

		bool addTerminal (QSharedPointer<RealTerminal> real_t);
		void addTerminals (QVector<QSharedPointer<RealTerminal>> real_t_vector);
		void addAndGroupTerminals (const QVector<QVector<QSharedPointer<RealTerminal>>> &real_t_vector);
		bool removeTerminal (QSharedPointer<RealTerminal> real_t);
		void removeTerminals (QVector<QSharedPointer<RealTerminal>> real_t_vector);
		bool addTerminal    (Element *terminal);
		bool removeTerminal (Element *terminal);
		bool addTerminal   (QSharedPointer<PhysicalTerminal> phy_t);
		void addTerminals (QVector<QSharedPointer<PhysicalTerminal>> phy_t_vector);
		bool removeTerminal (QSharedPointer<PhysicalTerminal> phy_t);
		void removeTerminals (QVector<QSharedPointer<PhysicalTerminal>> phy_t_vector);

		int pos(const QSharedPointer<PhysicalTerminal> &terminal) const;
		int physicalTerminalCount() const;
		QSharedPointer<PhysicalTerminal> physicalTerminal(int index) const;
		QSharedPointer<PhysicalTerminal> physicalTerminal(const QUuid &uuid) const;
		QVector<QSharedPointer<PhysicalTerminal>> physicalTerminal() const;
		QSharedPointer<RealTerminal> realTerminalForUuid(const QUuid &uuid) const;
		QVector<QSharedPointer<RealTerminal>> realTerminals() const;

		bool setOrderTo(const QVector<QSharedPointer<PhysicalTerminal>> &sorted_vector);
		bool groupTerminals(const QSharedPointer<PhysicalTerminal> &receiver_terminal, const QVector<QSharedPointer<RealTerminal>> &added_terminals);
		void unGroupTerminals(const QVector<QSharedPointer<RealTerminal>> &terminals_to_ungroup);
		bool setLevel(const QSharedPointer<RealTerminal> &real_terminal, int level);

		bool isBridgeable(const QVector<QSharedPointer<RealTerminal>> &real_terminals) const;
		bool isBridgeable(QSharedPointer<TerminalStripBridge> bridge, const QVector<QSharedPointer<RealTerminal>> &real_terminals) const;
		bool setBridge(const QVector<QSharedPointer<RealTerminal>> &real_terminals);
		bool setBridge(const QSharedPointer<TerminalStripBridge> &bridge, const QVector<QSharedPointer<RealTerminal>> &real_terminals);
		void unBridge(const QVector<QSharedPointer<RealTerminal>> &real_terminals);
		bool canUnBridge(const QVector <QSharedPointer<RealTerminal>> &real_terminals) const;
		QSharedPointer<TerminalStripBridge> isBridged(const QSharedPointer<RealTerminal> real_terminal) const;
		QSharedPointer<TerminalStripBridge> bridgeFor (const QVector<QSharedPointer<RealTerminal>> &terminal_vector) const;

		QSharedPointer<RealTerminal> previousTerminalInLevel(const QSharedPointer<RealTerminal> &real_terminal) const;
		QSharedPointer<RealTerminal> nextTerminalInLevel(const QSharedPointer<RealTerminal> &real_terminal) const;
		QSharedPointer<RealTerminal> previousRealTerminal(const QSharedPointer<RealTerminal> &real_terminal) const;
		QSharedPointer<RealTerminal> nextRealTerminal(const QSharedPointer<RealTerminal> &real_terminal) const;

		static QString xmlTagName() {return QStringLiteral("terminal_strip");}
		QDomElement toXml(QDomDocument &parent_document);
		bool fromXml(QDomElement &xml_element);

	private:
		TerminalStripData m_data;
		QPointer<QETProject> m_project;
		QVector<QSharedPointer<PhysicalTerminal>> m_physical_terminals;
		QVector<QSharedPointer<TerminalStripBridge>> m_bridge;
};

#endif // TERMINALSTRIP_H
