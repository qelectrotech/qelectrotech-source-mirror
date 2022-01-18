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
#include "terminalstripbridge.h"
#include "realterminal.h"
#include "terminalstrip.h"

TerminalStripBridge::TerminalStripBridge(TerminalStrip *parent_strip) :
	m_strip(parent_strip)
{}

/**
 * @brief TerminalStripBridge::sharedRef
 * @return a QSharedPointer of this
 */
QSharedPointer<TerminalStripBridge> TerminalStripBridge::sharedRef()
{
	QSharedPointer<TerminalStripBridge> this_shared(this->weakRef());
	if (this_shared.isNull())
	{
		this_shared = QSharedPointer<TerminalStripBridge>(this);
		m_this_weak = this_shared.toWeakRef();
	}

	return this_shared;
}

/**
 * @brief TerminalStripBridge::weakRef
 * @return a QWeakPointer of this, weak pointer can be null
 */
QWeakPointer<TerminalStripBridge> TerminalStripBridge::weakRef() {
	return m_this_weak;
}

/**
 * @brief TerminalStripBridge::color
 * @return The color of this bridge
 */
QColor TerminalStripBridge::color() const {
	return m_color;
}

void TerminalStripBridge::setColor(const QColor &color) {
	m_color = color;
	if (m_strip) {
		m_strip->bridgeColorChanged(sharedRef());
	}
}

/**
 * @brief TerminalStripBridge::realTerminals
 * @return the real terminals who are bridged by this bridge
 */
QVector<QSharedPointer<RealTerminal> > TerminalStripBridge::realTerminals() const {
	return m_real_terminals;
}

/**
 * @brief TerminalStripBridge::addTerminals
 * @param real_terminals
 * @return Add terminals of @a real_terminals to this bridge.
 * If a terminal is already bridged by this bridge, the terminal is ignored.
 * If at least one terminal doesn't belong to the same strip of this bridge
 * this function do nothing and return false.
 */
bool TerminalStripBridge::addTerminals(const QVector<QSharedPointer<RealTerminal> > &real_terminals)
{
	QVector<QSharedPointer<RealTerminal>> to_append;
	for (const auto &real_t : real_terminals)
	{
		if (!real_t.isNull())
		{
			if (real_t->parentStrip() != m_strip) {
				return false;
			}
			if (!m_real_terminals.contains(real_t)) {
				to_append.append(real_t);
			}
		} else {
			return false;
		}
	}

	m_real_terminals.append(to_append);
	return true;
}

/**
 * @brief TerminalStripBridge::removeTerminals
 * @param real_terminal
 * Remove all real terminal of @real_terminals from this bridge.
 * This function doesn't make any check, they just remove if exist.
 * @sa TerminalStrip::canUnBridge
 */
void TerminalStripBridge::removeTerminals(const QVector<QSharedPointer<RealTerminal>> &real_terminals)
{
	for (const auto &real_t : real_terminals) {
		m_real_terminals.removeOne(real_t);
	}
}
