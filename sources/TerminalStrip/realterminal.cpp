/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "terminalcolor.h"
#include "terminalcatalog.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetinformation.h"
#include "../diagram.h"
#include "../cable/cable.h"
#include "../cable/cablemanager.h"
#include "../qetproject.h"

#include <QHash>
#include <QSet>

namespace
{
		/**
		 * The cable cores wired to one terminal element, worked out from
		 * the cables of the project (PR #1438). A core knows its
		 * conductor, and CableManager::refreshLabels() writes the cable
		 * and the core into the properties of that conductor, so reading
		 * the conductors of the element is enough.
		 * Each pair is (cable designation, core colour or number).
		 */
	QList<QPair<QString, QString>> wiredCores(Element *element)
	{
		QList<QPair<QString, QString>> cores;
		if (!element || !element->diagram()) {
			return cores;
		}
		const auto project = element->diagram()->project();
		for (const auto terminal : element->terminals())
		{
			for (const auto conductor : terminal->conductors())
			{
				const auto properties = conductor->ownProperties();
				const auto cable = CableManager::cableByUuid(project, properties.m_cable_uuid);
				if (!cable || cable->designation().isEmpty()) {
					continue;
				}
				auto wire = cable->colorOfCore(properties.m_cable_slot);
				if (wire.isEmpty() && properties.m_cable_slot >= 0) {
					wire = QString::number(properties.m_cable_slot + 1);
				}
				const QPair<QString, QString> core(cable->designation(), wire);
				if (!cores.contains(core)) {
					cores.append(core);
				}
			}
		}
		return cores;
	}
}

/**
 * @brief RealTerminal
 * @param parent_strip : parent terminal strip
 * @param terminal : terminal element (if any) in a folio
 */
RealTerminal::RealTerminal(Element *terminal) :
	m_element(terminal)
{}

/**
 * @brief RealTerminal::createSpare
 * Create a spare terminal (a reserved terminal) : a terminal of a strip
 * which is not drawn on any folio. Its data (label, type, section,
 * width...) are its own, stored with the terminal strip.
 * @param data : the data of the terminal, only the terminal type,
 * function, led and the informations are used.
 * @param uuid : the uuid of the spare terminal
 * @return the new spare terminal, not yet added to a strip
 */
QSharedPointer<RealTerminal> RealTerminal::createSpare(const ElementData &data, const QUuid &uuid)
{
	auto raw_ = new RealTerminal(nullptr);
	raw_->m_spare = true;
	raw_->m_spare_uuid = uuid.isNull() ? QUuid::createUuid() : uuid;
	raw_->m_spare_data.m_type = ElementData::Terminal;
	raw_->m_spare_data.setTerminalType(data.terminalType());
	raw_->m_spare_data.setTerminalFunction(data.terminalFunction());
	raw_->m_spare_data.setTerminalLED(data.terminalLed());
	raw_->m_spare_data.m_informations = data.m_informations;
	return raw_->sharedRef();
}

/**
 * @brief RealTerminal::spareFromXml
 * @param xml_element : a real terminal saved by toXml()
 * @return the spare terminal saved in @a xml_element,
 * or a null pointer if @a xml_element is not a spare terminal.
 */
QSharedPointer<RealTerminal> RealTerminal::spareFromXml(const QDomElement &xml_element)
{
	if (xml_element.tagName() != xmlTagName() ||
		!xml_element.hasAttribute(QStringLiteral("spare_uuid"))) {
		return QSharedPointer<RealTerminal>();
	}

	ElementData data_;
	data_.setTerminalType(ElementData::terminalTypeFromString(
							  xml_element.attribute(QStringLiteral("type"))));
	data_.setTerminalFunction(ElementData::terminalFunctionFromString(
								  xml_element.attribute(QStringLiteral("function"))));
	data_.setTerminalLED(QET::infoFlagIsTrue(xml_element.attribute(QStringLiteral("led"))));
	data_.m_informations.fromXml(xml_element.firstChildElement(QStringLiteral("informations")),
								 QStringLiteral("information"));

	return createSpare(data_, QUuid(xml_element.attribute(QStringLiteral("spare_uuid"))));
}

