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
#include "../qetgraphicsitem/terminalelement.h"
#include "../elementprovider.h"
#include "../qetxml.h"
#include "../autoNum/assignvariables.h"

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
 * @sa RealTerminalData
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
		 * @brief parentStrip
		 * @return parent terminal strip
		 */
		TerminalStrip *parentStrip() const {
			return m_parent_terminal_strip.data();
		}
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

		/**
		 * @brief label
		 * @return the label of this real terminal
		 */
		QString label() const {
			if (!m_element.isNull()) {
				return m_element->actualLabel();
			} else {
				return QLatin1String();
			}
		}

		ElementData::TerminalType type() const {
			if (m_element) {
				return m_element->elementData().terminalType();
			} else {
				return ElementData::TTGeneric;
			}
		}

		ElementData::TerminalFunction function() const {
			if (m_element) {
				return m_element->elementData().terminalFunction();
			} else {
				return ElementData::TFGeneric;
			}
		}

		bool led() const {
			if (m_element) {
				return m_element->elementData().terminalLed();
			} else {
				return false;
			}
		}

		/**
		 * @brief elementUuid
		 * @return if this real terminal is an element
		 * in a folio, return the uuid of the element
		 * else return a null uuid.
		 */
		QUuid elementUuid() const {
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
		QUuid uuid() const {
			return m_uuid;
		}

		static QString xmlTagName() {
			return QStringLiteral("real_terminal");
		}

		/**
		 * @brief toXml
		 * @param parent_document
		 * @return this real terminal to xml
		 */
		QDomElement toXml(QDomDocument &parent_document) const
		{
			auto root_elmt = parent_document.createElement(this->xmlTagName());
			root_elmt.setAttribute("is_draw", m_element ? "true" : "false");
			root_elmt.setAttribute("uuid", m_element ? m_element->uuid().toString() :
													   m_uuid.toString());

			return root_elmt;
		}

		/**
		 * @brief fromXml
		 * @param xml_element
		 * @return
		 */
		bool fromXml(QDomElement xml_element, const QVector<TerminalElement *> &terminal_vector)
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

	private :
		QPointer<Element> m_element;
		QPointer<TerminalStrip> m_parent_terminal_strip;
		QUuid m_uuid = QUuid::createUuid();
};


/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/




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
 *	@sa PhysicalTerminalData
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
		 * Set the RealTerminal who compose this physical terminal.
		 * The position of the RealTerminal in @a terminals
		 * represent the level of these in this physical terminal.
		 * @param terminals
		 */
		void setTerminals(QVector<shared_real_terminal> terminals) {
			m_real_terminal = terminals;
		}

		/**
		 * @brief addTerminals
		 * Append the real terminal @a terminal
		 * to this physical terminal.
		 * @param terminal
		 */
		void addTerminal(shared_real_terminal terminal) {
			m_real_terminal.append(terminal);
		}

		/**
		 * @brief removeTerminal
		 * Remove @a terminal from the list of real terminal
		 * @param terminal
		 * @return true if sucessfully removed
		 */
		bool removeTerminal(shared_real_terminal terminal) {
			return m_real_terminal.removeOne(terminal);
		}

		/**
		 * @brief levelCount
		 * @return the number of level of this terminal
		 */
		int levelCount() const {
			return m_real_terminal.size();
		}

		/**
		 * @brief levelOf
		 * @param terminal
		 * @return the level of real terminal \p terminal or
		 * -1 if \terminal is not a part of this physicalTerminal
		 */
		int levelOf(shared_real_terminal terminal) const {
			return m_real_terminal.indexOf(terminal);
		}

		/**
		 * @brief setLevelOf
		 * Change the level of \p terminal
		 * @param terminal
		 * @param level
		 */
		bool setLevelOf(shared_real_terminal terminal, int level)
		{
			const int i = m_real_terminal.indexOf(terminal);
			if (i >= 0)
			{
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
				m_real_terminal.swapItemsAt(i, std::min(level, m_real_terminal.size()-1));
#else
				auto j = std::min(level, m_real_terminal.size()-1);
				std::swap(m_real_terminal.begin()[i], m_real_terminal.begin()[j]);
#endif
				return true;
			}
			return false;
		}

		/**
		 * @brief terminals
		 * @return A vector of RealTerminal who compose this PhysicalTerminal
		 */
		QVector<shared_real_terminal> terminals() const {
			return m_real_terminal;
		}

		/**
		 * @brief uuid
		 * @return the uuid of this physical terminal
		 */
		QUuid uuid() const {
			return m_uuid;
		}

		static QString xmlTagName() {
			return QStringLiteral("physical_terminal");
		}

		/**
		 * @brief toXml
		 * @param parent_document
		 * @return this physical terminal to xml
		 */
		QDomElement toXml(QDomDocument &parent_document) const
		{
			auto root_elmt = parent_document.createElement(this->xmlTagName());
			for (auto &real_t : m_real_terminal) {
				root_elmt.appendChild(real_t->toXml(parent_document));
			}

			return root_elmt;
		}

	private:
		QPointer<TerminalStrip> m_parent_terminal_strip;
		QVector<shared_real_terminal> m_real_terminal;
		const QUuid m_uuid = QUuid::createUuid();
};


