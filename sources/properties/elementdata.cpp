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
#include "elementdata.h"
#include "../qetxml.h"
#include <QDebug>

void ElementData::toSettings(QSettings &settings, const QString prefix) const {
	Q_UNUSED(settings)
	Q_UNUSED(prefix)
}

void ElementData::fromSettings(const QSettings &settings, const QString prefix) {
	Q_UNUSED(settings)
	Q_UNUSED(prefix)
}

QDomElement ElementData::toXml(QDomDocument &xml_element) const {
	Q_UNUSED(xml_element)
	return QDomElement();
}

/**
 * @brief ElementData::fromXml
 * load properties from xml element.
 * The tag name of xml_element must be definition
 * and have an attribute "type"
 * @param xml_element : tagName must be 'definition'
 * @return true is successfuly loaded
 */
bool ElementData::fromXml(const QDomElement &xml_element)
{
	if(xml_element.tagName() != "definition" ||
	   xml_element.attribute("type") != "element") {
		return false;
	}

	m_type = typeFromString(xml_element.attribute("link_type", "simple"));
	kindInfoFromXml(xml_element);
	m_informations.fromXml(xml_element.firstChildElement("elementInformations"),
							"elementInformation");
	m_names_list.fromXml(xml_element);

	auto xml_draw_info = xml_element.firstChildElement("informations");
	if(xml_draw_info.tagName() == "informations") {
		m_drawing_information = xml_draw_info.text();
	}

	return true;
}

QDomElement ElementData::kindInfoToXml(QDomDocument &document)
{
		//kindInformations
	auto returned_elmt = document.createElement("kindInformations");

	if (m_type == ElementData::Master)
	{
		auto xml_type  = document.createElement("kindInformation");
		xml_type.setAttribute("name", "type");
		auto type_txt = document.createTextNode(masterTypeToString(m_master_type));
		xml_type.appendChild(type_txt);

		returned_elmt.appendChild(xml_type);
	}
	else if (m_type == ElementData::Slave)
	{
			//type
		auto xml_type  = document.createElement("kindInformation");
		xml_type.setAttribute("name", "type");
		auto type_txt = document.createTextNode(slaveTypeToString(m_slave_type));
		xml_type.appendChild(type_txt);
		returned_elmt.appendChild(xml_type);

			//state
		auto xml_state = document.createElement("kindInformation");
		xml_state.setAttribute("name", "state");
		auto state_txt = document.createTextNode(slaveStateToString(m_slave_state));
		xml_state.appendChild(state_txt);

		returned_elmt.appendChild(xml_state);

			//contact count
		auto xml_count = document.createElement("kindInformation");
		xml_count.setAttribute("name", "number");
		auto count_txt = document.createTextNode(QString::number(m_contact_count));
		xml_count.appendChild(count_txt);

		returned_elmt.appendChild(xml_count);
	}
	else if (m_type == ElementData::Terminale)
	{
			//type
		auto xml_type  = document.createElement("kindInformation");
		xml_type.setAttribute("name", "type");
		auto type_txt = document.createTextNode(terminalTypeToString(m_terminal_type));
		xml_type.appendChild(type_txt);
		returned_elmt.appendChild(xml_type);

			//function
		auto xml_func = document.createElement("kindInformation");
		xml_func.setAttribute("name", "function");
		auto func_txt = document.createTextNode(terminalFunctionToString(m_terminal_function));
		xml_func.appendChild(func_txt);
		returned_elmt.appendChild(xml_func);
	}

	return returned_elmt;
}

/**
 * @brief ElementData::setTerminalType
 * Override the terminal type by \p t_type
 * @param t_type
 */
void ElementData::setTerminalType(ElementData::TerminalType t_type)
{
	if (t_type == m_terminal_type) {
		m_terminal_type_is_override = false;
	} else {
		m_override_terminal_type = t_type;
		m_terminal_type_is_override = true;
	}
}

