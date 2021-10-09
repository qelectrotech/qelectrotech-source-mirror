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
				return QStringLiteral("");
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
			auto uuid_ = QUuid::fromString(xml_element.attribute(QStringLiteral("uuid")));

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
		 * @brief addTerminals
		 * Append the real terminal \p terminal
		 * to this physical terminal.
		 * @param terminal
		 */
		void addTerminal(shared_real_terminal terminal) {
			m_real_terminal.append(terminal);
		}

		/**
		 * @brief removeTerminal
		 * Remove \p terminal from the list of real terminal
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
		 * @brief terminals
		 * @return A vector of real terminal who compose this physical terminal
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
 * The internal data of this strip to data.
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
 * @brief TerminalStrip::haveTerminal
 * @param terminal
 * @return true if \p terminal belong to this strip
 */
bool TerminalStrip::haveTerminal(Element *terminal) {
	return m_terminal_elements_vector.contains(terminal);
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
		ptd.physical_terminal = physical_terminal;
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

	ptd_.physical_terminal = phy_t;
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
bool TerminalStrip::setOrderTo(QVector<PhysicalTerminalData> sorted_vector)
{
	if (sorted_vector.size() != m_physical_terminals.size()) {
		return false;
	}

	QVector<QSharedPointer<PhysicalTerminal>> new_order;
	for (auto ptd : sorted_vector)
	{
		if (m_physical_terminals.contains(ptd.physical_terminal)) {
			new_order.append(ptd.physical_terminal);
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

/**
 * @brief TerminalStrip::elementForRealTerminal
 * @param rt
 * @return the element associated to \p rt, the returned element can be nullptr;
 */
Element *TerminalStrip::elementForRealTerminal(QSharedPointer<RealTerminal> rt) const {
	return rt.data()->element();
}

RealTerminalData TerminalStrip::realTerminalData(QSharedPointer<RealTerminal> real_terminal) const
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