/**
 * @brief RealTerminal::isSpare
 * @return true if this terminal is a spare terminal, not drawn on any folio
 * @sa createSpare
 */
bool RealTerminal::isSpare() const {
	return m_spare;
}

/**
 * @brief RealTerminal::spareData
 * @return the data of this spare terminal, an empty ElementData
 * if this terminal is not a spare terminal.
 */
ElementData RealTerminal::spareData() const {
	return m_spare_data;
}

/**
 * @brief RealTerminal::setSpareData
 * Set the data of this spare terminal (nothing is done if this terminal
 * is not a spare terminal) and tell the strip, so that it is redrawn.
 * @param data
 */
void RealTerminal::setSpareData(const ElementData &data)
{
	if (!m_spare) {
		return;
	}
	m_spare_data.setTerminalType(data.terminalType());
	m_spare_data.setTerminalFunction(data.terminalFunction());
	m_spare_data.setTerminalLED(data.terminalLed());
	m_spare_data.m_informations = data.m_informations;

	if (auto strip_ = parentStrip()) {
		strip_->terminalDataChanged();
	}
}

/**
 * @brief RealTerminal::hasData
 * @return true if this terminal has data : it is linked to an element,
 * or it is a spare terminal.
 */
bool RealTerminal::hasData() const {
	return m_spare || !m_element.isNull();
}

/**
 * @brief RealTerminal::terminalData
 * @return the data of this terminal : the data of its element,
 * or its own data if it is a spare terminal.
 */
ElementData RealTerminal::terminalData() const
{
	if (m_spare) {
		return m_spare_data;
	}
	if (m_element) {
		return m_element->elementData();
	}
	return ElementData();
}

RealTerminal::~RealTerminal()
{
	if (m_physical_terminal) {
		m_physical_terminal->removeTerminal(sharedRef());
	}
}

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
 * @brief RealTerminal::sharedRef
 * @return a shared reference of this, not that because
 * this method is const, the shared reference can be null if not already
 * used in another part of the code.
 */
QSharedPointer<RealTerminal> RealTerminal::sharedRef() const {
	return QSharedPointer<RealTerminal>(m_this_weak);
}

/**
 * @brief RealTerminal::weakRef
 * @return a QWeakPointer of this, weak pointer can be bull
 */
QWeakPointer<RealTerminal> RealTerminal::weakRef() {
	return m_this_weak;
}

/**
 * @brief toXml
 * @param parent_document
 * @return this real terminal to xml
 */
QDomElement RealTerminal::toXml(QDomDocument &parent_document) const
{
	auto root_elmt = parent_document.createElement(this->xmlTagName());
	if (m_spare)
	{
			//A spare terminal is on no folio : its data are saved here
		root_elmt.setAttribute(QStringLiteral("spare_uuid"), m_spare_uuid.toString());
		root_elmt.setAttribute(QStringLiteral("type"), ElementData::terminalTypeToString(m_spare_data.terminalType()));
		root_elmt.setAttribute(QStringLiteral("function"), ElementData::terminalFunctionToString(m_spare_data.terminalFunction()));
		if (m_spare_data.terminalLed()) {
			root_elmt.setAttribute(QStringLiteral("led"), QStringLiteral("true"));
		}
		auto xml_info = parent_document.createElement(QStringLiteral("informations"));
		m_spare_data.m_informations.toXml(xml_info, QStringLiteral("information"));
		root_elmt.appendChild(xml_info);
	}
	else if (m_element) {
		root_elmt.setAttribute(QStringLiteral("element_uuid"), m_element->uuid().toString());
	}

	return root_elmt;
}

/**
 * @brief RealTerminal::setPhysicalTerminal
 * Set the parent physical terminal of this real terminal
 * @param phy_t
 */
