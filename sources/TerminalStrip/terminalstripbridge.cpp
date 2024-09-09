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
#include "terminalstripbridge.h"
#include "realterminal.h"
#include "terminalstrip.h"
#include "../qetxml.h"

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
 * @brief TerminalStripBridge::toXml
 * Save this bridge into a QDomElement and return it.
 * @param parent_document
 * @return
 */
QDomElement TerminalStripBridge::toXml(QDomDocument &parent_document) const
{
	auto root_elmt = parent_document.createElement(xmlTagName());

	root_elmt.setAttribute(QStringLiteral("uuid"), m_uuid.toString());
	root_elmt.setAttribute(QStringLiteral("color"), m_color.name());

	auto terminals_elmt = parent_document.createElement(QStringLiteral("real_terminals"));
	for (const auto &real_t : std::as_const(m_real_terminals))
	{
		if (real_t)
		{
			auto terminal_elmt = parent_document.createElement(QStringLiteral("real_terminal"));
			terminal_elmt.setAttribute(QStringLiteral("uuid"), real_t->elementUuid().toString());
			terminals_elmt.appendChild(terminal_elmt);
		}
	}

	root_elmt.appendChild(terminals_elmt);
	return root_elmt;
}

void TerminalStripBridge::fromXml(const QDomElement &dom_element)
{
	if (dom_element.tagName() != xmlTagName() || !m_strip) {
		return;
	}

	m_uuid = QUuid(dom_element.attribute(QStringLiteral("uuid"), m_uuid.toString()));
	m_color = QColor::fromString(dom_element.attribute(QStringLiteral("color")));

	const auto real_t_vector = QETXML::subChild(dom_element,
										 QStringLiteral("real_terminals"),
										 QStringLiteral("real_terminal"));
	for (const auto &xml_real_t : real_t_vector)
	{
		auto real_t = m_strip->realTerminalForUuid(QUuid(xml_real_t.attribute(QStringLiteral("uuid"))));
		if (real_t) {
			m_real_terminals.append(real_t);
		}
	}
}

/**
 * @brief TerminalStripBridge::uuid
 * @return The uuid of this terminal
 */
QUuid TerminalStripBridge::uuid() const noexcept {
	return m_uuid;
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

void TerminalStripBridge::removeTerminal(const QSharedPointer<RealTerminal> &real_terminal) {
	m_real_terminals.removeOne(real_terminal);
}
