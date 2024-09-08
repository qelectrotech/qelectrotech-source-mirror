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
#include "terminalstrip.h"
#include "../qetproject.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminalelement.h"
#include "../elementprovider.h"
#include "../qetxml.h"
#include "../autoNum/assignvariables.h"
#include "physicalterminal.h"
#include "realterminal.h"
#include "terminalstripbridge.h"

using shared_real_terminal     = QSharedPointer<RealTerminal>;
using shared_physical_terminal = QSharedPointer<PhysicalTerminal>;

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
 * @brief TerminalStrip::project
 * @return the project of this terminal strip or nullptr
 */
QETProject *TerminalStrip::project() const {
	return m_project.data();
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

bool TerminalStrip::addTerminal(QSharedPointer<RealTerminal> real_t)
{
		//Check if terminal is already owned by a strip
		//return if this strip or false if another strip
	if (real_t->parentStrip()) {
		if (real_t->parentStrip() != this)
			return false;
		else
			return true;
	}

		//Create a new single level physical terminal
	auto raw_phy_ptr = new PhysicalTerminal(this, QVector<QSharedPointer<RealTerminal>>{real_t});
	m_physical_terminals.append(raw_phy_ptr->sharedRef());

	emit orderChanged();
	return true;
}

/**
 * @brief TerminalStrip::addTerminals
 * Add terminal in @a real_t_vector to this terminal
 * @param real_t_vector
 */
void TerminalStrip::addTerminals(QVector<QSharedPointer<RealTerminal>> real_t_vector)
{
	bool added_{false};
	for (const auto &real_t : real_t_vector)
	{
		if (real_t->parentStrip()) {
			continue;
		}

		auto raw_phy_ptr = new PhysicalTerminal(this, QVector<QSharedPointer<RealTerminal>>{real_t});
		m_physical_terminals.append(raw_phy_ptr->sharedRef());
		added_ = true;
	}

	if (added_) {
		emit orderChanged();
	}
}

/**
 * @brief TerminalStrip::addAndGroupTerminals
 * For each real terminals of second vector, add it to terminal strip and group
 * them together
 * @param real_t_vector
 */
void TerminalStrip::addAndGroupTerminals(const QVector<QVector<QSharedPointer<RealTerminal>>> &real_t_vector)
{
	QVector<QSharedPointer<RealTerminal>> vector_;
	bool added_{false};

	for (const auto &t_vector : real_t_vector)
	{
		vector_.clear();
		for (const auto &real_t : t_vector)
		{
			if (real_t->parentStrip()) {
				continue;
			}
			vector_.append(real_t);
		}

		auto raw_phy_ptr = new PhysicalTerminal(this, vector_);
		m_physical_terminals.append(raw_phy_ptr->sharedRef());
		added_ = true;
	}

	if (added_) {
		emit orderChanged();
	}
}

bool TerminalStrip::removeTerminal(QSharedPointer<RealTerminal> real_t)
{
	if (real_t->parentStrip() != this) {
		return false;
	}

	if (auto bridge_ = real_t->bridge()) {
		bridge_->removeTerminal(real_t);
	}

	if (auto phy_t = real_t->physicalTerminal())
	{
		phy_t->removeTerminal(real_t);
		if (phy_t->realTerminalCount() == 0) {
			m_physical_terminals.removeOne(phy_t);
		}

		emit orderChanged();
		return true;
	}

	return false;
}

/**
 * @brief TerminalStrip::removeTerminals
 * Remove terminals in @a real_t_vector from this terminal strip
 * @param real_t_vector
 */
void TerminalStrip::removeTerminals(QVector<QSharedPointer<RealTerminal>> real_t_vector)
{
	bool removed_{false};
	for (const auto &real_t : real_t_vector)
	{
		if (real_t->parentStrip() != this) {
			continue;
		}

		if (auto bridge_ = real_t->bridge()) {
			bridge_->removeTerminal(real_t);
		}

		if (auto phy_t = real_t->physicalTerminal())
		{
			phy_t->removeTerminal(real_t);
			if (phy_t->realTerminalCount() == 0) {
				m_physical_terminals.removeOne(phy_t);
			}
			removed_ = true;
		}
	}

	if (removed_) {
		emit orderChanged();
	}
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
	for (const auto &real_t : realTerminals()) {
		if (real_t->element() == terminal) {
			return false;
		}
	}

	if (terminal->elementData().m_type != ElementData::Terminal) {
		return false;
	}

	auto casted_ = static_cast<TerminalElement *>(terminal);

		//Create a new single level physical terminal
	auto raw_phy_ptr = new PhysicalTerminal(this, QVector<QSharedPointer<RealTerminal>>{casted_->realTerminal()});
	m_physical_terminals.append(raw_phy_ptr->sharedRef());

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
	for (const auto &real_t : realTerminals())
	{
		if (real_t->element() == terminal)
		{
			if (auto physical_t = real_t->physicalTerminal())
			{
				physical_t->removeTerminal(real_t);
				if (physical_t->realTerminalCount() == 0) {
					m_physical_terminals.removeOne(physical_t);
				}
			}

			return true;
		}
	}

	return false;
}

/**
 * @brief TerminalStrip::addTerminal
 * Add @a phy_t in this terminal strip.
 * @param phy_t
 * @return true if successfully added. A terminal can't be added is already
 * belong to another terminal strip.
 */
bool TerminalStrip::addTerminal(QSharedPointer<PhysicalTerminal> phy_t)
{
	if (phy_t->terminalStrip()) {
		if (phy_t->terminalStrip() == this)
			return true;
		else
			return false;
	}

	m_physical_terminals.append(phy_t);
	phy_t->setParentStrip(this);

	emit orderChanged();
	return true;

}

void TerminalStrip::addTerminals(QVector<QSharedPointer<PhysicalTerminal> > phy_t_vector)
{
	bool added_{false};
	for (const auto &phy_t : phy_t_vector)
	{
		if (phy_t->terminalStrip()) {
			continue;
		}

		m_physical_terminals.append(phy_t);
		phy_t->setParentStrip(this);
		added_ = true;
	}

	if (added_) {
		emit orderChanged();
	}
}

/**
 * @brief TerminalStrip::removeTerminal
 * Remove @a phy_t from this terminal strip.
 * @param phy_t
 * @return true if successfully removed. Return false if can't be removed
 * because not owned by this strip
 */
bool TerminalStrip::removeTerminal(QSharedPointer<PhysicalTerminal> phy_t)
{
	if (m_physical_terminals.removeOne(phy_t))
	{
		for (const auto &real_t : phy_t->realTerminals()) {
			if (auto bridge_ = real_t->bridge()) {
				bridge_->removeTerminal(real_t);
			}
		}

		phy_t->setParentStrip(nullptr);
		emit orderChanged();
		return true;
	}

	return false;
}

void TerminalStrip::removeTerminals(QVector<QSharedPointer<PhysicalTerminal> > phy_t_vector)
{
	bool removed_{false};
	for (const auto &phy_t : phy_t_vector)
	{
		if (m_physical_terminals.removeOne(phy_t))
		{
			for (const auto &real_t : phy_t->realTerminals()) {
				if (auto bridge_ = real_t->bridge()) {
					bridge_->removeTerminal(real_t);
				}
			}

			phy_t->setParentStrip(nullptr);
			removed_ = true;
		}
	}

	if (removed_) {
		emit orderChanged();
	}
}

/**
 * @brief TerminalStrip::pos
 * @param terminal
 * @return the position of the physical terminal
 * or -1 if not found
 */
int TerminalStrip::pos(const QSharedPointer<PhysicalTerminal> &terminal) const {
	return m_physical_terminals.indexOf(terminal);
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
 * returned QSharedPointer can be null
 */
QSharedPointer<PhysicalTerminal> TerminalStrip::physicalTerminal(int index) const
{
	if (index < m_physical_terminals.size()) {
		return m_physical_terminals.at(index);
	} else {
		return QSharedPointer<PhysicalTerminal>();
	}
}

/**
 * @brief TerminalStrip::physicalTerminal
 * @param uuid
 * @return the the physicalTerminal with the uuid @a uuid or a empty
 * QSharedPointer if empty.
 */
QSharedPointer<PhysicalTerminal> TerminalStrip::physicalTerminal(const QUuid &uuid) const
{
	for (const auto &phy_t : m_physical_terminals)
	{
		if (phy_t->uuid() == uuid) {
			return phy_t;
		}
	}

	return QSharedPointer<PhysicalTerminal>();
}

/**
 * @brief TerminalStrip::physicalTerminal
 * @return A vector of all physical terminal owned by this terminal strip.
 * The order of the vector is the same as the order of the terminal in the strip
 */
QVector<QSharedPointer<PhysicalTerminal>> TerminalStrip::physicalTerminal() const {
	return m_physical_terminals;
}

/**
 * @brief TerminalStrip::realTerminalForUuid
 * @param uuid
 * @return the real terminal with uuid @a uuid or a null QSharedPointer if not found
 */
QSharedPointer<RealTerminal> TerminalStrip::realTerminalForUuid(const QUuid &uuid) const
{
	for (const auto &t : realTerminals()) {
		if (t->elementUuid() == uuid) {
			return t;
		}
	}

	return QSharedPointer<RealTerminal>();
}

/**
 * @brief TerminalStrip::realTerminals
 * @return All real terminal owned by this strip
 */
QVector<QSharedPointer<RealTerminal>> TerminalStrip::realTerminals() const
{
	QVector<QSharedPointer<RealTerminal>> vector_;
	for (const auto &phy : qAsConst(m_physical_terminals)) {
		vector_.append(phy->realTerminals());
	}
	return vector_;
}


/**
 * @brief TerminalStrip::setSortedTo
 * Sort the physical terminal owned by this strip in the same order
 * as \p sorted_vector.
 * \p sorted_vector must contain exactly the same physical terminal as this strip
 * else this function do nothing.
 *
 * To avoid any mistake, you should call TerminalStrip::physicalTerminal()
 * sort the returned vector and call this function with sorted vector, then you are sure
 * the vector contain the same values, no more no less.
 *
 * @param sorted_vector
 * @return true is successfully sorted.
 */
bool TerminalStrip::setOrderTo(const QVector<QSharedPointer<PhysicalTerminal>> &sorted_vector)
{
	if (sorted_vector.size() != m_physical_terminals.size()) {
		return false;
	}

	QVector<QSharedPointer<PhysicalTerminal>> new_order;
	for (const auto &physical_t : sorted_vector)
	{
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
bool TerminalStrip::groupTerminals(const QSharedPointer<PhysicalTerminal> &receiver_terminal, const QVector<QSharedPointer<RealTerminal>> &added_terminals)
{
	if (receiver_terminal.isNull()) {
		qDebug() << "TerminalStrip::groupTerminal : Arguments terminals don't belong to this strip. Operation aborted.";
		return false;
	}

	bool have_grouped = false;
	for (const auto &added_terminal : added_terminals)
	{
		if (added_terminal.isNull()) {
			continue;
		}

		if (auto phy_t = added_terminal->physicalTerminal()) {
			phy_t->removeTerminal(added_terminal);
		}
		receiver_terminal->addTerminal(added_terminal);
		have_grouped = true;
	}

	if (have_grouped)
	{
		const auto vector_ = m_physical_terminals;
		for (const auto &phys : vector_) {
			if (phys->realTerminals().isEmpty()) {
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
void TerminalStrip::unGroupTerminals(const QVector<QSharedPointer<RealTerminal>> &terminals_to_ungroup)
{
	bool ungrouped = false;
	for (const auto &real_terminal : terminals_to_ungroup)
	{
		if (real_terminal)
		{
			if (auto physical_terminal = real_terminal->physicalTerminal()) //Get the physical terminal
			{
				if (physical_terminal->realTerminals().size() > 1) //Check if physical have more than one real terminal
				{
					physical_terminal->removeTerminal(real_terminal);
					auto raw_ptr = new PhysicalTerminal(this, QVector<QSharedPointer<RealTerminal>>{real_terminal});
					m_physical_terminals.append(raw_ptr->sharedRef());
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
bool TerminalStrip::setLevel(const QSharedPointer<RealTerminal> &real_terminal, int level)
{
	if (real_terminal)
	{
		if (auto physical_terminal = real_terminal->physicalTerminal())
		{
			if (physical_terminal->realTerminals().size() > 1 &&
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
 * Check if all realTerminal in @a real_terminals are bridgeable together.
 * To be bridgeable, each real terminal must belong to this terminal strip
 * be at the same level, be consecutive and not belong to the same physicalTerminal
 * and at least one terminal must be not bridged
 * @param real_terminals : a vector of realterminal
 * @return
 */
bool TerminalStrip::isBridgeable(const QVector<QSharedPointer<RealTerminal>> &real_terminals) const
{
	if (real_terminals.size() < 2) {
		return false;
	}

		// Check if first terminal belong to this strip
	const auto first_real_terminal = real_terminals.first();
	if (!first_real_terminal) {
		return false;
	}

		// Get the level of the first terminal
	const int level_ = first_real_terminal->level();

		// Get the physical terminal and pos
	auto first_physical_terminal = first_real_terminal->physicalTerminal();
	QVector<shared_physical_terminal> physical_vector{first_physical_terminal};
	QVector<qsizetype> pos_vector{m_physical_terminals.indexOf(first_physical_terminal)};

	auto bridge_ = isBridged(first_real_terminal);
		//bool to know at the end of this function if at least one terminal is not bridged
	bool no_bridged = bridge_ ? false : true;

			// Check for each terminals
	for (int i=1 ; i<real_terminals.size() ; ++i)
	{
			// If belong to this strip
		const auto real_terminal = real_terminals.at(i);
		if (!real_terminal) {
			return false;
		}

			// at the same level
		if (level_ != real_terminal->level()) {
			return false;
		}

			// Not to the same physical terminal of a previous checked real terminal
		const auto physical_terminal = real_terminal->physicalTerminal();
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
 * @brief TerminalStrip::isBridgeable
 * Check if all RealTerminal of @a real_terminals can be bridged to
 * the bridge @a bridge.
 * @param real_terminals
 * @return true if can be bridged.
 */
bool TerminalStrip::isBridgeable(QSharedPointer<TerminalStripBridge> bridge, const QVector<QSharedPointer<RealTerminal> > &real_terminals) const
{
	if (real_terminals.isEmpty() ||
		!m_bridge.contains(bridge)) {
		return false;
	}

	auto vector_ = bridge->realTerminals();
	vector_.append(real_terminals);

	return isBridgeable(vector_);
}

/**
 * @brief TerminalStrip::setBridge
 * Set a bridge between all real terminal of @a real_terminals
 * @sa TerminalStrip::isBridgeable
 * @return true if bridge was successfully created
 */
bool TerminalStrip::setBridge(const QVector<QSharedPointer<RealTerminal>> &real_terminals)
{
	if (!isBridgeable(real_terminals)) {
		return false;
	}

	auto bridge = bridgeFor(real_terminals);
	if (bridge.isNull())
	{
		auto br_ = new TerminalStripBridge(this);
		bridge = br_->sharedRef();
		m_bridge.append(bridge);
	}

	if (bridge->addTerminals(real_terminals))
	{
		emit bridgeChanged();;
		return true;
	}
	return false;
}

/**
 * @brief TerminalStrip::setBridge
 * Bridge the RealTerminal of @a real_terminals_data to @a bridge
 * @param bridge
 * @param real_terminals_data
 * @return true if all RealTerminal was successfully bridged
 */
bool TerminalStrip::setBridge(const QSharedPointer<TerminalStripBridge> &bridge, const QVector<QSharedPointer<RealTerminal>> &real_terminals)
{
	if (bridge)
	{
		if (!isBridgeable(bridge, real_terminals)) {
			return false;
		}

		if (bridge->addTerminals(real_terminals))
		{
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
void TerminalStrip::unBridge(const QVector<QSharedPointer<RealTerminal>> &real_terminals)
{
	if (canUnBridge(real_terminals))
	{
		auto bridge_ = isBridged(real_terminals.first());
		bridge_->removeTerminals(real_terminals);
		emit bridgeChanged();
	}
}

/**
 * @brief TerminalStrip::canUnBridge
 * @param m_real_terminals
 * @return True if all terminals of @a real_terminals can be unbridged.
 * For this method return True, all terminals must be bridged together,
 * be consecutive and in one or the both extremities of the bridge.
 */
bool TerminalStrip::canUnBridge(const QVector<QSharedPointer<RealTerminal> > &real_terminals) const
{
	if (real_terminals.isEmpty()) {
		return false;
	}

		//Get the bridge of first terminal
	const auto compar_bridge = isBridged(real_terminals.first());
	if (compar_bridge)
	{
		QMap<int, QSharedPointer<RealTerminal>> sorted_terminal;

			//Check if all terminals are bridged and if it's the same bridge.
			//If true insert the terminal in sorted_terminal QMap
			//with for key the position of the parent physical terminal
		for (const auto &real_t : real_terminals) {
			if (compar_bridge != isBridged(real_t)) {
				return false;
			} else {
				sorted_terminal.insert(m_physical_terminals.indexOf(real_t->physicalTerminal()),
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
			const auto previous_bridge = isBridged(previous_real_t);
			if (compar_bridge != previous_bridge) {
				return true;
			}
		}

			//Check if last terminal is the end of bridge
		const auto next_real_t = nextTerminalInLevel(sorted_terminal.last());
		if (next_real_t.isNull()) {
			return true;
		} else {
			const auto next_bridge = isBridged(next_real_t);
			if (compar_bridge != next_bridge) {
				return true;
			}
		}
	}

	return false;
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
			if (bridge_->realTerminals().contains(real_terminal))
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
 * @brief TerminalStrip::previousTerminalInLevel
 * @param real_terminal
 * @return The previous real terminal at the samne level of @a real_t
 * or a null RealTerminalData if there not a previous real terminal
 */
QSharedPointer<RealTerminal> TerminalStrip::previousTerminalInLevel(const QSharedPointer<RealTerminal> &real_terminal) const
{
	const auto phy_t = real_terminal->physicalTerminal();
	if (real_terminal && phy_t)
	{
		const auto level_ = phy_t->levelOf(real_terminal);
		const auto index = m_physical_terminals.indexOf(phy_t);
		if (index >= 1)
		{
			const auto t_vector = m_physical_terminals.at(index-1)->realTerminals();
			if (t_vector.size() > level_) {
				return t_vector.at(level_);
			}
		}
	}

	return QSharedPointer<RealTerminal>();
}

/**
 * @brief TerminalStrip::nextTerminalInLevel
 * @param real_terminal
 * @return The next real terminal at the same level of @a real_t
 * or a null RealTerminalData if there not a next real terminal
 */
QSharedPointer<RealTerminal> TerminalStrip::nextTerminalInLevel(const QSharedPointer<RealTerminal> &real_terminal) const
{
	const auto phy_t = real_terminal->physicalTerminal();
	if (real_terminal && phy_t)
	{
		const auto level_ = phy_t->levelOf(real_terminal);
		const auto index = m_physical_terminals.indexOf(phy_t);
		if (index < m_physical_terminals.size()-1)
		{
			const auto t_vector = m_physical_terminals.at(index+1)->realTerminals();
			if (t_vector.size() > level_) {
				return t_vector.at(level_);
			}
		}
	}

	return QSharedPointer<RealTerminal>();
}

QSharedPointer<RealTerminal> TerminalStrip::previousRealTerminal(const QSharedPointer<RealTerminal> &real_terminal) const
{
	const auto real_t_vector = realTerminals();
	const auto index = real_t_vector.indexOf(real_terminal);
	if (index) {
		return real_t_vector.at(index-1);
	}
	return QSharedPointer<RealTerminal>();
}

QSharedPointer<RealTerminal> TerminalStrip::nextRealTerminal(const QSharedPointer<RealTerminal> &real_terminal) const
{
	const auto real_t_vector = realTerminals();
	const auto index = real_t_vector.indexOf(real_terminal);
	if (index != real_t_vector.size()-1) {
		return real_t_vector.at(index+1);
	}
	return QSharedPointer<RealTerminal>();
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
	auto xml_layout = parent_document.createElement(QStringLiteral("layout"));

	for (auto &phy_t : m_physical_terminals) {
		xml_layout.appendChild(phy_t->toXml(parent_document));
	}
	root_elmt.appendChild(xml_layout);

	for (const auto &bridge_ : qAsConst(m_bridge)) {
		root_elmt.appendChild(bridge_->toXml(parent_document));
	}

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
		auto free_terminals = ep.freeTerminal();

			//Read each physical terminal
		for(auto &xml_physical : QETXML::findInDomElement(xml_layout, PhysicalTerminal::xmlTagName()))
		{
			QVector<shared_real_terminal> real_t_vector;

				//Read each real terminal of the current physical terminal of the loop
			for (auto &xml_real : QETXML::findInDomElement(xml_physical, RealTerminal::xmlTagName()))
			{
				const auto uuid_ = QUuid(xml_real.attribute(QStringLiteral("element_uuid")));
				for (auto terminal_elmt : qAsConst(free_terminals))
				{
					if (terminal_elmt->uuid() == uuid_)
					{
						real_t_vector.append(terminal_elmt->realTerminal());
							//Remove the actual terminal element from the vector, they dicrease the size
							//of the vector and so each iteration have less terminal element to check
						free_terminals.removeOne(terminal_elmt);
						break;
					}
				}
			}

			auto raw_ptr = new PhysicalTerminal(this, real_t_vector);
			m_physical_terminals.append(raw_ptr->sharedRef());
		}

	}

		//Read bridges
	const auto bridge_vector = QETXML::directChild(xml_element, TerminalStripBridge::xmlTagName());
	for (const auto &xml_bridge : bridge_vector)
	{
		auto bridge_ = new TerminalStripBridge(this);
		auto shared_bridge = bridge_->sharedRef();
		shared_bridge->fromXml(xml_bridge);
		m_bridge.append(shared_bridge);
	}

	return true;
}
