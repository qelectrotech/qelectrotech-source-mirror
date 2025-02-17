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
 * @return true is successfully loaded
 */
bool ElementData::fromXml(const QDomElement &xml_element)
{
	if(xml_element.tagName() != QLatin1String("definition") ||
	   xml_element.attribute(QStringLiteral("type")) != QLatin1String("element")) {
		return false;
	}

	m_type = typeFromString(xml_element.attribute(QStringLiteral("link_type"), QStringLiteral("simple")));
	kindInfoFromXml(xml_element);
	m_informations.fromXml(xml_element.firstChildElement(QStringLiteral("elementInformations")),
							QStringLiteral("elementInformation"));
	m_names_list.fromXml(xml_element);

	auto xml_draw_info = xml_element.firstChildElement(QStringLiteral("informations"));
	if(xml_draw_info.tagName() == QLatin1String("informations")) {
		m_drawing_information = xml_draw_info.text();
	}

	return true;
}

QDomElement ElementData::kindInfoToXml(QDomDocument &document)
{
		//kindInformations
	auto returned_elmt = document.createElement(QStringLiteral("kindInformations"));

	if (m_type == ElementData::Master)
	{
		auto xml_type  = document.createElement(QStringLiteral("kindInformation"));
		xml_type.setAttribute(QStringLiteral("name"), QStringLiteral("type"));
		auto type_txt = document.createTextNode(masterTypeToString(m_master_type));
		xml_type.appendChild(type_txt);

		returned_elmt.appendChild(xml_type);
	}
	else if (m_type == ElementData::Slave)
	{
			//type
		auto xml_type  = document.createElement(QStringLiteral("kindInformation"));
		xml_type.setAttribute(QStringLiteral("name"), QStringLiteral("type"));
		auto type_txt = document.createTextNode(slaveTypeToString(m_slave_type));
		xml_type.appendChild(type_txt);
		returned_elmt.appendChild(xml_type);

			//state
		auto xml_state = document.createElement(QStringLiteral("kindInformation"));
		xml_state.setAttribute(QStringLiteral("name"), QStringLiteral("state"));
		auto state_txt = document.createTextNode(slaveStateToString(m_slave_state));
		xml_state.appendChild(state_txt);

		returned_elmt.appendChild(xml_state);

			//contact count
		auto xml_count = document.createElement(QStringLiteral("kindInformation"));
		xml_count.setAttribute(QStringLiteral("name"), QStringLiteral("number"));
		auto count_txt = document.createTextNode(QString::number(m_contact_count));
		xml_count.appendChild(count_txt);

		returned_elmt.appendChild(xml_count);
	}
	else if (m_type == ElementData::Terminal)
	{
			//type
		auto xml_type  = document.createElement(QStringLiteral("kindInformation"));
		xml_type.setAttribute(QStringLiteral("name"), QStringLiteral("type"));
		auto type_txt = document.createTextNode(terminalTypeToString(m_terminal_type));
		xml_type.appendChild(type_txt);
		returned_elmt.appendChild(xml_type);

			//function
		auto xml_func = document.createElement(QStringLiteral("kindInformation"));
		xml_func.setAttribute(QStringLiteral("name"), QStringLiteral("function"));
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
	else if (data.m_type == ElementData::Terminal) {
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

QString ElementData::typeToString() const {
	return typeToString(m_type);
}

QString ElementData::typeToString(ElementData::Type type)
{
	switch (type) {
		case ElementData::Simple :
			return QStringLiteral("simple");
		case ElementData::NextReport :
			return QStringLiteral("next_report");
		case ElementData::PreviousReport :
			return QStringLiteral("previous_report");
		case ElementData::Master :
			return QStringLiteral("master");
		case ElementData::Slave :
			return QStringLiteral("slave");
		case ElementData::Terminal :
			return QStringLiteral("terminal");
		case ElementData::Thumbnail:
			return  QStringLiteral("thumbnail");
		default:
			qDebug() << "ElementData::typeToString : type don't exist"
					 << "return failsafe value 'simple'";
			return QStringLiteral("simple");
	}
}

ElementData::Type ElementData::typeFromString(const QString &string)
{
	if (string == QLatin1String("simple")) {
		return ElementData::Simple;
	} else if (string == QLatin1String("next_report")) {
		return ElementData::NextReport;
	} else if (string == QLatin1String("previous_report")) {
		return ElementData::PreviousReport;
	} else if (string == QLatin1String("master")) {
		return ElementData::Master;
	} else if (string == QLatin1String("slave")) {
		return ElementData::Slave;
	} else if (string == QLatin1String("terminal")) {
		return ElementData::Terminal;
	} else if (string == QLatin1String("thumbnail")) {
		return ElementData::Thumbnail;
	}

		//Return simple if nothing match
	qDebug() << "ElementData::typeFromString : string "
			 << string
			 << " don't exist, return failsafe value 'simple";
	return ElementData::Simple;
}

QString ElementData::masterTypeToString() const {
	if (m_type == Master)
		return masterTypeToString(m_master_type);
	else
		return QLatin1String();
}

QString ElementData::masterTypeToString(ElementData::MasterType type)
{
	switch (type) {
		case ElementData::Coil:
			return QStringLiteral("coil");
		case ElementData::Protection:
			return QStringLiteral("protection");
		case ElementData::Commutator:
			return QStringLiteral("commutator");
	}
	return QStringLiteral("coil");
}

ElementData::MasterType ElementData::masterTypeFromString(const QString &string)
{
	if (string == QLatin1String("coil")) {
		return ElementData::Coil;
	} else if (string == QLatin1String("protection")) {
		return  ElementData::Protection;
	} else if (string == QLatin1String("commutator")) {
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
			return QStringLiteral("simple");
		case ElementData::Power:
			return QStringLiteral("power");
		case ElementData::DelayOn:
			return QStringLiteral("delayOn");
		case ElementData::DelayOff:
			return  QStringLiteral("delayOff");
		case ElementData::delayOnOff:
			return QStringLiteral("delayOnOff");
	}
	return QStringLiteral("simple");
}

ElementData::SlaveType ElementData::slaveTypeFromString(const QString &string)
{
	if (string == QLatin1String("simple")) {
		return ElementData::SSimple;
	} else if (string == QLatin1String("power")) {
		return  ElementData::Power;
	} else if (string == QLatin1String("delayOn")) {
		return ElementData::DelayOn;
	} else if (string == QLatin1String("delayOff")) {
		return ElementData::DelayOff;
	} else if (string == QLatin1String("delayOnOff")) {
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
			return QStringLiteral("NO");
		case NC:
			return QStringLiteral("NC");
		case SW:
			return QStringLiteral("SW");
		case Other:
			return QStringLiteral("Other");
	}
	return QStringLiteral("NO");
}

ElementData::SlaveState ElementData::slaveStateFromString(const QString &string)
{
	if (string == QLatin1String("NO")) {
		return ElementData::NO;
	} else if (string == QLatin1String("NC")) {
		return ElementData::NC;
	} else if (string == QLatin1String("SW")) {
		return ElementData::SW;
	} else if (string == QLatin1String("Other")){
		return ElementData::Other;
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
	return QStringLiteral("generic");
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
	return QObject::tr("Générique", "generic terminal element type");
}

QString ElementData::terminalFunctionToString(ElementData::TerminalFunction function)
{
	switch (function) {
		case ElementData::TFGeneric:
			return QStringLiteral("generic");
		case ElementData::TFPhase:
			return QStringLiteral ("phase");
		case ElementData::TFNeutral:
			return QStringLiteral("neutral");
	}
	return QStringLiteral("generic");
}

ElementData::TerminalFunction ElementData::terminalFunctionFromString(const QString &string)
{
	if (string == QLatin1String("generic")) {
		return ElementData::TFGeneric;
	} else if (string == QLatin1String("phase")) {
		return ElementData::TFPhase;
	} else if (string == QLatin1String("neutral")) {
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
	return QObject::tr("Générique", "generic terminal element function");
}

void ElementData::kindInfoFromXml(const QDomElement &xml_element)
{
	if (m_type == ElementData::Master ||
		m_type == ElementData::Slave  ||
		m_type == ElementData::Terminal)
	{
		auto xml_kind = xml_element.firstChildElement(QStringLiteral("kindInformations"));
		for (const auto &dom_elmt : QETXML::findInDomElement(xml_kind, QStringLiteral("kindInformation")))
		{
			if (!dom_elmt.hasAttribute(QStringLiteral("name"))) {
				continue;
			}
			auto name = dom_elmt.attribute(QStringLiteral("name"));

			if (m_type == ElementData::Master &&
				name == QLatin1String("type")) {
				m_master_type = masterTypeFromString(dom_elmt.text());
			}
			else if (m_type == ElementData::Slave ) {
				if (name == QLatin1String("type")) {
					m_slave_type = slaveTypeFromString(dom_elmt.text());
				} else if (name == QLatin1String("state")) {
					m_slave_state = slaveStateFromString(dom_elmt.text());
				} else if (name == QLatin1String("number")) {
					m_contact_count = dom_elmt.text().toInt();
				}
			}
			else if (m_type == ElementData::Terminal) {
				if (name == QLatin1String("type")) {
					m_terminal_type = terminalTypeFromString(dom_elmt.text());
				} else if (name == QLatin1String("function")) {
					m_terminal_function = terminalFunctionFromString(dom_elmt.text());
				}
				
			}
		}
	}
}