/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/

/**
 * @brief TerminalStrip::TerminalStrip
 * @param project
 */
TerminalStrip::TerminalStrip(QETProject *project) :
	QObject(project),
	m_project(project)
{}

/**
 * @brief TerminalStrip::TerminalStrip
 * @param installation
 * @param location
 * @param name
 * @param project
 */
TerminalStrip::TerminalStrip(const QString &installation, const QString &location, const QString &name, QETProject *project) :
	QObject(project),
	m_project(project)
{
	m_data.m_installation = installation;
	m_data.m_location     = location;
	m_data.m_name         = name;
}

void TerminalStrip::setInstallation(const QString &installation) {
	m_data.m_installation = installation;
}

void TerminalStrip::setLocation(const QString &location) {
	m_data.m_location = location;
}

void TerminalStrip::setName(const QString &name) {
	m_data.m_name = name;
}

void TerminalStrip::setComment(const QString &comment) {
	m_data.m_comment = comment;
}

void TerminalStrip::setDescription(const QString &description) {
	m_data.m_description = description;
}

/**
 * @brief TerminalStrip::data
 * @return The internal data of this strip
 */
TerminalStripData TerminalStrip::data() const {
	return m_data;
}

/**
 * @brief TerminalStrip::setData
 * Set the internal data of this strip to @a data.
 * the uuid of the new data is set to the uuid
 * of the previous data to keep the uuid
 * of the terminal strip unchanged
 * @param data
 */
void TerminalStrip::setData(const TerminalStripData &data) {
	m_data = data;
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

	m_terminal_elements_vector.append(terminal);

		//Create the real terminal
	const shared_real_terminal real_terminal(new RealTerminal(this, terminal));
	m_real_terminals.append(real_terminal);

		//Create a new single level physical terminal
	const shared_physical_terminal physical_terminal(
				new PhysicalTerminal(this,
									 QVector<shared_real_terminal>{real_terminal}));

	m_physical_terminals.append(physical_terminal);

	static_cast<TerminalElement *>(terminal)->setParentTerminalStrip(this);

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
	if (m_terminal_elements_vector.contains(terminal))
	{
		m_terminal_elements_vector.removeOne(terminal);

			//Get the real and physical terminal associated to @terminal
		if (auto real_terminal = realTerminal(terminal))
		{
			if (auto physical_terminal = physicalTerminal(real_terminal))
			{
				if (physical_terminal->levelCount() == 1)  {
					m_physical_terminals.removeOne(physical_terminal);
				} else {
					auto v = physical_terminal->terminals();
					v.removeOne(real_terminal);
					physical_terminal->setTerminals(v);
				}
			}
			m_real_terminals.removeOne(real_terminal);

			static_cast<TerminalElement *>(terminal)->setParentTerminalStrip(nullptr);

			return true;
		}

			//There is no reason to be here, but in case of....
		return false;
	}
	return false;
}

/**
 * @brief TerminalStrip::pos
 * @param terminal
 * @return the position of the physical terminal
 * or -1 if not found
 */
int TerminalStrip::pos(const QWeakPointer<PhysicalTerminal> &terminal) const {
	return m_physical_terminals.indexOf(terminal.toStrongRef());
}

/**
 * @brief TerminalStrip::physicalTerminalCount
 * @return the number of physical terminal.
 * A physical terminal is the representation of a real electrical terminal.
 * Notice that a physical terminal can have level (like in real life)
 */
int TerminalStrip::physicalTerminalCount() const {
	return m_physical_terminals.size();
}

/**
 * @brief TerminalStrip::physicalTerminalData
 * @param index
 * @return The data of the physical terminal at index \p index
 */
