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

void ElementData::kindInfoFromXml(const QDomElement &xml_element)
{
	if (m_type != ElementData::Master ||
		m_type != ElementData::Slave) {
		return;
	}

	auto xml_kind = xml_element.firstChildElement("kindInformations");
	for (auto dom_elmt : QETXML::findInDomElement(xml_kind, "kindInformation"))
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
	}
}