void RealTerminal::setPhysicalTerminal(const QSharedPointer<PhysicalTerminal> &phy_t) {
	m_physical_terminal = phy_t;
}

/**
* @brief parentStrip
* @return parent terminal strip or nullptr
*/
TerminalStrip *RealTerminal::parentStrip() const noexcept {
	if (m_physical_terminal) {
		return m_physical_terminal->terminalStrip();
	} else {
		return nullptr;
	}
}

/**
 * @brief RealTerminal::physicalTerminal
 * @return The parent physical terminal of this terminal.
 * The returned QSharedPointer can be null
 */
QSharedPointer<PhysicalTerminal> RealTerminal::physicalTerminal() const noexcept{
	return m_physical_terminal;
}

/**
 * @brief RealTerminal::level
 * @return
 */
int RealTerminal::level() const
{
	if (m_physical_terminal &&
		sharedRef()) {
		return m_physical_terminal->levelOf(sharedRef());
	}

	return -1;
}

/**
 * @brief label
 * @return the label of this real terminal
 */
QString RealTerminal::label() const {
	if (m_spare) {
		return m_spare_data.m_informations.value(QStringLiteral("label")).toString();
	} else if (!m_element.isNull()) {
		return m_element->actualLabel();
	} else {
		return QLatin1String();
	}
}

/**
 * @brief RealTerminal::Xref
 * @return Convenient method to get the XRef
 * formatted to string
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
 * @return The name of the cable (hose) connected to this terminal.
 * The value is stored in the informations of the element.
 */
QString RealTerminal::cable() const
{
	const auto cores = wiredCores(m_element.data());
	if (!cores.isEmpty())
	{
		QStringList cables;
		for (const auto &core : cores) {
			if (!cables.contains(core.first)) {
				cables.append(core.first);
			}
		}
		return cables.join(QStringLiteral(", "));
	}

	if (hasData()) {
		return terminalData().m_informations.value(cableInfoKey()).toString();
	}
	return QString();
}

/**
 * @brief RealTerminal::cableIsAutomatic
 * @return true when the cable and the core of this terminal come from
 * a cable of the project wired to it, false when they are the values
 * typed in by the user in the terminal strip editor.
 */
bool RealTerminal::cableIsAutomatic() const {
	return !wiredCores(m_element.data()).isEmpty();
}

/**
 * @brief RealTerminal::cableWire
 * @return The color / number of the wire of the cable, connected to this terminal.
 */
QString RealTerminal::cableWire() const
{
	const auto cores = wiredCores(m_element.data());
	if (!cores.isEmpty())
	{
		QStringList wires;
		for (const auto &core : cores) {
			wires.append(core.second);
		}
		return wires.join(QStringLiteral(", "));
	}

	if (hasData()) {
		return terminalData().m_informations.value(cableWireInfoKey()).toString();
	}
	return QString();
}

/**
 * @brief RealTerminal::isShield
 * @return true if this terminal is the shield of the cable
 * and not one of its wires.
 */
bool RealTerminal::isShield() const {
	if (hasData()) {
		return QET::infoFlagIsTrue(
			terminalData().m_informations.value(shieldInfoKey()).toString());
	}
	return false;
}

/**
 * @brief RealTerminal::cableTarget
 * Where the cable of this terminal goes at its far end: the lines to
 * write in the box under the cable. What the project itself says when
 * a cable lands on a piece (see automaticCableTarget()), and the text
 * typed in by hand when none does -- the Target cell of the terminal
 * strip editor is read only in the first case, so a target the drawing
 * cannot work out is still worth naming by hand. A text typed in is
 * written one line per field, Installation, Location and Label (BMK),
 * the way the cell shows them: only a line break breaks it into fields.
 * A "/" written inside a field (a name like "ASP/01") is part of that
 * field and never breaks anything.
 * @return the lines to write in the box under the cable
 */