PhysicalTerminalData TerminalStrip::physicalTerminalData(int index) const
{
	if (index < m_physical_terminals.size()) {
		return PhysicalTerminalData(this, m_physical_terminals.at(index));
	} else {
		return PhysicalTerminalData();
	}
}

/**
 * @brief TerminalStrip::physicalTerminalData
 * @param real_terminal
 * @return the parent PhysicalTerminalData of \p real_terminal.
 * the PhysicalTerminalData can be invalid if \p real_terminal don't belong to this strip
 */
PhysicalTerminalData TerminalStrip::physicalTerminalData (const QWeakPointer<RealTerminal> &real_terminal) const
{
	const auto real_t = real_terminal.toStrongRef();
	if (real_t.isNull()) {
		return PhysicalTerminalData();
	}

	const auto phy_t = physicalTerminal(real_t);
	if (phy_t) {
		return PhysicalTerminalData(this, phy_t);
	}

	return PhysicalTerminalData();
}

/**
 * @brief TerminalStrip::physicalTerminalData
 * @return A vector of all physical terminal data owned by this terminal strip.
 * The order of the vector is the same as the order of the terminal in the strip
 */
QVector<PhysicalTerminalData> TerminalStrip::physicalTerminalData() const
{
	QVector<PhysicalTerminalData> v_;
	for (auto i = 0 ; i<physicalTerminalCount() ; ++i) {
		v_.append(physicalTerminalData(i));
	}

	return v_;
}

/**
 * @brief TerminalStrip::setSortedTo
 * Sort the physical terminal owned by this strip in the same order
 * as \p sorted_vector.
 * \p sorted_vector must contain exaclty the same physical terminal as this strip
 * else this function do nothing.
 *
 * To avoid any mistake, you should call TerminalStrip::physicalTerminalData()
 * sort the returned vector and call this function with sorted vector, then you are sure
 * the vector contain the same values, no more no less.
 *
 * @param sorted_vector
 * @return true is successfully sorted.
 */
bool TerminalStrip::setOrderTo(const QVector<PhysicalTerminalData> &sorted_vector)
{
	if (sorted_vector.size() != m_physical_terminals.size()) {
		return false;
	}

	QVector<QSharedPointer<PhysicalTerminal>> new_order;
	for (const auto &ptd : sorted_vector)
	{
		const auto physical_t = ptd.physicalTerminal().toStrongRef();
		if (physical_t.isNull()) {
			continue;
		}

		if (m_physical_terminals.contains(physical_t)) {
			new_order.append(physical_t);
		} else {
			return false;
		}
	}

	m_physical_terminals = new_order;
	rebuildRealVector();
	emit orderChanged();
	return true;
}

/**
 * @brief TerminalStrip::groupTerminal
 * Add \p added_terminal to \p receiver_terminal.
 * At the end of this method, if there is physical terminal
 * without any real terminal, they will be removed
 * and \p receiver_terminal become a multi-level terminal.
 * Emit the signal orderChanged();
 * @param added_terminal
 * @param receiver_terminal
 * @return true if success
 */
bool TerminalStrip::groupTerminals(const PhysicalTerminalData &receiver_terminal, const QVector<QWeakPointer<RealTerminal>> &added_terminals)
{
	const auto receiver_ = receiver_terminal.physicalTerminal().toStrongRef();
	if (receiver_.isNull()) {
		qDebug() << "TerminalStrip::groupTerminal : Arguments terminals don't belong to this strip. Operation aborted.";
		return false;
	}

	bool have_grouped = false;
	for (const auto &added : added_terminals)
	{
		const auto added_terminal = added.toStrongRef();

		if (added_terminal.isNull()) {
			continue;
		}

		auto physical_ = physicalTerminal(added_terminal);
		physical_->removeTerminal(added_terminal);

		receiver_->addTerminal(added_terminal);
		have_grouped = true;
	}

	if (have_grouped)
	{
		const auto vector_ = m_physical_terminals;
		for (const auto &phys : vector_) {
			if (phys->terminals().isEmpty()) {
				m_physical_terminals.removeOne(phys);
			}
		}

		emit orderChanged();
	}
	return true;
}

/**
 * @brief TerminalStrip::unGroupTerminals
 * Ungroup all real terminals of \p terminals_to_ungroup
 * @param terminals_to_ungroup
 */