/**
 * @brief ElementData::terminalType
 * @return the terminal type or overrided terminal type if set
 */
ElementData::TerminalType ElementData::terminalType() const
{
	return m_terminal_type_is_override ?
				m_override_terminal_type :
				m_terminal_type;
}

/**
 * @brief ElementData::setTerminalFunction
 * Override the terminal function by \p t_function
 * @param t_function
 */
void ElementData::setTerminalFunction(ElementData::TerminalFunction t_function)
{
	if (t_function == m_terminal_function) {
		m_terminal_function_is_override = false;
	} else {
		m_override_terminal_function = t_function;
		m_terminal_function_is_override = true;
	}
}

/**
 * @brief ElementData::terminalFunction
 * @return the terminal function or overrided terminal function if set
 */
ElementData::TerminalFunction ElementData::terminalFunction() const
{
	return m_terminal_function_is_override ?
				m_override_terminal_function :
				m_terminal_function;
}

/**
 * @brief ElementData::setTerminalLED
 * Override the terminal led by \p led
 * @param led
 */
void ElementData::setTerminalLED(bool led)
{
	if (led == m_terminal_led) {
		m_terminal_led_is_override = false;
	} else {
		m_override_terminal_led = led;
		m_terminal_led_is_override = true;
	}
}

/**
 * @brief ElementData::terminalLed
 * @return if terminal have led or overrided led if set
 */
bool ElementData::terminalLed() const
{
	return m_terminal_led_is_override ?
				m_override_terminal_led :
				m_terminal_led;
}

/**
 * @brief ElementData::terminalPropertiesIsOverrided
 * @return true if at least one sub properties of terminal type is overrided
 */
bool ElementData::terminalPropertiesIsOverrided() const
{
	if (m_terminal_type_is_override
		|| m_terminal_function_is_override
		|| m_terminal_led_is_override) {
		return true;
	}

	return false;
}

bool ElementData::operator==(const ElementData &data) const
{
	if (data.m_type != m_type) {
		return false;
	}

	if (data.m_type == ElementData::Master) {
		if(data.m_master_type != m_master_type) {
			return false;
		}
	}
	else if (data.m_type == ElementData::Slave) {
		if (data.m_slave_state   != m_slave_state ||
			data.m_slave_type    != m_slave_type  ||
			data.m_contact_count != m_contact_count) {
			return false;
		}
	}
	else if (data.m_type == ElementData::Terminale) {
			//Check terminal type or overrided terminal type
		if (data.m_terminal_type_is_override != m_terminal_type_is_override) {
			return false;
		}

		if (m_terminal_type_is_override) {
			if(data.m_override_terminal_type != m_override_terminal_type) {
				return false;
			}
		} else if (data.m_terminal_type != m_terminal_type) {
			return false;
		}

			//Check terminal led or override terminal led
		if (data.m_terminal_led_is_override != m_terminal_led_is_override) {
			return false;
		}
		if (m_terminal_led_is_override) {
			if (data.m_override_terminal_led != m_override_terminal_led) {
			return false;
			}
		} else if (data.m_terminal_led != m_terminal_led) {
			return false;
		}

			//Check terminal function or overrided terminal function
		if (data.m_terminal_function_is_override != m_terminal_function_is_override) {
			return false;
		}
		if (m_terminal_function_is_override) {
			if (data.m_override_terminal_function != m_override_terminal_function) {
				return false;
			}
		}
		else if (data.m_terminal_function != m_terminal_function) {
			return false;
		}
	}

	if(data.m_informations != m_informations) {
		return false;
	}

	if (data.m_names_list != m_names_list) {
		return false;
	}

	if (m_drawing_information != m_drawing_information) {
		return false;
	}

	return true;
}

bool ElementData::operator !=(const ElementData &data) const {
	return !(*this == data);
}

