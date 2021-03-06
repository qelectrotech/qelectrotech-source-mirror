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
#include "xrefproperties.h"

#include "../qetapp.h"

#include <QHash>
#include <QMetaEnum>

#include "../qetxml.h"

/**
	@brief XRefProperties::XRefProperties
	Default Constructor
*/
XRefProperties::XRefProperties()
{
    setTagName("xref");
}

/**
	@brief XRefProperties::toSettings
	Save to settings
	@param settings: QSettings to use
	@param prefix: prefix before properties name
*/
void XRefProperties::toSettings(QSettings &settings,
                const QString &prefix) const
{
	settings.setValue(prefix + "showpowerctc", m_show_power_ctc);
	QString display = m_display == Cross? "cross" : "contacts";
	settings.setValue(prefix + "displayhas", display);
	QString snap = m_snap_to == Bottom? "bottom" : "label";
	settings.setValue(prefix + "snapto", snap);
	int offset = m_offset;
	settings.setValue(prefix + "offset", offset);
	QString master_label = m_master_label;
	settings.setValue(prefix + "master_label", master_label);
	QString slave_label = m_slave_label;
	settings.setValue(prefix + "slave_label", slave_label);


	QMetaEnum var = QMetaEnum::fromType<Qt::Alignment>();
	settings.setValue(prefix + "xrefpos",  var.valueToKey(m_xref_pos));

	foreach (QString key, m_prefix.keys()) {
		settings.setValue(prefix + key + "prefix", m_prefix.value(key));
	}
}

/**
	@brief XRefProperties::fromSettings
	load from settings
	@param settings: QSettings to use
	@param prefix: prefix before properties name
*/
void XRefProperties::fromSettings(QSettings &settings,
                  const QString &prefix)
{
	m_show_power_ctc = settings.value(prefix + "showpowerctc", true).toBool();
	QString display = settings.value(prefix + "displayhas", "cross").toString();
	display == "cross"? m_display = Cross : m_display = Contacts;
	QString snap = settings.value(prefix + "snapto", "label").toString();
	snap == "bottom"? m_snap_to = Bottom : m_snap_to = Label;
	m_offset = settings.value(prefix + "offset", "0").toInt();
	m_master_label = settings.value(prefix + "master_label", "%f-%l%c").toString();
	m_slave_label = settings.value(prefix + "slave_label", "(%f-%l%c)").toString();

	QMetaEnum var = QMetaEnum::fromType<Qt::Alignment>();
	m_xref_pos = Qt::AlignmentFlag(var.keyToValue((settings.value(prefix + "xrefpos").toString()).toStdString().data()));

	for (QString key : m_prefix_keys) {
		m_prefix.insert(key, settings.value(prefix + key + "prefix").toString());
	}
}

/**
	@brief XRefProperties::toXml
	Save to xml
	@param xml_document : QDomElement to use for saving
	@return QDomElement
*/
void XRefProperties::toXmlPriv(QDomElement& e) const
{

    e.appendChild(QETXML::createXmlProperty("type", m_key));
    e.appendChild(QETXML::createXmlProperty("showpowerctc", m_show_power_ctc));
    e.appendChild(QETXML::createXmlProperty("displayhas", m_display == Cross? "cross" : "contacts"));
    e.appendChild(QETXML::createXmlProperty("snapto", m_snap_to == Bottom? "bottom" : "label"));


	QMetaEnum var = QMetaEnum::fromType<Qt::Alignment>();
    e.appendChild(QETXML::createXmlProperty("xrefpos", var.valueToKey(m_xref_pos)));
    e.appendChild(QETXML::createXmlProperty("offset", m_offset));
    e.appendChild(QETXML::createXmlProperty("master_label", m_master_label));
    e.appendChild(QETXML::createXmlProperty("slave_label", m_slave_label));

	foreach (QString key, m_prefix.keys()) {
        e.appendChild(QETXML::createXmlProperty(key + "prefix", m_prefix.value(key)));
	}
}

/** RETURNS True
    @brief XRefProperties::fromXmlPriv
	Load from xml
	@param xml_element: QDomElement to use for load
*/
bool XRefProperties::fromXmlPriv(const QDomElement &xml_element) {

	if (QETXML::propertyBool(xml_element, "showpowerctc", &m_show_power_ctc))
		return false;

	QString display;
    if (QETXML::propertyString(xml_element, "displayhas", &display) != QETXML::PropertyFlags::NotFound) {
		display == "cross"? m_display = Cross : m_display = Contacts;
	}


	QString snap;
    if (QETXML::propertyString(xml_element, "snapto", &snap)  != QETXML::PropertyFlags::NotFound) {
		snap == "bottom"? m_snap_to = Bottom : m_snap_to = Label;
	}

	QString xrefpos;
    if (QETXML::propertyString(xml_element, "xrefpos", &xrefpos) != QETXML::PropertyFlags::NotFound) {
		QMetaEnum var = QMetaEnum::fromType<Qt::Alignment>();
		m_xref_pos = Qt::AlignmentFlag(var.keyToValue(xrefpos.toStdString().data()));
	}
	// TODO: why it compiles without this true??
	QETXML::propertyInteger(xml_element, "offset", &m_offset);
    QETXML::propertyString(xml_element, "master_label", &m_master_label);
    QETXML::propertyString(xml_element, "slave_label", &m_slave_label);
	QString value;
	foreach (QString key, m_prefix_keys) {
        if (!QETXML::propertyString(xml_element, key + "prefix", &value))
			m_prefix.insert(key, value);
	}
	return true;
}

/**
	@brief XRefProperties::defaultProperties
	@return the default properties stored in the setting file
	For the xref, there is 2 propreties.
	For coil, stored with the string "coil" in the returned QHash.
	For protection, stored with the string "protection" in the returned QHash.
*/
QHash<QString, XRefProperties> XRefProperties::defaultProperties()
{
	QHash <QString, XRefProperties> hash;
	QStringList keys;
	keys << "coil" << "protection" << "commutator";

	QSettings settings;

	foreach (QString key, keys)
	{
		XRefProperties properties;
		QString str("diagrameditor/defaultxref");
		properties.fromSettings(settings, str += key);
		hash.insert(key, properties);
	}

	return hash;
}

bool XRefProperties::operator ==(const XRefProperties &xrp) const{
	return (m_show_power_ctc == xrp.m_show_power_ctc &&
			m_display == xrp.m_display &&
			m_snap_to == xrp.m_snap_to &&
			m_prefix == xrp.m_prefix &&
			m_master_label == xrp.m_master_label &&
			 m_offset == xrp.m_offset &&
			m_xref_pos == xrp.m_xref_pos );
}

bool XRefProperties::operator !=(const XRefProperties &xrp) const
{
	return (! (*this == xrp));
}