void TerminalStrip::unGroupTerminals(const QVector<QWeakPointer<RealTerminal>> &terminals_to_ungroup)
{
	bool ungrouped = false;
	for (const auto &rt_ : terminals_to_ungroup)
	{
		if (auto real_terminal = rt_.toStrongRef()) //Get the shared real terminal
		{
			if (auto physical_terminal = physicalTerminal(real_terminal)) //Get the physical terminal
			{
				if (physical_terminal->terminals().size() > 1) //Check if physical have more than one real terminal
				{
					physical_terminal->removeTerminal(real_terminal);
					const shared_physical_terminal new_physical_terminal (
								new PhysicalTerminal(this, QVector<shared_real_terminal>{real_terminal}));

					m_physical_terminals.append(new_physical_terminal);
					ungrouped = true;
				}
			}
		}
	}

	if (ungrouped) {
		emit orderChanged();
	}
}

/**
 * @brief TerminalStrip::setLevel
 * @param real_terminal_data
 * @param level
 * @return
 */
bool TerminalStrip::setLevel(const QWeakPointer<RealTerminal> &real_terminal, int level)
{
	const auto real_t = real_terminal.toStrongRef();
	if (real_t)
	{
		auto physical_terminal = physicalTerminal(real_t);
		if (physical_terminal)
		{
			if (physical_terminal->terminals().size() > 1 &&
				physical_terminal->setLevelOf(real_t, level))
			{
				emit orderChanged();
				return true;
			}
		}
	}

	return false;
}

/**
 * @brief TerminalStrip::isBridgeable
 * Check if all realTerminal in @a real_terminals are bridgeable together.
 * To be bridgeable, each real terminal must belong to this terminal strip
 * be at the same level, be consecutive and not belong to the same physicalTerminal
 * and at least one terminal must be not bridged
 * @param real_terminals : a vector of realterminal
 * @return
 */
bool TerminalStrip::isBridgeable(const QVector<QWeakPointer<RealTerminal>> &real_terminals) const
{
	if (real_terminals.size() < 2) {
		return false;
	}

		// Check if first terminal belong to this strip
	const auto first_real_terminal = real_terminals.first().toStrongRef();
	if (!first_real_terminal) {
		return false;
	}

		// Get the level of the first terminal
	const auto rtd_ = realTerminalDataFor(real_terminals.first());
	const int level_ = rtd_.level();

		// Get the physical terminal and pos
	auto first_physical_terminal = physicalTerminal(first_real_terminal);
	QVector<shared_physical_terminal> physical_vector{first_physical_terminal};
	QVector<int> pos_vector{m_physical_terminals.indexOf(first_physical_terminal)};

	auto bridge_ = isBridged(first_real_terminal);
		//bool to know at the end of this function if at least one terminal is not bridged
	bool no_bridged = bridge_ ? false : true;

			// Check for each terminals
	for (int i=1 ; i<real_terminals.size() ; ++i)
	{
			// If belong to this strip
		const auto real_terminal = real_terminals.at(i).toStrongRef();
		if (!real_terminal) {
			return false;
		}

			// at the same level
		if (level_ != realTerminalDataFor(real_terminals.first()).level()) {
			return false;
		}

			// Not to the same physical terminal of a previous checked real terminal
		const auto physical_terminal = physicalTerminal(real_terminal);
		if (physical_vector.contains(physical_terminal)) {
			return false;
		} else {
			physical_vector.append(physical_terminal);
		}

			// Not in another bridge of a previous checked real terminal
		const auto checked_bridge = isBridged(real_terminal);
		if (checked_bridge)
		{
			if (bridge_.isNull()) {
				bridge_ = checked_bridge;
			} else if (checked_bridge != bridge_) {
				return false;
			}
		} else {
			no_bridged = true;
		}

		pos_vector.append(m_physical_terminals.indexOf(physical_terminal));
	}

		// Check if concecutive
	const auto count_ = pos_vector.size();
	const auto min_max = std::minmax_element(pos_vector.constBegin(), pos_vector.constEnd());
	if ((*min_max.second - *min_max.first) + 1 != count_) {
		return false;
	}

	return no_bridged;
}

/**
 * @brief TerminalStrip::setBridge
 * Set a bridge betwen all real terminal of @a real_terminals
 * @sa TerminalStrip::isBridgeable
 * @return true if bridge was successfully created
 */
