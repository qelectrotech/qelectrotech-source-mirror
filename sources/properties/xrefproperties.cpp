/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "xrefproperties.h"

/**
 * @brief XRefProperties::XRefProperties
 * Default Constructor
 */
XRefProperties::XRefProperties()
{
}

/**
 * @brief XRefProperties::toSettings
 * Save to settings
 * @param settings: QSettings to use
 * @param prefix: prefix before properties name
 */
void XRefProperties::toSettings(QSettings &settings, const QString prefix) const {
	settings.setValue(prefix + "showpowerctc", m_show_power_ctc);
}

/**
 * @brief XRefProperties::fromSettings
 * load from settings
 * @param settings: QSettings to use
 * @param prefix: prefix before properties name
 */
void XRefProperties::fromSettings(const QSettings &settings, const QString prefix) {
	m_show_power_ctc = settings.value(prefix + "showpowerctc", false).toBool();
}

/**
 * @brief XRefProperties::toXml
 * Save to xml
 * @param xml_element: QDomElement to use for saving
 */
void XRefProperties::toXml(QDomElement &xml_element) const {
	xml_element.setAttribute("showpowerctc", m_show_power_ctc? "true" : "fasle");
}

/**
 * @brief XRefProperties::fromXml
 * Load from xml
 * @param xml_element: QDomElement to use for load
 */
void XRefProperties::fromXml(const QDomElement &xml_element) {
	m_show_power_ctc = xml_element.attribute("showpowerctc")  == "true";
}

bool XRefProperties::operator ==(const XRefProperties &xrp) const{
	return (m_show_power_ctc == xrp.m_show_power_ctc);
}

bool XRefProperties::operator !=(const XRefProperties &xrp) const {
	return (! (*this == xrp));
}


