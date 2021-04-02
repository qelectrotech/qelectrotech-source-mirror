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

class Element;
class RealTerminal;
class QETProject;
class PhysicalTerminal;

class TerminalStrip : public QObject
{
		Q_OBJECT
	public:
		TerminalStrip(const QString &name, QETProject *project);

		TerminalStrip(const QString &installation,
					  const QString &location,
					  const QString &name,
					  QETProject *project);

		void setInstallation(const QString &installation);
		void setLocation(const QString &location);
		void setName(const QString &name);

		bool addTerminal(Element *terminal);
		bool removeTerminal(Element *terminal);
		bool haveTerminal(Element *terminal);

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





#endif // TERMINALSTRIP_H