bool TerminalStrip::setBridge(const QVector<QWeakPointer<RealTerminal>> &real_terminals)
{
	if (!isBridgeable(real_terminals)) {
		return false;
	}
	QVector<shared_real_terminal> real_terminals_vector;

	for (const auto &real_terminal : real_terminals)
	{
		const auto real_t = real_terminal.toStrongRef();
		if (real_t) {
			real_terminals_vector.append(real_t);
		}
	}

	auto bridge = bridgeFor(real_terminals_vector);
	if (bridge.isNull()) {
		bridge = QSharedPointer<TerminalStripBridge>::create();
		m_bridge.append(bridge);
	}

	for (const auto &real_t : qAsConst(real_terminals_vector))
	{
		if (!bridge->real_terminals.contains(real_t))
			bridge->real_terminals.append(real_t);
	}

	emit bridgeChanged();
	return true;
}

/**
 * @brief TerminalStrip::setBridge
 * Bridge the RealTerminal of @a real_terminals_data to @a bridge
 * @param bridge
 * @param real_terminals_data
 * @return true if all RealTerminal was successfully bridged
 */
bool TerminalStrip::setBridge(const QSharedPointer<TerminalStripBridge> &bridge, const QVector<QWeakPointer<RealTerminal>> &real_terminals)
{
	if (bridge)
	{
		if (!isBridgeable(real_terminals)) {
			return false;
		}

		bool b_ = false;
		for (const auto & rt_ : real_terminals)
		{
			const auto real_t = rt_.toStrongRef();
			if (real_t &&
				!bridge->real_terminals.contains(real_t))
			{
				bridge->real_terminals.append(real_t);
				b_ = true;
			}
		}

		if (b_) {
			emit bridgeChanged();
			return true;
		}
	}

	return false;
}

/**
 * @brief TerminalStrip::unBridge
 * Unbridge all real terminals of @a real_terminals
 * @sa TerminalStrip::canUnBridge
 * @param real_terminals
 */
void TerminalStrip::unBridge(const QVector<QWeakPointer<RealTerminal>> &real_terminals)
{
	if (canUnBridge(real_terminals))
	{
		auto bridge_ = isBridged(real_terminals.first().toStrongRef());
		for (const auto &real_t : qAsConst(real_terminals)) {
			bridge_->real_terminals.removeOne(real_t.toStrongRef());
		}

		emit bridgeChanged();
	}
}

/**
 * @brief TerminalStrip::canUnBridge
 * @param m_real_terminals
 * @return True if all terminals of @a real_terminals can be unbridged.
 * For this method return True, all terminals must be bridged together,
 * be consecutive and in an one or the both extremities of the bridge.
 */
bool TerminalStrip::canUnBridge(const QVector<QWeakPointer<RealTerminal> > &real_terminals) const
{
	if (real_terminals.isEmpty()) {
		return false;
	}

		//Get the bridge of first terminal
	const auto compar_bridge = isBridged(real_terminals.first().toStrongRef());
	if (compar_bridge)
	{
		QMap<int, QWeakPointer<RealTerminal>> sorted_terminal;

			//Check if all terminals are bridged and if it's the same bridge.
			//If true insert the terminal in sorted_terminal QMap
			//with for key the position of the parent physical terminal
		for (const auto &real_t : real_terminals) {
			if (compar_bridge != isBridged(real_t.toStrongRef())) {
				return false;
			} else {
				sorted_terminal.insert(m_physical_terminals.indexOf(physicalTerminal(real_t.toStrongRef())),
									   real_t);
			}
		}

			//Check if consecutive
		const auto count_ = sorted_terminal.size();
		const auto min_max = std::minmax_element(sorted_terminal.keyBegin(), sorted_terminal.keyEnd());
		if ((*min_max.second - *min_max.first) + 1 != count_) {
			return false;
		}

			//Check if first terminal is the begin of bridge
		const auto previous_real_t = previousTerminalInLevel(sorted_terminal.first());
		if (previous_real_t.isNull())
			return true;
		else {
			const auto previous_bridge = isBridged(previous_real_t.realTerminal());
			if (compar_bridge != previous_bridge) {
				return true;
			}
		}

			//Check if last terminal is the end of bridge
		const auto next_real_t = nextTerminalInLevel(sorted_terminal.last());
		if (next_real_t.isNull()) {
			return true;
		} else {
			const auto next_bridge = isBridged(next_real_t.realTerminal());
			if (compar_bridge != next_bridge) {
				return true;
			}
		}
	}

	return false;
}

QSharedPointer<TerminalStripBridge> TerminalStrip::bridgeFor(const QWeakPointer<RealTerminal> &real_terminal) const
{
	return this->isBridged(real_terminal.toStrongRef());
}

