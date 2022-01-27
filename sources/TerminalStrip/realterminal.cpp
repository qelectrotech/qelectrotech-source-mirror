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
#include "realterminal.h"
#include "terminalstrip.h"
#include "../qetgraphicsitem/terminalelement.h"
#include "physicalterminal.h"

/**
 * @brief RealTerminal
 * @param parent_strip : parent terminal strip
 * @param terminal : terminal element (if any) in a folio
 */
RealTerminal::RealTerminal(TerminalStrip *parent_strip,
						   Element *terminal) :
	m_element(terminal),
	m_parent_terminal_strip(parent_strip)
{}

/**
 * @brief RealTerminal::sharedRef
 * @return a QSharedPointer of this
 */
QSharedPointer<RealTerminal> RealTerminal::sharedRef()
{
	QSharedPointer<RealTerminal> this_shared(this->weakRef());
	if (this_shared.isNull())
	{
		this_shared = QSharedPointer<RealTerminal>(this);
		m_this_weak = this_shared.toWeakRef();
	}

	return this_shared;
}

/**
 * @brief RealTerminal::weakRef
 * @return a QWeakPointer of this, weak pointer can be bull
 */
QWeakPointer<RealTerminal> RealTerminal::weakRef() {
	return m_this_weak;
}

/**
 * @brief fromXml
 * @param xml_element
 * @return
 */
bool RealTerminal::fromXml(QDomElement xml_element, const QVector<TerminalElement *> &terminal_vector)
{
	if (xml_element.tagName() != xmlTagName()) {
		return true;
	}

	auto is_draw = xml_element.attribute(QStringLiteral("is_draw")) == QLatin1String("true")
				   ? true : false;

	QUuid uuid_(xml_element.attribute(QStringLiteral("uuid")));

	if (is_draw) {
		for (auto terminal : terminal_vector) {
			if (terminal->uuid() == uuid_)
			{
				m_element = terminal;
				break;
			}
		}
	} else {
		m_uuid = uuid_;
	}

	return true;
}

/**
 * @brief toXml
 * @param parent_document
 * @return this real terminal to xml
 */
QDomElement RealTerminal::toXml(QDomDocument &parent_document) const
{
	auto root_elmt = parent_document.createElement(this->xmlTagName());
	root_elmt.setAttribute("is_draw", m_element ? "true" : "false");
	root_elmt.setAttribute("uuid", m_element ? m_element->uuid().toString() :
											   m_uuid.toString());

	return root_elmt;
}

/**
		 * @brief parentStrip
		 * @return parent terminal strip
		 */
TerminalStrip *RealTerminal::parentStrip() const {
	return m_parent_terminal_strip.data();
}

/**
 * @brief RealTerminal::level
 * @return
 */
int RealTerminal::level() const
{
	if (m_parent_terminal_strip) {
		const auto phy_t = m_parent_terminal_strip->physicalTerminal(m_this_weak);
		if (phy_t) {
			return phy_t->levelOf(m_this_weak);
		}
	}

	return -1;
}

/**
 * @brief label
 * @return the label of this real terminal
 */
QString RealTerminal::label() const {
	if (!m_element.isNull()) {
		return m_element->actualLabel();
	} else {
		return QLatin1String();
	}
}

/**
 * @brief RealTerminal::Xref
 * @return Conveniant method to get the XRef
 * formated to string
 */
QString RealTerminal::Xref() const
{
	if (!m_element.isNull()) {
		return autonum::AssignVariables::genericXref(m_element.data());
	} else {
		return QString();
	}
}

/**
 * @brief RealTerminal::cable
 * @return
 */
QString RealTerminal::cable() const {
	return QString();
}

/**
 * @brief RealTerminal::cableWire
 * @return
 */
QString RealTerminal::cableWire() const {
	return QString();
}

/**
 * @brief RealTerminal::conductor
 * @return
 */
QString RealTerminal::conductor() const {
	return QString();
}

/**
 * @brief RealTerminal::type
 * @return
 */
ElementData::TerminalType RealTerminal::type() const {
	if (m_element) {
		return m_element->elementData().terminalType();
	} else {
		return ElementData::TTGeneric;
	}
}

/**
 * @brief RealTerminal::function
 * @return
 */
ElementData::TerminalFunction RealTerminal::function() const {
	if (m_element) {
		return m_element->elementData().terminalFunction();
	} else {
		return ElementData::TFGeneric;
	}
}

/**
 * @brief RealTerminal::isLed
 * @return
 */
bool RealTerminal::isLed() const {
	if (m_element) {
		return m_element->elementData().terminalLed();
	} else {
		return false;
	}
}

/**
 * @brief isElement
 * @return true if this real terminal is linked to a terminal element
 */
bool RealTerminal::isElement() const {
	return m_element.isNull() ? false : true;
}

/**
 * @brief RealTerminal::isBridged
 * @return true if is bridged.
 * @sa TerminalStrip::isBridged
 */
bool RealTerminal::isBridged() const
{
	if (m_parent_terminal_strip) {
		return !m_parent_terminal_strip->isBridged(m_this_weak.toStrongRef()).isNull();
	} else {
		return false;
	}
}

/**
 * @brief RealTerminal::bridge
 * @return
 */
QSharedPointer<TerminalStripBridge> RealTerminal::bridge() const
{
	if (m_parent_terminal_strip) {
		return m_parent_terminal_strip->isBridged(m_this_weak.toStrongRef());
	} else {
		return QSharedPointer<TerminalStripBridge>();
	}
}

/**
 * @brief element
 * @return the element linked to this real terminal
 * or nullptr if not linked to an Element.
 */
Element *RealTerminal::element() const {
	return m_element.data();
}

/**
 * @brief elementUuid
 * @return if this real terminal is an element
 * in a folio, return the uuid of the element
 * else return a null uuid.
 */
QUuid RealTerminal::elementUuid() const {
	if (!m_element.isNull()) {
		return m_element->uuid();
	} else {
		return QUuid();
	}
}

/**
 * @brief uuid
 * @return the uuid of this real terminal
 */
QUuid RealTerminal::uuid() const {
	return m_uuid;
}

/**
 * @brief RealTerminal::RealTerminal::xmlTagName
 * @return
 */
QString RealTerminal::RealTerminal::xmlTagName() {
	return QStringLiteral("real_terminal");
}
