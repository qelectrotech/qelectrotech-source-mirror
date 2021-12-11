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
void TerminalStrip::setData(const TerminalStripData &data)
{
	auto uuid_ = m_data.m_uuid;
	m_data = data;
	m_data.m_uuid = uuid_;
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
	shared_real_terminal real_terminal(new RealTerminal(this, terminal));
	m_real_terminals.append(real_terminal);

		//Create a new single level physical terminal
	shared_physical_terminal physical_terminal(
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
	PhysicalTerminalData ptd;

	if (index < m_physical_terminals.size())
	{
		auto physical_terminal = m_physical_terminals.at(index);
		ptd.pos_ = index;
		for (auto real_terminal : physical_terminal->terminals()) {
			auto rtd = realTerminalData(real_terminal);
			ptd.real_terminals_vector.append(rtd);
		}
		ptd.uuid_ = physical_terminal->uuid();
	}

	return ptd;
}

/**
 * @brief TerminalStrip::physicalTerminalData
 * @param real_data
 * @return the parent PhysicalTerminalData of \p real_data.
 * the PhysicalTerminalData can be invalid if \p real_data don't belong to this strip
 */
PhysicalTerminalData TerminalStrip::physicalTerminalData(const RealTerminalData &real_data) const
{
	PhysicalTerminalData ptd_;

	const auto real_t = realTerminalForUuid(real_data.real_terminal_uuid);
	if (real_t.isNull()) {
		return ptd_;
	}

	const auto phy_t = physicalTerminal(real_t);
	if (phy_t.isNull()) {
		return ptd_;
	}

	ptd_.pos_ = m_physical_terminals.indexOf(phy_t);
	for (auto real_terminal : phy_t->terminals()) {
		auto rtd = realTerminalData(real_terminal);
		ptd_.real_terminals_vector.append(rtd);
	}
	ptd_.uuid_ = phy_t->uuid();

	return ptd_;
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
		const auto physical_t = physicalTerminalForUuid(ptd.uuid_);
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
bool TerminalStrip::groupTerminals(const PhysicalTerminalData &receiver_terminal, const QVector<RealTerminalData> &added_terminals)
{
	const auto receiver_ = physicalTerminalForUuid(receiver_terminal.uuid_);
	if (receiver_.isNull()) {
		qDebug() << "TerminalStrip::groupTerminal : Arguments terminals don't belong to this strip. Operation aborted.";
		return false;
	}

	bool have_grouped = false;
	for (const auto &added : added_terminals)
	{
		const auto added_terminal = realTerminalForUuid(added.real_terminal_uuid);

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
void TerminalStrip::unGroupTerminals(const QVector<RealTerminalData> &terminals_to_ungroup)
{
	bool ungrouped = false;
	for (const auto &rtd_ : terminals_to_ungroup)
	{
		if (auto real_terminal = realTerminalForUuid(rtd_.real_terminal_uuid)) //Get the shared real terminal
		{
			if (auto physical_terminal = physicalTerminal(real_terminal)) //Get the physical terminal
			{
				if (physical_terminal->terminals().size() > 1) //Check if physical have more than one real terminal
				{
					physical_terminal->removeTerminal(real_terminal);
					shared_physical_terminal new_physical_terminal (
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
bool TerminalStrip::setLevel(const RealTerminalData &real_terminal_data, int level)
{
	auto real_terminal = realTerminalForUuid(real_terminal_data.real_terminal_uuid);
	if (real_terminal)
	{
		auto physical_terminal = physicalTerminal(real_terminal);
		if (physical_terminal)
		{
			if (physical_terminal->terminals().size() > 1 &&
				physical_terminal->setLevelOf(real_terminal, level))
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
 * Check if all realTerminal represented by the uuid of @a real_terminals_uuid are bridgeable together.
 * To be bridgeable, each real terminal must belong to this terminal strip
 * be at the same level, be consecutive and not belong to the same physicalTerminal
 * and at least one terminal must be not bridged
 * @param real_terminals_uuid : a vector of RealTerminal uuid
 * @sa member real_terminal_uuid of struct RealTerminalData
 * @return
 */
bool TerminalStrip::isBridgeable(const QVector<QUuid> &real_terminals_uuid) const
{
	if (real_terminals_uuid.size() < 2) {
		return false;
	}

		// Check if first terminal belong to this strip
	auto first_real_terminal = realTerminalForUuid(real_terminals_uuid.first());
	if (!first_real_terminal) {
		return false;
	}
		// Get the level of the first terminal
	int level_ = realTerminalData(first_real_terminal).level_;
		// Get the physical terminal and pos
	auto first_physical_terminal = physicalTerminal(first_real_terminal);
	QVector<shared_physical_terminal> physical_vector{first_physical_terminal};
	QVector<int> pos_vector{m_physical_terminals.indexOf(first_physical_terminal)};

	auto bridge_ = isBridged(first_real_terminal);
		//bool to know at the end of this function if at least one terminal is not bridged
	bool no_bridged = bridge_ ? false : true;

		// Check for each terminals
	for (int i=1 ; i<real_terminals_uuid.size() ; ++i)
	{
			// If belong to this strip
		auto real_terminal = realTerminalForUuid(real_terminals_uuid.at(i));
		if (!real_terminal) {
			return false;
		}

			// at the same level
		if (level_ != realTerminalData(real_terminal).level_) {
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
		auto checked_bridge = isBridged(real_terminal);
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
	auto count_ = pos_vector.size();
	auto min_max = std::minmax_element(pos_vector.constBegin(), pos_vector.constEnd());
	if ((*min_max.second - *min_max.first) + 1 != count_) {
		return false;
	}

	return no_bridged;
}

/**
 * @brief TerminalStrip::setBridge
 * Set a bridge betwen all real terminal represented by they uuid
 * @param real_terminals_uuid
 * @sa TerminalStrip::isBridgeable
 * @return true if bridge was successfully created
 */
bool TerminalStrip::setBridge(const QVector<QUuid> &real_terminals_uuid)
{
	if (!isBridgeable(real_terminals_uuid)) {
		return false;
	}
	QVector<shared_real_terminal> real_terminals_vector;

	for (const auto & uuid_ : real_terminals_uuid) {
		auto real_t = realTerminalForUuid(uuid_);
		if (real_t)
			real_terminals_vector << real_t;
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
 * Bridge the RealTerminal with uuid in @a real_terminals_uuid to
 * the bridge with uuid @a bridge_uuid.
 * @param bridge_uuid
 * @param real_terminals_uuid
 * @return true if all RealTerminal was successfully bridged
 */
bool TerminalStrip::setBridge(const QUuid &bridge_uuid, const QVector<QUuid> &real_terminals_uuid)
{
	auto bridge_ = bridgeForUuid(bridge_uuid);
	if (bridge_)
	{
		if (!isBridgeable(real_terminals_uuid)) {
			return false;
		}

		bool b_ = false;
		for (const auto & uuid_ : real_terminals_uuid)
		{
			auto real_t = realTerminalForUuid(uuid_);
			if (real_t &&
				!bridge_->real_terminals.contains(real_t))
			{
				bridge_->real_terminals.append(real_t);
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
 * Unbridge all real terminal represented by they uuid
 * @param real_terminals_uuid
 */
void TerminalStrip::unBridge(const QVector<QUuid> &real_terminals_uuid)
{
	for (const auto & uuid_ : real_terminals_uuid)
	{
		auto real_t = realTerminalForUuid(uuid_);
		if (real_t)
		{
			auto bridge_ = isBridged(real_t);
			if (bridge_)
				bridge_->real_terminals.removeOne(real_t);
		}
	}

	emit bridgeChanged();
}

/**
 * @brief TerminalStrip::bridgeFor
 * @param real_terminal_uuid
 * @return
 */
const QSharedPointer<TerminalStripBridge> TerminalStrip::bridgeFor(const QUuid &real_terminal_uuid) const
{
	auto rt = realTerminalForUuid(real_terminal_uuid);
	return bridgeFor(QVector{rt});
}

/**
 * @brief TerminalStrip::previousTerminalInLevel
 * @param real_terminal_uuid
 * @return The previous real terminal at the samne level
 * as the real terminal with uuid @a real_terminal_uuid
 * If there is not a previous terminal, return a null RealTerminalData
 */
RealTerminalData TerminalStrip::previousTerminalInLevel(const QUuid &real_terminal_uuid) const
{
	auto real_t = realTerminalForUuid(real_terminal_uuid);
	if (real_t)
	{
		auto phy_t = physicalTerminal(real_t);
		if (phy_t)
		{
			auto level_ = phy_t->levelOf(real_t);
			auto index = m_physical_terminals.indexOf(phy_t);
			if (index >= 1)
			{
				auto t_vector = m_physical_terminals.at(index-1)->terminals();
				if (t_vector.size() > level_)
				{
					return realTerminalData(t_vector.at(level_));
				}
			}
		}
	}

	return RealTerminalData();
}

/**
 * @brief TerminalStrip::nextTerminalInLevel
 * @param real_terminal_uuid
 * @return The next real terminal at the samne level
 * as the real terminal with uuid @a real_terminal_uuid
 * If there is not a next terminal, return a null RealTerminalData
 */
RealTerminalData TerminalStrip::nextTerminalInLevel(const QUuid &real_terminal_uuid) const
{
	auto real_t = realTerminalForUuid(real_terminal_uuid);
	if (real_t)
	{
		auto phy_t = physicalTerminal(real_t);
		if (phy_t)
		{
			auto level_ = phy_t->levelOf(real_t);
			auto index = m_physical_terminals.indexOf(phy_t);
			if (index < m_physical_terminals.size()-1)
			{
				auto t_vector = m_physical_terminals.at(index+1)->terminals();
				if (t_vector.size() > level_)
				{
					return realTerminalData(t_vector.at(level_));
				}
			}
		}
	}

	return RealTerminalData();
}

RealTerminalData TerminalStrip::previousRealTerminal(const QUuid &real_terminal_uuid) const
{
	auto real = realTerminalForUuid(real_terminal_uuid);
	auto index = m_real_terminals.indexOf(real);
	if (index) {
		return realTerminalData(m_real_terminals.at(index-1));
	}
	return RealTerminalData();
}

RealTerminalData TerminalStrip::nextRealTerminal(const QUuid &real_terminal_uuid) const
{
	auto real = realTerminalForUuid(real_terminal_uuid);
	auto index = m_real_terminals.indexOf(real);
	if (index != m_real_terminals.size()-1) {
		return realTerminalData(m_real_terminals.at(index+1));
	}
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
		ElementProvider ep(m_project);
		auto free_terminals = ep.freeTerminal();

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

			shared_physical_terminal phy_t(new PhysicalTerminal(this, real_t_vector));
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
	shared_real_terminal rt;

	for (auto &real : qAsConst(m_real_terminals)) {
		if (real->element() == terminal) {
			return real;
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
QSharedPointer<PhysicalTerminal> TerminalStrip::physicalTerminal(QSharedPointer<RealTerminal> terminal) const
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

RealTerminalData TerminalStrip::realTerminalData(const QSharedPointer<RealTerminal> real_terminal) const
{
	RealTerminalData rtd;

	auto physical_terminal = physicalTerminal(real_terminal);

	rtd.real_terminal_uuid = real_terminal->uuid();
	rtd.level_ = physical_terminal->levelOf(real_terminal);
	rtd.label_ = real_terminal->label();

	if (real_terminal->isElement()) {
		rtd.Xref_ = autonum::AssignVariables::genericXref(real_terminal->element());
		rtd.element_uuid = real_terminal->elementUuid();
		rtd.element_ = real_terminal->element();
	}
	rtd.type_      = real_terminal->type();
	rtd.function_  = real_terminal->function();
	rtd.led_       = real_terminal->led();
	rtd.is_element = real_terminal->isElement();
	rtd.is_bridged = isBridged(real_terminal);
	if (rtd.is_bridged) {
		for (auto bridge : m_bridge) {
			if (bridge->real_terminals.contains(real_terminal)) {
				rtd.bridge_uuid = bridge->uuid_;
				break;
			}
		}
	}

	return rtd;
}

/**
 * @brief TerminalStrip::physicalTerminalForUuid
 * Return the PhysicalTerminal with uuid \p uuid or a null
 * PhysicalTerminal if uuid don't match
 * @param uuid
 * @return
 */
QSharedPointer<PhysicalTerminal> TerminalStrip::physicalTerminalForUuid(const QUuid &uuid) const
{
	shared_physical_terminal return_pt;

	for (const auto &pt_ : qAsConst(m_physical_terminals)) {
		if (pt_->uuid() == uuid) {
			return_pt = pt_;
			break;
		}
	}

	return return_pt;
}

/**
 * @brief TerminalStrip::realTerminalForUuid
 * @param uuid
 * @return  the RealTerminal with uuid \p uuid or a null
 * RealTerminal if uuid don't match
 */
QSharedPointer<RealTerminal> TerminalStrip::realTerminalForUuid(const QUuid &uuid) const
{
	shared_real_terminal return_rt;

	for (const auto &rt_ : qAsConst(m_real_terminals)) {
		if (rt_->uuid() == uuid) {
			return_rt = rt_;
			break;
		}
	}

	return return_rt;
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
		auto bridge_ = isBridged(terminal);
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
 * @brief TerminalStrip::bridgeForUuid
 * @param bridge_uuid
 * @return the bridge with uuid @a bridge_uuid or null QSharedPointer if not exist
 */
QSharedPointer<TerminalStripBridge> TerminalStrip::bridgeForUuid(const QUuid &bridge_uuid)
{
	for (const auto &bridge : qAsConst(m_bridge)) {
		if (bridge->uuid_ == bridge_uuid) {
			return bridge;
		}
	}

	return QSharedPointer<TerminalStripBridge>();
}

/**
 * @brief TerminalStrip::rebuildRealVector
 * Rebuild the real terminal vector
 * to be ordered
 */
void TerminalStrip::rebuildRealVector()
{
	m_real_terminals.clear();
	for (const auto phy : m_physical_terminals) {
		for (const auto &real : phy->terminals())
			m_real_terminals.append(real);
	}
}