/**
 * @brief TerminalStrip::previousTerminalInLevel
 * @param real_terminal
 * @return The previous real terminal at the samne level of @a real_t
 * or a null RealTerminalData if there not a previous real terminal
 */
RealTerminalData TerminalStrip::previousTerminalInLevel(const QWeakPointer<RealTerminal> &real_terminal) const
{
	const auto real_t = real_terminal.toStrongRef();
	const auto phy_t = physicalTerminal(real_t);
	if (real_t && phy_t)
	{
		const auto level_ = phy_t->levelOf(real_t);
		const auto index = m_physical_terminals.indexOf(phy_t);
		if (index >= 1)
		{
			const auto t_vector = m_physical_terminals.at(index-1)->terminals();
			if (t_vector.size() > level_) {
				return RealTerminalData(t_vector.at(level_));
			}
		}
	}

	return RealTerminalData();
}

/**
 * @brief TerminalStrip::nextTerminalInLevel
 * @param real_terminal
 * @return The next real terminal at the same level of @a real_t
 * or a null RealTerminalData if there not a next real terminal
 */
RealTerminalData TerminalStrip::nextTerminalInLevel(const QWeakPointer<RealTerminal> &real_terminal) const
{
	const auto real_t = real_terminal.toStrongRef();
	const auto phy_t = physicalTerminal(real_t);
	if (real_t && phy_t)
	{
		const auto level_ = phy_t->levelOf(real_t);
		const auto index = m_physical_terminals.indexOf(phy_t);
		if (index < m_physical_terminals.size()-1)
		{
			const auto t_vector = m_physical_terminals.at(index+1)->terminals();
			if (t_vector.size() > level_) {
				return RealTerminalData(t_vector.at(level_));
			}
		}
	}

	return RealTerminalData();
}

RealTerminalData TerminalStrip::previousRealTerminal(const QWeakPointer<RealTerminal> &real_terminal) const
{
	const auto real = real_terminal.toStrongRef();
	const auto index = m_real_terminals.indexOf(real);
	if (index) {
		return RealTerminalData(m_real_terminals.at(index-1));
	}
	return RealTerminalData();
}

RealTerminalData TerminalStrip::nextRealTerminal(const QWeakPointer<RealTerminal> &real_terminal) const
{
	const auto real = real_terminal.toStrongRef();
	const auto index = m_real_terminals.indexOf(real);
	if (index != m_real_terminals.size()-1) {
		return RealTerminalData(m_real_terminals.at(index+1));
	}
	return RealTerminalData();
}

RealTerminalData TerminalStrip::realTerminalDataFor(const QWeakPointer<RealTerminal> &real_terminal) const
{
	const auto rt = real_terminal.toStrongRef();
	if (rt && m_real_terminals.contains(rt))
		return RealTerminalData(rt);
	else
		return RealTerminalData();
}

/**
 * @brief TerminalStrip::terminalElement
 * @return A vector of all terminal element owned by this strip
 */
QVector<QPointer<Element> > TerminalStrip::terminalElement() const {
	return m_terminal_elements_vector;
}

/**
 * @brief TerminalStrip::toXml
 * @param parent_document
 * @return
 */
QDomElement TerminalStrip::toXml(QDomDocument &parent_document)
{
	auto root_elmt = parent_document.createElement(this->xmlTagName());

	root_elmt.appendChild(m_data.toXml(parent_document));

		//Undrawed terminals
	auto xml_layout = parent_document.createElement("layout");
	for (auto &phy_t : m_physical_terminals) {
		xml_layout.appendChild(phy_t->toXml(parent_document));
	}
	root_elmt.appendChild(xml_layout);

	return root_elmt;
}

/**
 * @brief TerminalStrip::fromXml
 * @param xml_element
 * @return Set up this terminal strip from the xml description \p xml_element
 */