QString ElementData::typeToString(ElementData::Type type)
{
	switch (type) {
		case ElementData::Simple :
			return QString("simple");
		case ElementData::NextReport :
			return QString ("next_report");
		case ElementData::PreviousReport :
			return QString("previous_report");
		case ElementData::Master :
			return QString("master");
		case ElementData::Slave :
			return QString("slave");
		case ElementData::Terminale :
			return QString("terminal");
		default:
			qDebug() << "ElementData::typeToString : type don't exist"
					 << "return failsafe value 'simple'";
			return QString("simple");
	}
}

ElementData::Type ElementData::typeFromString(const QString &string)
{
	if (string == "simple") {
		return ElementData::Simple;
	} else if (string == "next_report") {
		return ElementData::NextReport;
	} else if (string == "previous_report") {
		return ElementData::PreviousReport;
	} else if (string == "master") {
		return ElementData::Master;
	} else if (string == "slave") {
		return ElementData::Slave;
	} else if (string == "terminal") {
		return ElementData::Terminale;
	}

		//Return simple if nothing match
	qDebug() << "ElementData::typeFromString : string "
			 << string
			 << " don't exist, return failsafe value 'simple";
	return ElementData::Simple;
}

QString ElementData::masterTypeToString(ElementData::MasterType type)
{
	switch (type) {
		case ElementData::Coil:
			return QString ("coil");
		case ElementData::Protection:
			return QString ("protection");
		case ElementData::Commutator:
			return QString ("commutator");
	}
}

ElementData::MasterType ElementData::masterTypeFromString(const QString &string)
{
	if (string == "coil") {
		return ElementData::Coil;
	} else if (string == "protection") {
		return  ElementData::Protection;
	} else if (string == "commutator") {
		return ElementData::Commutator;
	}

	qDebug() << "ElementData::masterTypeFromString : string "
			 << string
			 << " don't exist, return failsafe value 'coil'";
	return ElementData::Coil;
}

QString ElementData::slaveTypeToString(ElementData::SlaveType type)
{
	switch (type) {
		case ElementData::SSimple:
			return QString ("simple");
		case ElementData::Power:
			return QString ("power");
		case ElementData::DelayOn:
			return QString("delayOn");
		case ElementData::DelayOff:
			return  QString("delayOff");
		case ElementData::delayOnOff:
			return QString("delayOnOff");
	}
}

ElementData::SlaveType ElementData::slaveTypeFromString(const QString &string)
{
	if (string == "simple") {
		return ElementData::SSimple;
	} else if (string == "power") {
		return  ElementData::Power;
	} else if (string == "delayOn") {
		return ElementData::DelayOn;
	} else if (string == "delayOff") {
		return ElementData::DelayOff;
	} else if (string == "delayOnOff") {
		return ElementData::delayOnOff;
	}

	qDebug() << "ElementData::slaveTypeFromSting : string "
			 << string
			 << " don't exist, return failsafe value 'simple'";
	return ElementData::SSimple;
}

QString ElementData::slaveStateToString(ElementData::SlaveState type)
{
	switch (type) {
		case NO:
			return QString("NO");
		case NC:
			return QString("NC");
		case SW:
			return QString("SW");
	}
}

ElementData::SlaveState ElementData::slaveStateFromString(const QString &string)
{
	if (string == "NO") {
		return ElementData::NO;
	} else if (string == "NC") {
		return ElementData::NC;
	} else if (string == "SW") {
		return ElementData::SW;
	}

	qDebug() << "ElementData::slaveStateFromString : string : "
			 << string
			 << " don't exist, return failsafe value 'NO'";
	return ElementData::NO;
}

QString ElementData::terminalTypeToString(ElementData::TerminalType type)
{
	switch (type) {
		case ElementData::TTGeneric :
			return QStringLiteral("generic");
		case ElementData::TTFuse :
			return  QStringLiteral("fuse");
		case ElementData::TTSectional:
			return QStringLiteral("sectional");
		case ElementData::TTDiode:
			return QStringLiteral("diode");
		case ElementData::TTGround:
			return QStringLiteral("ground");
	}
}