QVector<QString> RealTerminal::cableTarget() const
{
	const auto automatic_{automaticCableTarget()};
	if (!automatic_.isEmpty()) {
		return automatic_;
	}

	if (hasData())
	{
		const auto typed_{terminalData().m_informations
						  .value(cableTargetInfoKey()).toString()};
		if (!typed_.isEmpty())
		{
				//One line by field, and only a line break breaks the text
				//into fields : a "/" written inside a field stays where it is
			QString text_{typed_};
			text_.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
			text_.replace(QLatin1Char('\r'), QLatin1Char('\n'));

			QVector<QString> lines_;
			for (const QString &field_
				 : text_.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
				lines_ << field_;
			}
			return lines_;
		}
	}
	return QVector<QString>();
}

/**
 * @brief RealTerminal::cableTargetIsAutomatic
 * @return true when the target of this terminal is worked out from the
 * project -- one of its cables lands on a piece worth naming -- and
 * false when only a text typed in by hand is there to show.
 */
bool RealTerminal::cableTargetIsAutomatic() const
{
	return !automaticCableTarget().isEmpty();
}

/**
 * @brief RealTerminal::automaticCableTarget
 * Where the cable of this terminal goes at its far end: the
 * Installation, Location and Label (BMK) of the first piece the
 * wires of the cable land on, past the strip's own terminals, in
 * plan order -- the same three fields the cable list shows for a
 * target, in the same order. An empty field is left out; a terminal
 * with no cable, or a cable which lands on nothing else, gives no
 * lines at all.
 *
 * The cable is the one the wires drawn to this terminal carry; a
 * cable name typed by hand, with no wire behind it, is looked up by
 * its name among the cables of the project.
 * @return the lines to write in the box under the cable
 */
QVector<QString> RealTerminal::automaticCableTarget() const
{
	if (m_spare || m_element.isNull()) {
		return QVector<QString>();
	}
	TerminalStrip *strip_ = parentStrip();
	if (!strip_ || !strip_->project()) {
		return QVector<QString>();
	}
	QETProject *project_{strip_->project()};

		//The cable the wires drawn to this terminal are part of
	QUuid cable_uuid_;
	for (Terminal *terminal_ : m_element->terminals())
	{
		if (!terminal_) {
			continue;
		}
		for (Conductor *conductor_ : terminal_->conductors())
		{
			if (conductor_ && !conductor_->properties().m_cable_uuid.isNull()) {
				cable_uuid_ = conductor_->properties().m_cable_uuid;
				break;
			}
		}
		if (!cable_uuid_.isNull()) {
			break;
		}
	}
	if (cable_uuid_.isNull())
	{
			//A cable name typed by hand, no wire behind it: the first
			//cable of the project drawn under that name
		const auto name_{cable()};
		if (name_.isEmpty()) {
			return QVector<QString>();
		}
		for (Cable *cable_ : project_->cables())
		{
			if (cable_ && cable_->designation() == name_) {
				cable_uuid_ = cable_->uuid();
				break;
			}
		}
		if (cable_uuid_.isNull()) {
			return QVector<QString>();
		}
	}

		//The own side: this strip's terminals never are the target
	QSet<QUuid> own_;
	for (const auto &physical_ : strip_->physicalTerminal())
	{
		if (!physical_) {
			continue;
		}
		for (const auto &real_ : physical_->realTerminals()) {
			if (real_ && real_->element()) {
				own_.insert(real_->element()->uuid());
			}
		}
	}

		//Where the wires of the cable land: one piece whatever the
		//number of its cores, never a report (the cable continues on
		//the sheet the report points to) and never the strip's own
		//terminals. Same rule as the cable list (cablelistrows.cpp).
	QHash<QUuid, Element *> landed_;
	for (Diagram *diagram_ : project_->diagrams())
	{
		for (Conductor *conductor_ : diagram_->conductors())
		{
			if (!conductor_ || conductor_->properties().m_cable_uuid != cable_uuid_) {
				continue;
			}
			Terminal *ends_[2] = {conductor_->terminal1, conductor_->terminal2};
			for (Terminal *terminal_ : ends_)
			{
				if (!terminal_) {
					continue;
				}
				Element *element_ = terminal_->parentElement();
				if (!element_ || (element_->linkType() & Element::AllReport)) {
					continue;
				}
				if (own_.contains(element_->uuid())) {
					continue;
				}
				landed_.insert(element_->uuid(), element_);
			}
		}
	}
	if (landed_.isEmpty()) {
		return QVector<QString>();
	}

		//The first piece in plan order is the target, as the cable
		//list chooses it
	Element *target_{nullptr};
	for (Element *element_ : std::as_const(landed_)) {
		if (!target_ || comparPos(element_, target_)) {
			target_ = element_;
		}
	}

	const DiagramContext informations_{target_->elementInformations()};
	const auto installation_{informations_.value(QETInformation::ELMT_PLANT).toString()};
	const auto location_{informations_.value(QETInformation::ELMT_LOCATION).toString()};
	const auto label_{informations_.value(QETInformation::ELMT_LABEL).toString()};

	QVector<QString> lines_;
	if (!installation_.isEmpty()) {
		lines_ << installation_;
	}
	if (!location_.isEmpty()) {
		lines_ << location_;
	}
	if (!label_.isEmpty()) {
		lines_ << label_;
	}
	return lines_;
}

