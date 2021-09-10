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
	QSharedPointer<RealTerminal> m_real_terminal;

	int pos_ = 0,
		level_ = 0;

	QString label_,
			Xref_,
			cable_,
			cable_wire_,
			conductor_;

	ElementData::TerminalType type_;

	bool led_ = false,
		 is_element = false;
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
		int realTerminalCount() const;
		TerminalStripIndex index(int index = 0);
		RealTerminalData realTerminalData(int real_terminal_index);

		QVector<QPointer<Element>> terminalElement() const;

		static QString xmlTagName() {return QStringLiteral("terminal_strip");}
		QDomElement toXml(QDomDocument &parent_document);
		bool fromXml(QDomElement &xml_element);

	private:
		QSharedPointer<RealTerminal> realTerminal(Element *terminal);
		QSharedPointer<PhysicalTerminal> physicalTerminal(QSharedPointer<RealTerminal> terminal);

	private:
		TerminalStripData m_data;
		QPointer<QETProject> m_project;
		QVector<QPointer<Element>> m_terminal_elements_vector;
		QVector<QSharedPointer<RealTerminal>> m_real_terminals;
		QVector<QSharedPointer<PhysicalTerminal>> m_physical_terminals;
};

class TerminalStripIndex
{
	friend class TerminalStrip;

	private :
		TerminalStripIndex () {}
		TerminalStripIndex (TerminalStripIndex *) {}

	public:
		bool isValid() const;
		QString label(int level = 0) const;
		QUuid uuid(int level = 0) const;
		bool isElement(int level = 0) const;
		TerminalElement *element(int level = 0) const;

	private:
		QVector<QString> m_label;
		QVector<QUuid> m_uuid;
		bool m_valid = false;
		QVector<bool> m_is_element;
		QVector<TerminalElement *> m_element;
};

#endif // TERMINALSTRIP_H
