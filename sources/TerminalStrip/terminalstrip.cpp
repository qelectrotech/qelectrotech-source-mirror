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
#include "terminalstrip.h"
#include "../qetproject.h"
#include "../qetgraphicsitem/element.h"

using shared_real_terminal     = QSharedPointer<RealTerminal>;
using shared_physical_terminal = QSharedPointer<PhysicalTerminal>;


/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

/**
 * @brief The RealTerminal class
 * Represent a real terminal.
 * A real terminal can be a drawed terminal in a folio
 * or a terminal set by user but not present
 * on any folio (for example a reserved terminal).
 */
class RealTerminal
{
	public :

		/**
		 * @brief RealTerminal
		 * @param parent_strip : parent terminal strip
		 * @param terminal : terminal element (if any) in a folio
		 */
		RealTerminal(TerminalStrip *parent_strip, Element *terminal = nullptr) :
			m_element(terminal),
			m_parent_terminal_strip(parent_strip)
		{}

		/**
		 * @brief isElement
		 * @return true if this real terminal is linked to a terminal element
		 */
		bool isElement() const {
			return m_element.isNull() ? false : true;
		}

		/**
		 * @brief element
		 * @return the element linked to this real terminal
		 * or nullptr if not linked to an Element.
		 */
		Element *element() const {
			return m_element.data();
		}

	private :
		QPointer<Element> m_element;
		QPointer<TerminalStrip> m_parent_terminal_strip;
};


/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/




/**
 * @brief The PhysicalTerminal class
 * Represent a physical terminal.
 * A physical terminal is composed a least by one real terminal.
 * When a physical terminal have more than one real terminal
 * that mean the physical terminal have levels (one by real terminal).
 * The index of terminals returned by the function terminals()
 * is the same as the real level of the physical terminal, the index are from back to front.
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
 */
class PhysicalTerminal
{
	public:
		/**
		 * @brief PhysicalTerminal
		 * @param parent_strip : Parent terminal strip
		 * @param terminals : A vector of real terminals
		 * who compose this physical terminal.
		 * \p terminals must have at least one terminal
		 */
		PhysicalTerminal(TerminalStrip *parent_strip,
						 QVector<shared_real_terminal> terminals) :
			m_parent_terminal_strip(parent_strip),
			m_real_terminal(terminals)
		{}

		/**
		 * @brief setTerminals
		 * Set the real terminal of this physical terminal
		 * the order of the terminal in \p terminals represent
		 * the level index.
		 * @param terminals
		 */
		void setTerminals(QVector<shared_real_terminal> terminals) {
			m_real_terminal = terminals;
		}

		/**
		 * @brief levelCount
		 * @return the number of level of this terminal
		 */
		int levelCount() const {
			return m_real_terminal.size();
		}

		/**
		 * @brief terminals
		 * @return A vector of real terminal who compose this physical terminal
		 */
		QVector<shared_real_terminal> terminals() const {
			return m_real_terminal;
		}

	private:
		QPointer<TerminalStrip> m_parent_terminal_strip;
		QVector<shared_real_terminal> m_real_terminal;
};


/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/



/**
 * @brief TerminalStrip::TerminalStrip
 * @param name
 * @param project
 */
TerminalStrip::TerminalStrip(const QString &name, QETProject *project) :
	QObject(project),
	m_name(name),
	m_project(project)
{}

TerminalStrip::TerminalStrip(const QString &installation, const QString &location, const QString &name, QETProject *project) :
	QObject(project),
	m_installation(installation),
	m_location(location),
	m_name(name),
	m_project(project)
{}

void TerminalStrip::setInstallation(const QString &installation) {
	m_installation = installation;
}

void TerminalStrip::setLocation(const QString &location) {
	m_location = location;
}

void TerminalStrip::setName(const QString &name) {
	m_name = name;
}

/**
 * @brief TerminalStrip::addTerminal
 * Add terminal to this terminal strip
 * @param terminal
 * @return true if the terminal was successfully added.
 * Return false, if terminal already exist.
 * Return false, if terminal is not a terminal element.
 */
bool TerminalStrip::addTerminal(Element *terminal)
{
	if (m_terminal_elements_vector.contains(terminal)) {
		return false;
	}
	if (terminal->elementData().m_type != ElementData::Terminale) {
		return false;
	}

		//Create the real terminal
	shared_real_terminal real_terminal(new RealTerminal(this, terminal));
	m_real_terminals.append(real_terminal);

		//Create a new single level physical terminal
	shared_physical_terminal physical_terminal(
				new PhysicalTerminal(this,
									 QVector<shared_real_terminal>{real_terminal}));

	m_physical_terminals.append(physical_terminal);

	return true;
}

/**
 * @brief TerminalStrip::removeTerminal
 * Remove terminal from this terminal strip
 * @param terminal
 * @return true if terminal was successfully removed
 */
bool TerminalStrip::removeTerminal(Element *terminal)
{
	if (auto real_terminal = realTerminal(terminal))
	{
		if (auto physical_terminal = physicalTerminal(real_terminal))
		{
			if (physical_terminal->levelCount() == 1) {
				m_physical_terminals.removeOne(physical_terminal);
			} else {
				auto v = physical_terminal->terminals();
				v.removeOne(real_terminal);
				physical_terminal->setTerminals(v);
			}
		}
		m_real_terminals.removeOne(real_terminal);

		return true;
	}

	return false;
}

/**
 * @brief TerminalStrip::haveTerminal
 * @param terminal
 * @return true if \p terminal belong to this strip
 */
bool TerminalStrip::haveTerminal(Element *terminal) {
	return m_terminal_elements_vector.contains(terminal);
}

/**
 * @brief TerminalStrip::realTerminal
 * @param terminal
 * @return the real terminal linked to \p terminal
 * the returned QSharedPointer can be null.
 */
QSharedPointer<RealTerminal> TerminalStrip::realTerminal(Element *terminal)
{
	shared_real_terminal rt;

	if (m_terminal_elements_vector.contains(terminal))
	{
		for (auto &real : qAsConst(m_real_terminals))
		{
			if (real->element() == terminal) {
				rt = real;
				break;
			}
		}
	}

	return rt;
}

/**
 * @brief TerminalStrip::physicalTerminal
 * @param terminal
 * @return the physical terminal linked to \p terminal.
 * The returned QSharedPointer can be null.
 */
QSharedPointer<PhysicalTerminal> TerminalStrip::physicalTerminal(QSharedPointer<RealTerminal> terminal)
{
	shared_physical_terminal pt;

	for (auto &physical : qAsConst(m_physical_terminals))
	{
		if (physical->terminals().contains(terminal))
		{
			pt = physical;
			break;
		}
	}

	return pt;
}