/**
 * @brief RealTerminal::color
 * @return The color set by the user for this terminal, drawn in the
 * terminal strip. An invalid color mean that the user did not set a color.
 * The value is stored in the informations of the element.
 * @sa stripeColor
 */
QColor RealTerminal::color() const
{
	QColor main_, stripe_;
	if (hasData()) {
		TerminalColor::parse(terminalData().m_informations.value(colorInfoKey()).toString(),
							 &main_, &stripe_);
	}
	return main_;
}

/**
 * @brief RealTerminal::stripeColor
 * @return The second color of a two-tone color (green/yellow for example)
 * or an invalid color if the color of this terminal is plain or not set.
 */
QColor RealTerminal::stripeColor() const
{
	QColor main_, stripe_;
	if (hasData()) {
		TerminalColor::parse(terminalData().m_informations.value(colorInfoKey()).toString(),
							 &main_, &stripe_);
	}
	return stripe_;
}

/**
 * @brief RealTerminal::section
 * @return The section in mm2 set by the user for this terminal
 * or 0 if no section was set.
 * The value is stored in the informations of the element,
 * with the dot as decimal separator, whatever the locale.
 */
qreal RealTerminal::section() const
{
	if (hasData())
	{
		bool ok_ = false;
		const auto section_ = terminalData().m_informations.value(sectionInfoKey()).toString().toDouble(&ok_);
		if (ok_ && section_ > 0) {
			return section_;
		}
	}
	return 0;
}

/**
 * @brief RealTerminal::widthMm
 * @return The width in mm of this terminal (the width of the terminal
 * sold by the manufacturer), set by the user, or 0 if no width was set.
 * The value is stored in the informations of the element,
 * with the dot as decimal separator, whatever the locale.
 */
qreal RealTerminal::widthMm() const
{
	if (hasData())
	{
		bool ok_ = false;
		const auto width_ = terminalData().m_informations.value(widthInfoKey()).toString().toDouble(&ok_);
		if (ok_ && width_ > 0) {
			return width_;
		}
	}
	return 0;
}

/**
 * @brief RealTerminal::catalogEntry
 * @return the label of the terminal chosen in the catalog
 * (TerminalCatalog::WidthEntry::label), for example "Phoenix Contact ST 4",
 * or an empty string if the width was typed or not set.
 * Several terminals of the catalog can have the same width,
 * the width alone doesn't say which one was chosen.
 */
QString RealTerminal::catalogEntry() const
{
	if (hasData()) {
		return terminalData().m_informations.value(catalogEntryInfoKey()).toString();
	}
	return QString();
}

