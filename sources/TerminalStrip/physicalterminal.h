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
#ifndef PHYSICALTERMINAL_H
#define PHYSICALTERMINAL_H

#include <QSharedPointer>
#include <QDomElement>
#include <QPointer>
#include <QUuid>
#include <QVector>

class RealTerminal;
class TerminalStrip;

/**
 * @brief The PhysicalTerminal class
 * Represent a physical terminal.
 * A physical terminal is composed a least by one RealTerminal.
 * When a physical terminal have more than one real terminal
 * that mean the physical terminal have levels (one by real terminal).
 * The index of real terminals returned by the function terminals()
 * is the same as the real level of the real terminal, the index are from back to front.
 *
 * Example for a 3 levels terminal.
 * index 0 = back (mounting plate)
 * index 1 = middle
 * index 2 = front (electrical cabinet door)
 *
 *	m
 *	o	  _
 *	u	 | |
 *	n	 | | _
 *	t	 | || |
 *	i	 | || | _
 *	n	 | || || |   d
 *	g	 |0||1||2|   o
 *		 | || ||_|   o
 *	p	 | || |      r
 *	l	 | ||_|
 *	a	 | |
 *	t	 |_|
 *	e
 *
 *
 */
class PhysicalTerminal
{
		friend class TerminalStrip;
		friend class RealTerminal;

	private:
		PhysicalTerminal(TerminalStrip *parent_strip, QVector<QSharedPointer<RealTerminal>> terminals);
		QSharedPointer<PhysicalTerminal> sharedRef();
		QWeakPointer<PhysicalTerminal> weakRef();

		QDomElement toXml(QDomDocument &parent_document) const;

		void setTerminals(const QVector<QSharedPointer<RealTerminal>> &terminals);
		void addTerminal(const QSharedPointer<RealTerminal> &terminal);
		bool removeTerminal(const QSharedPointer<RealTerminal> &terminal);

		bool setLevelOf(const QSharedPointer<RealTerminal> &terminal, int level);

		void setParentStrip(TerminalStrip *strip);

	public:
		PhysicalTerminal(){}
		~PhysicalTerminal();

		TerminalStrip* terminalStrip() const;
		int levelCount() const;
		int levelOf(const QSharedPointer<RealTerminal> &terminal) const;
		QVector<QSharedPointer<RealTerminal>> realTerminals() const;
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

#endif // PHYSICALTERMINAL_H