bool TerminalStrip::fromXml(QDomElement &xml_element)
{
	if (xml_element.tagName() != xmlTagName()) {
		return false;
	}

		//Read terminal strip data
	auto xml_data = xml_element.firstChildElement(m_data.xmlTagName());
	if (!xml_data.isNull()) {
		m_data.fromXml(xml_data);
	}

		//Read layout
	auto xml_layout = xml_element.firstChildElement(QStringLiteral("layout"));
	if (!xml_layout.isNull())
	{
			//Get all free elements terminal of the project
		const ElementProvider ep(m_project);
		const auto free_terminals = ep.freeTerminal();

			//Read each physical terminal
		for(auto &xml_physical : QETXML::findInDomElement(xml_layout, PhysicalTerminal::xmlTagName()))
		{
			QVector<shared_real_terminal> real_t_vector;

				//Read each real terminal of the current physical terminal of the loop
			for (auto &xml_real : QETXML::findInDomElement(xml_physical, RealTerminal::xmlTagName()))
			{
				shared_real_terminal real_t(new RealTerminal(this));
				real_t->fromXml(xml_real, free_terminals);
				if(real_t->isElement())
				{
					m_terminal_elements_vector.append(real_t->element());
					static_cast<TerminalElement*>(real_t->element())->setParentTerminalStrip(this);
				}
				real_t_vector.append(real_t);
			}

			const shared_physical_terminal phy_t(new PhysicalTerminal(this, real_t_vector));
			m_physical_terminals.append(phy_t);
			m_real_terminals.append(real_t_vector);
		}

	}

	return true;
}

/**
 * @brief TerminalStrip::realTerminal
 * @param terminal
 * @return the real terminal linked to \p terminal
 * the returned QSharedPointer can be null.
 */
QSharedPointer<RealTerminal> TerminalStrip::realTerminal(Element *terminal)
{
	for (auto &real : qAsConst(m_real_terminals)) {
		if (real->element() == terminal) {
			return real;
		}
	}

	return shared_real_terminal();
}

/**
 * @brief TerminalStrip::physicalTerminal
 * @param terminal
 * @return the physical terminal linked to \p terminal.
 * The returned QSharedPointer can be null.
 */
QSharedPointer<PhysicalTerminal> TerminalStrip::physicalTerminal(QSharedPointer<RealTerminal> terminal) const
{
	shared_physical_terminal pt;

	if (terminal.isNull()) {
		return pt;
	}

	for (auto &physical : qAsConst(m_physical_terminals)) {
		if (physical->terminals().contains(terminal)) {
			pt = physical;
			break;
		}
	}

	return pt;
}

/**
 * @brief TerminalStrip::isBridged
 * Check if @a real_terminal is bridged
 * @param real_terminal
 * @return a pointer of TerminalStripBridge if bridget or a null QSharedPointer.
 */
QSharedPointer<TerminalStripBridge> TerminalStrip::isBridged(const QSharedPointer<RealTerminal> real_terminal) const
{
	if (real_terminal)
	{
		for (const auto &bridge_ : qAsConst(m_bridge)) {
			if (bridge_->real_terminals.contains(real_terminal))
				return bridge_;
		}
	}
	return QSharedPointer<TerminalStripBridge>();
}

/**
 * @brief TerminalStrip::bridgeFor
 * Return the bridge where at least one terminal of @a terminal_vector belong.
 * If several terminals are bridged but not to the same bridge return
 * a TerminalStripBridge with 0 real_terminals_uuid_vector
 * @sa TerminalStripBridge
 * @param terminal_vector
 * @return
 */
QSharedPointer<TerminalStripBridge> TerminalStrip::bridgeFor(const QVector<QSharedPointer<RealTerminal> > &terminal_vector) const
{
	QSharedPointer<TerminalStripBridge> return_bridge;

	for (const auto &terminal : terminal_vector)
	{
		const auto bridge_ = isBridged(terminal);
		if (!bridge_.isNull())
		{
			if (return_bridge.isNull()) {
				return_bridge = bridge_;
			}
			else if (return_bridge != bridge_) {
				return QSharedPointer<TerminalStripBridge>();
			}
		}
	}

	return return_bridge;
}

/**
 * @brief TerminalStrip::rebuildRealVector
 * Rebuild the real terminal vector
 * to be ordered
 */
void TerminalStrip::rebuildRealVector()
{
	m_real_terminals.clear();
	for (const auto &phy : qAsConst(m_physical_terminals)) {
		m_real_terminals.append(phy->terminals());
	}
}

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/


/**
 * @brief RealTerminalData::RealTerminalData
 * @param real_terminal
 */
RealTerminalData::RealTerminalData(QSharedPointer<RealTerminal> real_terminal) :
	m_real_terminal(real_terminal.toWeakRef())
{}

RealTerminalData::RealTerminalData(QWeakPointer<RealTerminal> real_terminal) :
	m_real_terminal(real_terminal)
{}

bool RealTerminalData::isNull() const
{
	return m_real_terminal.isNull();
}