/**
 * @brief RealTerminal::applyCatalogEntry
 * Store in @a informations the terminal chosen in the catalog,
 * and write its manufacturer and its reference in the manufacturer and
 * manufacturer reference informations (used by the bill of materials).
 * These two informations are only written when they are empty,
 * or when they still hold what the previous entry of the catalog wrote :
 * a value typed by the user is never overwritten.
 * @param informations : the informations of the element
 * @param entry_label : label of an entry of the catalog,
 * an empty string (or a label unknown by the catalog) removes the entry.
 */
void RealTerminal::applyCatalogEntry(DiagramContext &informations, const QString &entry_label)
{
	const auto *old_entry = TerminalCatalog::entryByLabel(
				informations.value(catalogEntryInfoKey()).toString());
	const auto *new_entry = TerminalCatalog::entryByLabel(entry_label);

	if (new_entry) {
		informations.addValue(catalogEntryInfoKey(), QString::fromUtf8(new_entry->label));
	} else if (informations.contains(catalogEntryInfoKey())) {
		informations.remove(catalogEntryInfoKey());
	}

	const auto update_ = [&informations](const QString &key, const QString &old_value, const QString &new_value)
	{
		const auto current_ = informations.value(key).toString();
		if (!current_.isEmpty() && current_ != old_value) {
			return;	//Typed by the user, keep it
		}
		if (!new_value.isEmpty()) {
			informations.addValue(key, new_value);
		} else if (!current_.isEmpty()) {
			informations.addValue(key, QString());
		}
	};

	update_(QETInformation::ELMT_MANUFACTURER,
			old_entry ? QString::fromUtf8(old_entry->manufacturer) : QString(),
			new_entry ? QString::fromUtf8(new_entry->manufacturer) : QString());
	update_(QETInformation::ELMT_MANUFACTURER_REF,
			old_entry ? TerminalCatalog::modelName(*old_entry) : QString(),
			new_entry ? TerminalCatalog::modelName(*new_entry) : QString());
}

/**
 * @brief RealTerminal::hasSeparatorAfter
 * @return true if a separator plate (partition plate) is mounted
 * after this terminal, between this terminal and the next one of the strip.
 * Used to separate groups of terminals (voltages, functions...).
 */
bool RealTerminal::hasSeparatorAfter() const {
	if (hasData()) {
		return QET::infoFlagIsTrue(
			terminalData().m_informations.value(separatorInfoKey()).toString());
	}
	return false;
}

/**
 * @brief RealTerminal::conductor
 * @return
 */
QString RealTerminal::conductor() const {
	if (m_element)
	{
		const auto conductors_{m_element->conductors()};
		if (conductors_.size()) {
			return conductors_.first()->properties().text;
		}
	}
	return QString();
}

/**
 * @brief RealTerminal::type
 * @return
 */
ElementData::TerminalType RealTerminal::type() const {
	if (hasData()) {
		return terminalData().terminalType();
	} else {
		return ElementData::TTGeneric;
	}
}

/**
 * @brief RealTerminal::function
 * @return
 */
ElementData::TerminalFunction RealTerminal::function() const {
	if (hasData()) {
		return terminalData().terminalFunction();
	} else {
		return ElementData::TFGeneric;
	}
}

/**
 * @brief RealTerminal::isLed
 * @return
 */
bool RealTerminal::isLed() const {
	if (hasData()) {
		return terminalData().terminalLed();
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
	if (parentStrip()) {
		return !parentStrip()->isBridged(m_this_weak.toStrongRef()).isNull();
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
	if (parentStrip()) {
		return parentStrip()->isBridged(m_this_weak.toStrongRef());
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
	if (m_spare) {
		return m_spare_uuid;	//Used to find the terminal (bridges...) like an element uuid
	}
	if (!m_element.isNull()) {
		return m_element->uuid();
	} else {
		return QUuid();
	}
}

/**
 * @brief RealTerminal::RealTerminal::xmlTagName
 * @return
 */
QString RealTerminal::RealTerminal::xmlTagName() {
	return QStringLiteral("real_terminal");
}