ElementData::TerminalType ElementData::terminalTypeFromString(const QString &string)
{
	if (string == QLatin1String("generic")) {
		return ElementData::TTGeneric;
	} else if (string == QLatin1String("fuse")) {
		return ElementData::TTFuse;
	} else if (string == QLatin1String("sectional")) {
		return ElementData::TTSectional;
	} else if (string == QLatin1String("diode")) {
		return ElementData::TTDiode;
	} else if (string == QLatin1String("ground")) {
		return ElementData::TTGround;
	}

	qDebug() << "ElementData::terminalTypeFromString : string : "
			 << string
			 << " don't exist, return failsafe value 'generic'";
	return ElementData::TTGeneric;
}

QString ElementData::translatedTerminalType(ElementData::TerminalType type)
{
	switch (type) {
		case ElementData::TTGeneric :
			return QObject::tr("Générique", "generic terminal element type");
		case ElementData::TTFuse :
			return  QObject::tr("Fusible", "fuse terminal element type");
		case ElementData::TTSectional:
			return QObject::tr("Sectionable", "sectional terminal element type");
		case ElementData::TTDiode:
			return QObject::tr("Diode", "diode terminal element type");
		case ElementData::TTGround:
			return QObject::tr("Terre", "ground terminal element type");
	}
}

QString ElementData::terminalFunctionToString(ElementData::TerminalFunction function)
{
	switch (function) {
		case ElementData::TFGeneric:
			return QString("generic");
		case ElementData::TFPhase:
			return QString ("phase");
		case ElementData::TFNeutral:
			return QString("neutral");
	}
}

ElementData::TerminalFunction ElementData::terminalFunctionFromString(const QString &string)
{
	if (string == "generic") {
		return ElementData::TFGeneric;
	} else if (string == "phase") {
		return ElementData::TFPhase;
	} else if (string == "neutral") {
		return ElementData::TFNeutral;
	}

	qDebug() << "ElementData::terminalFunctionFromString : string : "
			 << string
			 << " don't exist, return failsafe value 'generic'";
	return ElementData::TFGeneric;
}

QString ElementData::translatedTerminalFunction(ElementData::TerminalFunction function)
{
	switch (function) {
		case TFGeneric : return QObject::tr("Générique", "generic terminal element function");
		case TFPhase :   return QObject::tr("Phase", "phase terminal element function" );
		case TFNeutral : return QObject::tr("Neutre", "neutral terminal element function");
	}
}

void ElementData::kindInfoFromXml(const QDomElement &xml_element)
{
	if (m_type == ElementData::Master ||
		m_type == ElementData::Slave  ||
		m_type == ElementData::Terminale)
	{
		auto xml_kind = xml_element.firstChildElement("kindInformations");
		for (const auto &dom_elmt : QETXML::findInDomElement(xml_kind, "kindInformation"))
		{
			if (!dom_elmt.hasAttribute("name")) {
				continue;
			}
			auto name = dom_elmt.attribute("name");

			if (m_type == ElementData::Master &&
				name == "type") {
				m_master_type = masterTypeFromString(dom_elmt.text());
			}
			else if (m_type == ElementData::Slave ) {
				if (name == "type") {
					m_slave_type = slaveTypeFromString(dom_elmt.text());
				} else if (name == "state") {
					m_slave_state = slaveStateFromString(dom_elmt.text());
				} else if (name == "number") {
					m_contact_count = dom_elmt.text().toInt();
				}
			}
			else if (m_type == ElementData::Terminale) {
				if (name == "type") {
					m_terminal_type = terminalTypeFromString(dom_elmt.text());
				} else if (name == "function") {
					m_terminal_function = terminalFunctionFromString(dom_elmt.text());
				}
			}
		}
	}
}
