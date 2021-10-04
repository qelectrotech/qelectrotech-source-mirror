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
#include "terminalstripdata.h"
#include "../properties/elementdata.h"

class Element;
class RealTerminal;
class QETProject;
class PhysicalTerminal;
class TerminalStripIndex;
class TerminalElement;



struct RealTerminalData
{
	int level_ = -1;

	QString label_,
			Xref_,
			cable_,
			cable_wire_,
			conductor_;

	QUuid uuid_;

	ElementData::TerminalType type_;
	ElementData::TerminalFunction function_;

	bool led_ = false,
		 is_element = false;

	QPointer<Element> element_;
};

struct PhysicalTerminalData
{
		QVector<RealTerminalData> real_terminals_vector;
		int pos_ = -1;
		QSharedPointer<PhysicalTerminal> physical_terminal;
};

/**
 * @brief The TerminalStrip class
 * This class hold all the datas and configurations
 * of a terminal strip (but the not the visual aspect).
 * A terminal strip have some informations (name comment etc...)
 * and is composed by terminals (draw in a diagram or described in the terminal strip)
 */
class TerminalStrip : public QObject
{
	friend class TerminalStripModel;

		Q_OBJECT
	public:
	signals:
		void orderChanged(); //Emitted when the order of the physical terminal is changed

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
		bool haveTerminal   (Element *terminal);

		int physicalTerminalCount() const;
		PhysicalTerminalData physicalTerminalData(int index) const;
		QVector<PhysicalTerminalData> physicalTerminalData() const;
		bool setOrderTo(QVector<PhysicalTerminalData> sorted_vector);
		bool groupTerminals(const PhysicalTerminalData &receiver_terminal, const QVector<PhysicalTerminalData> &added_terminals);
		void unGroupTerminals(const QVector<RealTerminalData> &terminals_to_ungroup);

		QVector<QPointer<Element>> terminalElement() const;

		static QString xmlTagName() {return QStringLiteral("terminal_strip");}
		QDomElement toXml(QDomDocument &parent_document);
		bool fromXml(QDomElement &xml_element);

		Element *elementForRealTerminal(QSharedPointer<RealTerminal> rt) const;

	private:
		QSharedPointer<RealTerminal> realTerminal(Element *terminal);
		QSharedPointer<PhysicalTerminal> physicalTerminal(QSharedPointer<RealTerminal> terminal) const;
		RealTerminalData realTerminalData(QSharedPointer<RealTerminal> real_terminal) const;

	private:
		TerminalStripData m_data;
		QPointer<QETProject> m_project;
		QVector<QPointer<Element>> m_terminal_elements_vector;
		QVector<QSharedPointer<RealTerminal>> m_real_terminals;
		QVector<QSharedPointer<PhysicalTerminal>> m_physical_terminals;
};

#endif // TERMINALSTRIP_H
