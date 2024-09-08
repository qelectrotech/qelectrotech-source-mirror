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
#include "physicalterminal.h"
#include "realterminal.h"
#include "terminalstrip.h"

/**
 * @brief PhysicalTerminal
 * @param parent_strip : Parent terminal strip
 * @param terminals : A vector of real terminals
 * who compose this physical terminal.
 * \p terminals must have at least one terminal
 */
PhysicalTerminal::PhysicalTerminal(TerminalStrip *parent_strip,
								   QVector<QSharedPointer<RealTerminal>> terminals) :
	m_parent_terminal_strip(parent_strip),
	m_real_terminal(terminals)
{
	for (const auto &real_t : m_real_terminal) {
		if (real_t) {
			real_t->setPhysicalTerminal(sharedRef());
		}
	}
}

/**
 * @brief PhysicalTerminal::sharedRef
 * @return a QSharedPointer of this
 */
QSharedPointer<PhysicalTerminal> PhysicalTerminal::sharedRef()
{
	QSharedPointer<PhysicalTerminal> this_shared(this->weakRef());
	if (this_shared.isNull())
	{
		this_shared = QSharedPointer<PhysicalTerminal>(this);
		m_this_weak = this_shared.toWeakRef();
	}

	return this_shared;
}

/**
 * @brief PhysicalTerminal::weakRef
 * @return a QWeakPointer of this, weak pointer can be null
 */
QWeakPointer<PhysicalTerminal> PhysicalTerminal::weakRef() {
	return m_this_weak;
}

/**
 * @brief toXml
 * @param parent_document
 * @return this physical terminal to xml
 */
QDomElement PhysicalTerminal::toXml(QDomDocument &parent_document) const
{
	auto root_elmt = parent_document.createElement(this->xmlTagName());
	for (auto &real_t : m_real_terminal) {
		root_elmt.appendChild(real_t->toXml(parent_document));
	}

	return root_elmt;
}

/**
 * @brief setTerminals
 * Set the RealTerminal who compose this physical terminal.
 * The position of the RealTerminal in @a terminals
 * represent the level of these in this physical terminal.
 * @param terminals
 */
void PhysicalTerminal::setTerminals(const QVector<QSharedPointer<RealTerminal>> &terminals) {
	m_real_terminal = terminals;
	for (const auto &real_t : m_real_terminal) {
		if (real_t) {
			real_t->setPhysicalTerminal(sharedRef());
		}
	}
}

/**
 * @brief addTerminals
 * Append the real terminal @a terminal
 * to this physical terminal.
 * @param terminal
 */
void PhysicalTerminal::addTerminal(const QSharedPointer<RealTerminal> &terminal) {
	m_real_terminal.append(terminal);
	terminal->setPhysicalTerminal(sharedRef());
}

/**
 * @brief removeTerminal
 * Remove @a terminal from the list of real terminal
 * @param terminal
 * @return true if successfully removed
 */
bool PhysicalTerminal::removeTerminal(const QSharedPointer<RealTerminal> &terminal)
{
	if (m_real_terminal.removeOne(terminal)) {
		terminal->setPhysicalTerminal(QSharedPointer<PhysicalTerminal>());
		return true;
	}
	return false;
}

/**
 * @brief setLevelOf
 * Change the level of \p terminal
 * @param terminal
 * @param level
 */
bool PhysicalTerminal::setLevelOf(const QSharedPointer<RealTerminal> &terminal, int level)
{
	const int i = m_real_terminal.indexOf(terminal);
	if (i >= 0)
	{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
		m_real_terminal.swapItemsAt(i, std::min((qsizetype)level, m_real_terminal.size()-1));
#else
		auto j = std::min(level, m_real_terminal.size()-1);
		std::swap(m_real_terminal.begin()[i], m_real_terminal.begin()[j]);
#endif
		return true;
	}
	return false;
}

void PhysicalTerminal::setParentStrip(TerminalStrip *strip)
{
	m_parent_terminal_strip = strip;
}

PhysicalTerminal::~PhysicalTerminal()
{
	for (const auto &real_t : m_real_terminal) {
		if (real_t) {
			real_t->setPhysicalTerminal(QSharedPointer<PhysicalTerminal>());
		}
	}
}

/**
 * @brief PhysicalTerminal::terminalStrip
 * @return The parent terminal strip ornullptr
 */
TerminalStrip *PhysicalTerminal::terminalStrip() const {
	return m_parent_terminal_strip.data();
}

/**
 * @brief levelCount
 * @return the number of level of this terminal
 */
int PhysicalTerminal::levelCount() const {
	return m_real_terminal.size();
}

/**
 * @brief levelOf
 * @param terminal
 * @return the level of real terminal \p terminal or
 * -1 if \terminal is not a part of this physicalTerminal
 */
int PhysicalTerminal::levelOf(const QSharedPointer<RealTerminal> &terminal) const {
	return m_real_terminal.indexOf(terminal);
}

/**
 * @brief terminals
 * @return A vector of RealTerminal who compose this PhysicalTerminal
 */
QVector<QSharedPointer<RealTerminal>> PhysicalTerminal::realTerminals() const {
	return m_real_terminal;
}

/**
 * @brief uuid
 * @return the uuid of this physical terminal
 */
QUuid PhysicalTerminal::uuid() const {
	return m_uuid;
}

int PhysicalTerminal::pos() const
{
	if (m_parent_terminal_strip) {
		return m_parent_terminal_strip->pos(m_this_weak);
	} else {
		return -1;
	}
}

int PhysicalTerminal::realTerminalCount() const {
	return m_real_terminal.size();
}

QString PhysicalTerminal::xmlTagName() {
	return QStringLiteral("physical_terminal");
}