int RealTerminalData::level() const
{
	const auto shared_ = m_real_terminal.toStrongRef();
	if (shared_) {
		auto strip = shared_->parentStrip();
		if (strip) {
			auto phys = strip->physicalTerminal(shared_);
			if (phys) {
				return phys->levelOf(shared_);
			}
		}
	}

	return -1;
}

QString RealTerminalData::label() const
{
	const auto shared_ = m_real_terminal.toStrongRef();
	if (shared_) {
		return shared_->label();
	} else {
		return QString();
	}
}

QString RealTerminalData::Xref() const
{
	const auto shared_ = m_real_terminal.toStrongRef();
	if (shared_ && shared_->isElement()) {
		return autonum::AssignVariables::genericXref(shared_->element());
	} else {
		return QString();
	}
}

QString RealTerminalData::cable() const {
	return QString();
}

QString RealTerminalData::cableWire() const {
	return QString();
}

QString RealTerminalData::conductor() const {
	return QString();
}

ElementData::TerminalType RealTerminalData::type() const
{
	const auto shared_ = m_real_terminal.toStrongRef();
	if (shared_) {
		return shared_->type();
	}

	return ElementData::TerminalType::TTGeneric;
}

ElementData::TerminalFunction RealTerminalData::function() const
{
	const auto shared_ = m_real_terminal.toStrongRef();
	if (shared_) {
		return shared_->function();
	}

	return ElementData::TerminalFunction::TFGeneric;
}

bool RealTerminalData::isLed() const
{
	const auto shared_ = m_real_terminal.toStrongRef();
	if (shared_) {
		return shared_->led();
	}

	return false;
}

bool RealTerminalData::isElement() const
{
	const auto shared_ = m_real_terminal.toStrongRef();
	if (shared_) {
		return shared_->isElement();
	}

	return false;
}

bool RealTerminalData::isBridged() const
{
	const auto shared_ = m_real_terminal.toStrongRef();
	if (shared_) {
		auto strip = shared_->parentStrip();
		if (strip) {
			return !strip->isBridged(shared_).isNull();
		}
	}
	return false;
}

/**
 * @brief RealTerminalData::element
 * @return The element represented by this real
 * terminal, or nullptr
 */
Element *RealTerminalData::element() const
{
	const auto shared_ = m_real_terminal.toStrongRef();
	if (shared_) {
		return shared_->element();
	}

	return nullptr;
}

QUuid RealTerminalData::elementUuid() const
{
	const auto element_ = element();
	if (element_) {
		return element_->uuid();
	}
	return QUuid();
}

QSharedPointer<TerminalStripBridge> RealTerminalData::bridge() const
{
	const auto shared_ = m_real_terminal.toStrongRef();
	if (shared_) {
		auto strip = shared_->parentStrip();
		if (strip) {
			return strip->isBridged(shared_);
		}
	}
	return QSharedPointer<TerminalStripBridge>();
}

QWeakPointer<RealTerminal> RealTerminalData::realTerminal() const
{
	return m_real_terminal;
}

/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/
/************************************************************************************/


PhysicalTerminalData::PhysicalTerminalData(const TerminalStrip *strip, QSharedPointer<PhysicalTerminal> terminal) :
	m_strip(strip),
	m_physical_terminal(terminal.toWeakRef())
{}

bool PhysicalTerminalData::isNull() const
{
	return m_physical_terminal.isNull();
}

int PhysicalTerminalData::pos() const
{
	if (m_strip) {
		return m_strip->pos(m_physical_terminal);
	} else {
		return -1;
	}
}

QUuid PhysicalTerminalData::uuid() const
{
	const auto pt_ = m_physical_terminal.toStrongRef();
	if (pt_) {
		return pt_->uuid();
	} else {
		return QUuid();
	}
}

int PhysicalTerminalData::realTerminalCount() const
{
	const auto pt_ = m_physical_terminal.toStrongRef();
	if (pt_) {
		return pt_->terminals().size();
	} else {
		return 0;
	}

}

QVector<RealTerminalData> PhysicalTerminalData::realTerminalDatas() const
{
	QVector<RealTerminalData> rtd_vector;
	const auto pt_ = m_physical_terminal.toStrongRef();
	if (pt_ && m_strip)
	{
		for (const auto & rt_ : pt_->terminals()) {
			rtd_vector.append(m_strip->realTerminalDataFor(rt_.toWeakRef()));
		}
	}

	return rtd_vector;
}

QWeakPointer<PhysicalTerminal> PhysicalTerminalData::physicalTerminal() const {
	return m_physical_terminal;
}
