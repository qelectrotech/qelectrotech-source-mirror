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
{}

/**
 * @brief XRefProperties::toSettings
 * Save to settings
 * @param settings: QSettings to use
 * @param prefix: prefix before properties name
 */
void XRefProperties::toSettings(QSettings &settings, const QString prefix) const {
	settings.setValue(prefix + "showpowerctc", m_show_power_ctc);
	QString display = m_display == Cross? "cross" : "contacts";
	settings.setValue(prefix + "displayhas", display);
	QString snap = m_snap_to == Bottom? "bottom" : "label";
	settings.setValue(prefix + "snapto", snap);
	settings.setValue(prefix + "powerprefix",  m_prefix.value("power"));
	settings.setValue(prefix + "delayprefix", m_prefix.value("delay"));
}

/**
 * @brief XRefProperties::fromSettings
 * load from settings
 * @param settings: QSettings to use
 * @param prefix: prefix before properties name
 */
void XRefProperties::fromSettings(const QSettings &settings, const QString prefix) {
	m_show_power_ctc = settings.value(prefix + "showpowerctc", false).toBool();
	QString display = settings.value(prefix + "displayhas", "cross").toString();
	display == "cross"? m_display = Cross : m_display = Contacts;
	QString snap = settings.value(prefix + "snapto", "label").toString();
	snap == "bottom"? m_snap_to = Bottom : m_snap_to = Label;
	m_prefix.insert("power", settings.value(prefix + "powerprefix").toString());
	m_prefix.insert("delay", settings.value(prefix + "delayprefix").toString());
}

/**
 * @brief XRefProperties::toXml
 * Save to xml
 * @param xml_element: QDomElement to use for saving
 */
void XRefProperties::toXml(QDomElement &xml_element) const {
	xml_element.setAttribute("showpowerctc", m_show_power_ctc? "true" : "fasle");
	QString display = m_display == Cross? "cross" : "contacts";
	xml_element.setAttribute("displayhas", display);
	QString snap = m_snap_to == Bottom? "bottom" : "label";
	xml_element.setAttribute("snapto", snap);
	xml_element.setAttribute("powerprefix", m_prefix.value("power"));
	xml_element.setAttribute("delayprefix", m_prefix.value("delay"));
}

/**
 * @brief XRefProperties::fromXml
 * Load from xml
 * @param xml_element: QDomElement to use for load
 */
void XRefProperties::fromXml(const QDomElement &xml_element) {
	m_show_power_ctc = xml_element.attribute("showpowerctc")  == "true";
	QString display = xml_element.attribute("displayhas", "cross");
	display == "cross"? m_display = Cross : m_display = Contacts;
	QString snap = xml_element.attribute("snapto", "label");
	snap == "bottom"? m_snap_to = Bottom : m_snap_to = Label;
	m_prefix.insert("power", xml_element.attribute("powerprefix"));
	m_prefix.insert("delay", xml_element.attribute("delayprefix"));
}

bool XRefProperties::operator ==(const XRefProperties &xrp) const{
	return (m_show_power_ctc == xrp.m_show_power_ctc &&
			m_display == xrp.m_display &&
			m_snap_to == xrp.m_snap_to &&
			m_prefix == xrp.m_prefix);
}

bool XRefProperties::operator !=(const XRefProperties &xrp) const {
	return (! (*this == xrp));
}


