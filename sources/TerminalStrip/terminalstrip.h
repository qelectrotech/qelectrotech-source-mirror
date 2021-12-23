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

/**
 * @brief The RealTerminal class
 * Represent a real terminal.
 * A real terminal can be a drawed terminal in a folio
 * or a terminal set by user but not present
 * on any folio (for example a reserved terminal).
 *
 * When create a new instance of RealTerminal you must
 * call sharedRef() and only use the returned QSharedPointer
 * instead of the raw pointer
 */
class RealTerminal
{
		friend class TerminalStrip;
		friend class PhysicalTerminal;

	private:
		RealTerminal(TerminalStrip *strip, Element *element = nullptr);
		QSharedPointer<RealTerminal> sharedRef();
		QWeakPointer<RealTerminal> weakRef();

		bool fromXml(QDomElement xml_element, const QVector<TerminalElement *> &terminal_vector);
		QDomElement toXml(QDomDocument &parent_document) const;

	public:
		TerminalStrip *parentStrip() const;
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

		QSharedPointer<TerminalStripBridge> bridge() const;

		Element* element() const;
		QUuid elementUuid() const;
		QUuid uuid() const;

		static QString xmlTagName();

	private :
		QPointer<Element> m_element;
		QPointer<TerminalStrip> m_parent_terminal_strip;
		QUuid m_uuid = QUuid::createUuid();
		QWeakPointer<RealTerminal> m_this_weak;
};

class PhysicalTerminal
{
		friend class TerminalStrip;

	private:
		PhysicalTerminal(TerminalStrip *parent_strip, QVector<QSharedPointer<RealTerminal>> terminals);
		QSharedPointer<PhysicalTerminal> sharedRef();
		QWeakPointer<PhysicalTerminal> weakRef();

		QDomElement toXml(QDomDocument &parent_document) const;

		void setTerminals(const QVector<QSharedPointer<RealTerminal>> &terminals);
		void addTerminal(const QSharedPointer<RealTerminal> &terminal);
		bool removeTerminal(const QSharedPointer<RealTerminal> &terminal);

		bool setLevelOf(const QSharedPointer<RealTerminal> &terminal, int level);

	public:
		PhysicalTerminal(){}

		int levelCount() const;
		int levelOf(const QWeakPointer<RealTerminal> &terminal) const;
		QVector<QWeakPointer<RealTerminal>> realTerminals() const;
		QUuid uuid() const;
		int pos() const;
		int realTerminalCount() const;

		static QString xmlTagName();

	private:
		QPointer<TerminalStrip> m_parent_terminal_strip;
		QVector<QSharedPointer<RealTerminal>> m_real_terminal;
		QUuid m_uuid = QUuid::createUuid();
		QWeakPointer<PhysicalTerminal> m_this_weak;
};

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
		QWeakPointer<PhysicalTerminal> physicalTerminal(int index) const;
		QWeakPointer<PhysicalTerminal> physicalTerminal (const QWeakPointer<RealTerminal> &real_terminal) const;
		QVector<QWeakPointer<PhysicalTerminal>> physicalTerminal() const;

		bool setOrderTo(const QVector<QWeakPointer<PhysicalTerminal>> &sorted_vector);
		bool groupTerminals(const QWeakPointer<PhysicalTerminal> &receiver_terminal, const QVector<QWeakPointer<RealTerminal>> &added_terminals);
		void unGroupTerminals(const QVector<QWeakPointer<RealTerminal>> &terminals_to_ungroup);
		bool setLevel(const QWeakPointer<RealTerminal> &real_terminal, int level);

		bool isBridgeable(const QVector<QWeakPointer<RealTerminal>> &real_terminals) const;
		bool setBridge(const QVector<QWeakPointer<RealTerminal>> &real_terminals);
		bool setBridge(const QSharedPointer<TerminalStripBridge> &bridge, const QVector<QWeakPointer<RealTerminal>> &real_terminals);
		void unBridge(const QVector<QWeakPointer<RealTerminal>> &real_terminals);
		bool canUnBridge(const QVector <QWeakPointer<RealTerminal>> &real_terminals) const;
		QSharedPointer<TerminalStripBridge> bridgeFor(const QWeakPointer<RealTerminal> &real_terminal) const;

		QWeakPointer<RealTerminal> previousTerminalInLevel(const QWeakPointer<RealTerminal> &real_terminal) const;
		QWeakPointer<RealTerminal> nextTerminalInLevel(const QWeakPointer<RealTerminal> &real_terminal) const;
		QWeakPointer<RealTerminal> previousRealTerminal(const QWeakPointer<RealTerminal> &real_terminal) const;
		QWeakPointer<RealTerminal> nextRealTerminal(const QWeakPointer<RealTerminal> &real_terminal) const;

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
