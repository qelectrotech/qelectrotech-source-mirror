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

		// Save max_slaves only if a specific limit is set (not default -1)
		if (m_max_slaves != -1) {
			auto xml_max_slaves = document.createElement(QStringLiteral("kindInformation"));
			xml_max_slaves.setAttribute(QStringLiteral("name"), QStringLiteral("max_slaves"));
			auto max_slaves_txt = document.createTextNode(QString::number(m_max_slaves));
			xml_max_slaves.appendChild(max_slaves_txt);

			returned_elmt.appendChild(xml_max_slaves);
		}

		// Save slave contact groups if enabled
		if (m_slave_contact_groups_enabled) {
			auto xml_groups = document.createElement(QStringLiteral("slaveContactGroups"));
			for (const auto &group : m_slave_contact_groups) {
				auto xml_group = document.createElement(QStringLiteral("group"));
				xml_group.setAttribute(QStringLiteral("type"),
					slaveStateToString(group.type));
				xml_group.setAttribute(QStringLiteral("subtype"),
					slaveTypeToString(group.subtype));
				xml_group.setAttribute(QStringLiteral("contactCount"),
					group.contactCount);
				xml_group.setAttribute(QStringLiteral("terminalCount"),
					group.terminalCount);

				for (const auto &label : group.labels) {
					auto xml_label = document.createElement(QStringLiteral("label"));
					auto label_txt = document.createTextNode(label);
					xml_label.appendChild(label_txt);
					xml_group.appendChild(xml_label);
				}
				xml_groups.appendChild(xml_group);
			}
			returned_elmt.appendChild(xml_groups);
		}

		// Save PLC master data if type is PLC
		if (m_master_type == ElementData::PLC) {
		auto xml_plc = document.createElement(QStringLiteral("plcMasterData"));
		xml_plc.setAttribute(QStringLiteral("rowHeight"),
			QString::number(m_plc_master_data.rowHeight, 'f', 2));

		// Save break positions
		{
			auto xml_breaks = document.createElement(QStringLiteral("breakPositions"));
			for (int bp : m_plc_master_data.breakPositions) {
				auto xml_bp = document.createElement(QStringLiteral("break"));
				xml_bp.appendChild(document.createTextNode(QString::number(bp)));
				xml_breaks.appendChild(xml_bp);
			}
			xml_plc.appendChild(xml_breaks);
		}

			// Save column widths
			auto xml_col_widths = document.createElement(QStringLiteral("columnWidths"));
			for (auto it = m_plc_master_data.colWidths.constBegin();
				 it != m_plc_master_data.colWidths.constEnd(); ++it) {
				auto xml_col = document.createElement(QStringLiteral("column"));
				xml_col.setAttribute(QStringLiteral("index"), it.key());
				xml_col.setAttribute(QStringLiteral("width"), QString::number(it.value(), 'f', 2));
				xml_col_widths.appendChild(xml_col);
			}
			xml_plc.appendChild(xml_col_widths);

			// Save column visibility
			auto xml_col_vis = document.createElement(QStringLiteral("columnVisibility"));
			for (auto it = m_plc_master_data.colVisible.constBegin();
				 it != m_plc_master_data.colVisible.constEnd(); ++it) {
				auto xml_col = document.createElement(QStringLiteral("column"));
				xml_col.setAttribute(QStringLiteral("index"), it.key());
				xml_col.setAttribute(QStringLiteral("visible"), it.value() ? "true" : "false");
				xml_col_vis.appendChild(xml_col);
			}
			xml_plc.appendChild(xml_col_vis);

			// Save fonts
			if (!m_plc_master_data.headerFont.family().isEmpty()) {
				auto xml_hfont = document.createElement(QStringLiteral("headerFont"));
				xml_hfont.setAttribute(QStringLiteral("family"), m_plc_master_data.headerFont.family());
				xml_hfont.setAttribute(QStringLiteral("size"), m_plc_master_data.headerFont.pointSize());
				xml_hfont.setAttribute(QStringLiteral("bold"), m_plc_master_data.headerFont.bold() ? "true" : "false");
				xml_plc.appendChild(xml_hfont);
			}
			if (!m_plc_master_data.cellFont.family().isEmpty()) {
				auto xml_cfont = document.createElement(QStringLiteral("cellFont"));
				xml_cfont.setAttribute(QStringLiteral("family"), m_plc_master_data.cellFont.family());
				xml_cfont.setAttribute(QStringLiteral("size"), m_plc_master_data.cellFont.pointSize());
				xml_cfont.setAttribute(QStringLiteral("bold"), m_plc_master_data.cellFont.bold() ? "true" : "false");
				xml_plc.appendChild(xml_cfont);
			}

			// Save custom column names
			if (!m_plc_master_data.columnNames.isEmpty()) {
				auto xml_names = document.createElement(QStringLiteral("columnNames"));
				for (int i = 0; i < m_plc_master_data.columnNames.size(); ++i) {
					auto xml_name = document.createElement(QStringLiteral("column"));
					xml_name.setAttribute(QStringLiteral("index"), i);
					xml_name.appendChild(document.createTextNode(m_plc_master_data.columnNames.at(i)));
					xml_names.appendChild(xml_name);
				}
				xml_plc.appendChild(xml_names);
			}

			// Save column order
			if (!m_plc_master_data.columnOrder.isEmpty()) {
				auto xml_order = document.createElement(QStringLiteral("columnOrder"));
				QString order_str;
				for (int i = 0; i < m_plc_master_data.columnOrder.size(); ++i) {
					if (i > 0) order_str += QStringLiteral(",");
					order_str += QString::number(m_plc_master_data.columnOrder.at(i));
				}
				xml_order.appendChild(document.createTextNode(order_str));
				xml_plc.appendChild(xml_order);
			}

			// Save showHeaders
			{
				auto xml_sh = document.createElement(QStringLiteral("showHeaders"));
				xml_sh.appendChild(document.createTextNode(
					m_plc_master_data.showHeaders ? QStringLiteral("1") : QStringLiteral("0")));
				xml_plc.appendChild(xml_sh);
			}

			// Save IO entries
			auto xml_ios = document.createElement(QStringLiteral("plcIOs"));
			for (const auto &io : m_plc_master_data.ios) {
			auto xml_io = document.createElement(QStringLiteral("plcIO"));
			xml_io.setAttribute(QStringLiteral("type"), plcIOTypeToString(io.type));
			xml_io.setAttribute(QStringLiteral("address"), io.address);
			xml_io.setAttribute(QStringLiteral("functionText"), io.functionText);
			xml_io.setAttribute(QStringLiteral("comment"), io.comment);
			xml_io.setAttribute(QStringLiteral("crossRef"), io.crossRef);
			xml_io.setAttribute(QStringLiteral("terminalCount"), io.terminalCount);
			for (const auto &t : io.terminals) {
				auto xml_term = document.createElement(QStringLiteral("terminal"));
				xml_term.appendChild(document.createTextNode(t));
				xml_io.appendChild(xml_term);
			}
			xml_ios.appendChild(xml_io);
			}
			xml_plc.appendChild(xml_ios);

			returned_elmt.appendChild(xml_plc);
		}
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
		if (data.m_max_slaves != m_max_slaves) {
			return false;
		}
		if (data.m_slave_contact_groups_enabled != m_slave_contact_groups_enabled) {
			return false;
		}
		if (data.m_slave_contact_groups != m_slave_contact_groups) {
			return false;
		}
		if (data.m_master_type == ElementData::PLC) {
			if (data.m_plc_master_data != m_plc_master_data) {
				return false;
			}
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
		case ElementData::ConductorDefinition:
			return QStringLiteral("conductor_definition");
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
	} else if (string == QLatin1String("conductor_definition")) {
		return ElementData::ConductorDefinition;
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
		case ElementData::PLC:
			return QStringLiteral("plc");
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
	} else if (string == QLatin1String("plc")) {
		return ElementData::PLC;
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
		case ElementData::PLCSlave:
			return QStringLiteral("plc");
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
	} else if (string == QLatin1String("plc")) {
		return ElementData::PLCSlave;
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

			if (m_type == ElementData::Master) {
				if (name == QLatin1String("type")) {
					m_master_type = masterTypeFromString(dom_elmt.text());
				} else if (name == QLatin1String("max_slaves")) {
					m_max_slaves = dom_elmt.text().toInt();
				}
			}
			else if (m_type == ElementData::Slave) {
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

	// Parse slave contact groups for Master elements
	if (m_type == ElementData::Master) {
		auto xml_kind = xml_element.firstChildElement(QStringLiteral("kindInformations"));
		auto xml_groups = xml_kind.firstChildElement(QStringLiteral("slaveContactGroups"));
		if (!xml_groups.isNull()) {
			m_slave_contact_groups_enabled = true;
			auto group_list = QETXML::findInDomElement(xml_groups, QStringLiteral("group"));
			for (const auto &xml_group : group_list) {
				SlaveContactGroup group;
				group.type = slaveStateFromString(
					xml_group.attribute(QStringLiteral("type"), QStringLiteral("NO")));
				group.subtype = slaveTypeFromString(
					xml_group.attribute(QStringLiteral("subtype"), QStringLiteral("simple")));
				group.contactCount = xml_group.attribute(
					QStringLiteral("contactCount"), QStringLiteral("1")).toInt();
				group.terminalCount = xml_group.attribute(
					QStringLiteral("terminalCount"), QStringLiteral("1")).toInt();

				auto label_list = QETXML::findInDomElement(xml_group, QStringLiteral("label"));
				for (const auto &xml_label : label_list) {
					group.labels.append(xml_label.text());
				}
				m_slave_contact_groups.append(group);
			}
		}

		// Parse PLC master data
		auto xml_plc = xml_kind.firstChildElement(QStringLiteral("plcMasterData"));
		if (!xml_plc.isNull()) {
			m_plc_master_data.rowHeight = xml_plc.attribute(
				QStringLiteral("rowHeight"), QStringLiteral("8.00")).toDouble();

			// Parse break positions (new format: child elements)
			auto xml_breaks = xml_plc.firstChildElement(QStringLiteral("breakPositions"));
			if (!xml_breaks.isNull()) {
				for (const auto &xml_bp : QETXML::findInDomElement(xml_breaks, QStringLiteral("break"))) {
					m_plc_master_data.breakPositions.append(xml_bp.text().toInt());
				}
			} else {
				// Backward compat: old format was breakAfter attribute
				int old_break = xml_plc.attribute(
					QStringLiteral("breakAfter"), QStringLiteral("0")).toInt();
				m_plc_master_data.breakPositions.append(old_break > 0 ? old_break : 0);
			}

			// Parse column widths
			auto xml_col_widths = xml_plc.firstChildElement(QStringLiteral("columnWidths"));
			for (const auto &xml_col : QETXML::findInDomElement(xml_col_widths, QStringLiteral("column"))) {
				int index = xml_col.attribute(QStringLiteral("index")).toInt();
				qreal width = xml_col.attribute(QStringLiteral("width")).toDouble();
				m_plc_master_data.colWidths[index] = width;
			}

			// Parse column visibility
			auto xml_col_vis = xml_plc.firstChildElement(QStringLiteral("columnVisibility"));
			for (const auto &xml_col : QETXML::findInDomElement(xml_col_vis, QStringLiteral("column"))) {
				int index = xml_col.attribute(QStringLiteral("index")).toInt();
				bool visible = xml_col.attribute(QStringLiteral("visible")) == QLatin1String("true");
				m_plc_master_data.colVisible[index] = visible;
			}

			// Parse fonts
			auto xml_hfont = xml_plc.firstChildElement(QStringLiteral("headerFont"));
			if (!xml_hfont.isNull()) {
				m_plc_master_data.headerFont.setFamily(xml_hfont.attribute(QStringLiteral("family")));
				m_plc_master_data.headerFont.setPointSize(xml_hfont.attribute(QStringLiteral("size"), QStringLiteral("8")).toInt());
				m_plc_master_data.headerFont.setBold(xml_hfont.attribute(QStringLiteral("bold")) == QLatin1String("true"));
			}
			auto xml_cfont = xml_plc.firstChildElement(QStringLiteral("cellFont"));
			if (!xml_cfont.isNull()) {
				m_plc_master_data.cellFont.setFamily(xml_cfont.attribute(QStringLiteral("family")));
				m_plc_master_data.cellFont.setPointSize(xml_cfont.attribute(QStringLiteral("size"), QStringLiteral("8")).toInt());
				m_plc_master_data.cellFont.setBold(xml_cfont.attribute(QStringLiteral("bold")) == QLatin1String("true"));
			}

			// Parse custom column names
			auto xml_names = xml_plc.firstChildElement(QStringLiteral("columnNames"));
			if (!xml_names.isNull()) {
				for (const auto &xml_col : QETXML::findInDomElement(xml_names, QStringLiteral("column"))) {
					int index = xml_col.attribute(QStringLiteral("index")).toInt();
					if (index < 0 || index >= 5)
						continue;
					while (m_plc_master_data.columnNames.size() <= index)
						m_plc_master_data.columnNames.append(QString());
					m_plc_master_data.columnNames[index] = xml_col.text();
				}
			}

			// Parse column order
			auto xml_order = xml_plc.firstChildElement(QStringLiteral("columnOrder"));
			if (!xml_order.isNull()) {
				QStringList parts = xml_order.text().split(',');
				for (const auto &p : parts) {
					if (m_plc_master_data.columnOrder.size() >= 5)
						break;
					bool ok;
					int val = p.trimmed().toInt(&ok);
					if (ok)
						m_plc_master_data.columnOrder.append(val);
				}
			}

			// Parse showHeaders
			auto xml_sh = xml_plc.firstChildElement(QStringLiteral("showHeaders"));
			if (!xml_sh.isNull())
				m_plc_master_data.showHeaders = (xml_sh.text().trimmed() != QStringLiteral("0"));

			// Parse IO entries
			auto xml_ios = xml_plc.firstChildElement(QStringLiteral("plcIOs"));
			for (const auto &xml_io : QETXML::findInDomElement(xml_ios, QStringLiteral("plcIO"))) {
				if (m_plc_master_data.ios.size() >= 128)
					break;
				PlcIO io;
				io.type = plcIOTypeFromString(
					xml_io.attribute(QStringLiteral("type"), QStringLiteral("entree_digitale")));
				io.address = xml_io.attribute(QStringLiteral("address"));
				io.functionText = xml_io.attribute(QStringLiteral("functionText"));
				io.comment = xml_io.attribute(QStringLiteral("comment"));
				io.crossRef = xml_io.attribute(QStringLiteral("crossRef"));
				io.terminalCount = xml_io.attribute(
					QStringLiteral("terminalCount"), QStringLiteral("0")).toInt();
				for (const auto &xml_term : QETXML::findInDomElement(xml_io, QStringLiteral("terminal"))) {
					io.terminals.append(xml_term.text());
				}
				m_plc_master_data.ios.append(io);
			}
		}
	}
}

/**
 * @brief ElementData::slaveContactGroupTypeToString
 * Convert a SlaveState to string for XML storage in slave contact groups.
 * Maps: NO -> "NO", NC -> "NC", SW -> "SW", Other -> "Other"
 */
QString ElementData::slaveContactGroupTypeToString(ElementData::SlaveState type)
{
	return slaveStateToString(type);
}

/**
 * @brief ElementData::slaveContactGroupTypeFromString
 * Convert a string from XML to SlaveState for slave contact groups.
 */
ElementData::SlaveState ElementData::slaveContactGroupTypeFromString(const QString &string)
{
	return slaveStateFromString(string);
}

/**
 * @brief ElementData::slaveContactGroupSubtypeToString
 * Convert a SlaveType to string for XML storage in slave contact groups.
 */
QString ElementData::slaveContactGroupSubtypeToString(ElementData::SlaveType type)
{
	return slaveTypeToString(type);
}

/**
 * @brief ElementData::slaveContactGroupSubtypeFromString
 * Convert a string from XML to SlaveType for slave contact groups.
 */
ElementData::SlaveType ElementData::slaveContactGroupSubtypeFromString(const QString &string)
{
	return slaveTypeFromString(string);
}

/**
 * @brief ElementData::plcIOTypeToString
 * Convert a PlcIOType to string for XML storage.
 */
QString ElementData::plcIOTypeToString(PlcIOType type)
{
	switch (type) {
		case EntreeDigitale:
			return QStringLiteral("entree_digitale");
		case SortieDigitale:
			return QStringLiteral("sortie_digitale");
		case EntreeAnalogique:
			return QStringLiteral("entree_analogique");
		case SortieAnalogique:
			return QStringLiteral("sortie_analogique");
		case EntreeUniverselle:
			return QStringLiteral("entree_universelle");
		case SortieUniverselle:
			return QStringLiteral("sortie_universelle");
	}
	return QStringLiteral("entree_digitale");
}

/**
 * @brief ElementData::plcIOTypeFromString
 * Convert a string from XML to PlcIOType.
 */
ElementData::PlcIOType ElementData::plcIOTypeFromString(const QString &string)
{
	if (string == QLatin1String("entree_digitale")) {
		return ElementData::EntreeDigitale;
	} else if (string == QLatin1String("sortie_digitale")) {
		return ElementData::SortieDigitale;
	} else if (string == QLatin1String("entree_analogique")) {
		return ElementData::EntreeAnalogique;
	} else if (string == QLatin1String("sortie_analogique")) {
		return ElementData::SortieAnalogique;
	} else if (string == QLatin1String("entree_universelle")) {
		return ElementData::EntreeUniverselle;
	} else if (string == QLatin1String("sortie_universelle")) {
		return ElementData::SortieUniverselle;
	}

	qDebug() << "ElementData::plcIOTypeFromString : string "
			 << string
			 << " don't exist, return failsafe value 'entree_digitale'";
	return ElementData::EntreeDigitale;
}

/**
 * @brief ElementData::translatedPlcIOType
 * Return translated string for PlcIOType.
 */
QString ElementData::translatedPlcIOType(PlcIOType type)
{
	switch (type) {
		case EntreeDigitale:
			return QObject::tr("Entrée digitale");
		case SortieDigitale:
			return QObject::tr("Sortie digitale");
		case EntreeAnalogique:
			return QObject::tr("Entrée analogique");
		case SortieAnalogique:
			return QObject::tr("Sortie analogique");
		case EntreeUniverselle:
			return QObject::tr("Entrée universelle");
		case SortieUniverselle:
			return QObject::tr("Sortie universelle");
	}
	return QObject::tr("Entrée digitale");
}

/**
 * @brief ElementData::plcIOTypeList
 * Return list of all translated PLC IO type strings (for combo boxes).
 */
QStringList ElementData::plcIOTypeList()
{
	return QStringList()
		<< translatedPlcIOType(EntreeDigitale)
		<< translatedPlcIOType(SortieDigitale)
		<< translatedPlcIOType(EntreeAnalogique)
		<< translatedPlcIOType(SortieAnalogique)
		<< translatedPlcIOType(EntreeUniverselle)
		<< translatedPlcIOType(SortieUniverselle);
}
