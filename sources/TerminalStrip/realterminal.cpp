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
#include "../qetinformation.h"

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
QString RealTerminal::cable() const {
	if (hasData()) {
		return terminalData().m_informations.value(cableInfoKey()).toString();
	}
	return QString();
}

/**
 * @brief RealTerminal::cableWire
 * @return The color / number of the wire of the cable, connected to this terminal.
 */
QString RealTerminal::cableWire() const {
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
