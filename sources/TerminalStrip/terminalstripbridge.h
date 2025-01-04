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
#ifndef TERMINALSTRIPBRIDGE_H
#define TERMINALSTRIPBRIDGE_H

#include <QSharedPointer>
#include <QUuid>
#include <QPointer>
#include <QColor>
#include <QDomDocument>
#include <QVector>

class RealTerminal;
class TerminalStrip;

class TerminalStripBridge
{
		friend class TerminalStrip;

	public:
		static QVector<QColor> bridgeColor() {return QVector<QColor>{Qt::red, Qt::blue, Qt::white, Qt::darkGray, Qt::black};}

		TerminalStripBridge(TerminalStrip *parent_strip = nullptr);
		QSharedPointer<TerminalStripBridge> sharedRef();
		QWeakPointer<TerminalStripBridge> weakRef();

		QColor color() const;
		void setColor(const QColor &color);
		QVector<QSharedPointer<RealTerminal>> realTerminals() const;

		static QString xmlTagName() {return QStringLiteral("terminal_strip_bridge");}
		QDomElement toXml(QDomDocument &parent_document) const;
		void fromXml(const QDomElement &dom_element);
		QUuid uuid() const noexcept;

	private:
		bool addTerminals(const QVector<QSharedPointer<RealTerminal>> &real_terminals);
		void removeTerminals(const QVector<QSharedPointer<RealTerminal>> &real_terminals);
		void removeTerminal(const QSharedPointer<RealTerminal> &real_terminal);


	private:
		QPointer<TerminalStrip> m_strip;
		QVector<QSharedPointer<RealTerminal>> m_real_terminals;
		QColor m_color = Qt::darkGray;
		QUuid m_uuid = QUuid::createUuid();
		QWeakPointer<TerminalStripBridge> m_this_weak;
};



#endif // TERMINALSTRIPBRIDGE_H
