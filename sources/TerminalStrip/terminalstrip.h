/*
	Copyright 2006-2021 The QElectroTech Team
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

struct TerminalStripBridge
{
		QVector<QSharedPointer<RealTerminal>> real_terminals;
		QColor color_ = Qt::darkGray;
		QUuid uuid_ = QUuid::createUuid();
};

class RealTerminalData
{
		friend class TerminalStrip;
		friend class PhysicalTerminalData;
	private:
		RealTerminalData(QSharedPointer<RealTerminal> real_terminal);
		RealTerminalData(QWeakPointer<RealTerminal> real_terminal);

	public:
		RealTerminalData() {}

		bool isNull() const;
		int level() const;
		QString label() const;
		QString Xref() const;
		QString cable() const;
		QString cableWire() const;
		QString conductor() const;

		ElementData::TerminalType type() const;
		ElementData::TerminalFunction function() const;

		bool isLed() const;
		bool isElement() const;
		bool isBridged() const;

		Element* element() const;
		QUuid elementUuid() const;

		QSharedPointer<TerminalStripBridge> bridge() const;
		QWeakPointer<RealTerminal> realTerminal() const;

	private:
		QWeakPointer<RealTerminal> m_real_terminal;
};

/**
 * @brief The PhysicalTerminalData
 * Conveniant struct to quickly get some values
 * of a PhysicalTerminal
 */
class PhysicalTerminalData
{
		friend class TerminalStrip;

	private:
		PhysicalTerminalData(const TerminalStrip *strip, QSharedPointer<PhysicalTerminal> terminal);

	public:
		PhysicalTerminalData(){}

		bool isNull() const;
		int pos() const;
		QUuid uuid() const;
		int realTerminalCount() const;
		QVector<RealTerminalData> realTerminalDatas() const;
		QWeakPointer<PhysicalTerminal> physicalTerminal() const;

	private:
		QPointer<const TerminalStrip> m_strip;
		QWeakPointer<PhysicalTerminal> m_physical_terminal;
};

//Code to use PhysicalTerminalData as key for QHash
inline bool operator == (const PhysicalTerminalData &phy_1, const PhysicalTerminalData &phy_2) {
	return phy_1.uuid() == phy_2.uuid();
}

inline uint qHash(const PhysicalTerminalData &key, uint seed) {
	return qHash(key.uuid(), seed);
}

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
		static QVector<QColor> bridgeColor() {return QVector<QColor>{Qt::red, Qt::blue, Qt::white, Qt::darkGray, Qt::black};}

	signals:
		void orderChanged(); //Emitted when the order of the physical terminal is changed
		void bridgeChanged();

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

		TerminalStripData data() const;
		void setData(const TerminalStripData &data);

		bool addTerminal    (Element *terminal);
		bool removeTerminal (Element *terminal);

		int pos(const QWeakPointer<PhysicalTerminal> &terminal) const;
		int physicalTerminalCount() const;
		PhysicalTerminalData physicalTerminalData(int index) const;
		PhysicalTerminalData physicalTerminalData (const QWeakPointer<RealTerminal> &real_terminal) const;
		QVector<PhysicalTerminalData> physicalTerminalData() const;

		bool setOrderTo(const QVector<PhysicalTerminalData> &sorted_vector);
		bool groupTerminals(const PhysicalTerminalData &receiver_terminal, const QVector<QWeakPointer<RealTerminal>> &added_terminals);
		void unGroupTerminals(const QVector<QWeakPointer<RealTerminal>> &terminals_to_ungroup);
		bool setLevel(const QWeakPointer<RealTerminal> &real_terminal, int level);

		bool isBridgeable(const QVector<QWeakPointer<RealTerminal>> &real_terminals) const;
		bool setBridge(const QVector<QWeakPointer<RealTerminal>> &real_terminals);
		bool setBridge(const QSharedPointer<TerminalStripBridge> &bridge, const QVector<QWeakPointer<RealTerminal>> &real_terminals);
		void unBridge(const QVector<QWeakPointer<RealTerminal>> &real_terminals);
		bool canUnBridge(const QVector <QWeakPointer<RealTerminal>> &real_terminals) const;
		QSharedPointer<TerminalStripBridge> bridgeFor(const QWeakPointer<RealTerminal> &real_terminal) const;

		RealTerminalData previousTerminalInLevel(const QWeakPointer<RealTerminal> &real_terminal) const;
		RealTerminalData nextTerminalInLevel(const QWeakPointer<RealTerminal> &real_terminal) const;
		RealTerminalData previousRealTerminal(const QWeakPointer<RealTerminal> &real_terminal) const;
		RealTerminalData nextRealTerminal(const QWeakPointer<RealTerminal> &real_terminal) const;
		RealTerminalData realTerminalDataFor(const QWeakPointer<RealTerminal> &real_terminal) const;

		QVector<QPointer<Element>> terminalElement() const;

		static QString xmlTagName() {return QStringLiteral("terminal_strip");}
		QDomElement toXml(QDomDocument &parent_document);
		bool fromXml(QDomElement &xml_element);

	private:
		QSharedPointer<RealTerminal> realTerminal(Element *terminal);
		QSharedPointer<PhysicalTerminal> physicalTerminal(QSharedPointer<RealTerminal> terminal) const;
		QSharedPointer<TerminalStripBridge> isBridged(const QSharedPointer<RealTerminal> real_terminal) const;
		QSharedPointer<TerminalStripBridge> bridgeFor (const QVector<QSharedPointer<RealTerminal>> &terminal_vector) const;
		void rebuildRealVector();

	private:
		TerminalStripData m_data;
		QPointer<QETProject> m_project;
		QVector<QPointer<Element>> m_terminal_elements_vector;
		QVector<QSharedPointer<RealTerminal>> m_real_terminals;
		QVector<QSharedPointer<PhysicalTerminal>> m_physical_terminals;
		QVector<QSharedPointer<TerminalStripBridge>> m_bridge;
};

#endif // TERMINALSTRIP_H
