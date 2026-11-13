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
#include "terminalstripdata.h"
#include <qetxml.h>
#include <QDebug>

TerminalStripData::TerminalStripData()
{

}

TerminalStripData::TerminalStripData(const TerminalStripData &other) {
	*this = other;
}

QDomElement TerminalStripData::toXml(QDomDocument &xml_document) const
{
	auto root_elmt = xml_document.createElement(this->xmlTagName());

	root_elmt.setAttribute(QStringLiteral("uuid"), m_uuid.toString());

	auto info_elmt = xml_document.createElement(QStringLiteral("informations"));
	root_elmt.appendChild(info_elmt);

	if (!m_installation.isEmpty()) {
		info_elmt.appendChild(infoToXml(xml_document, QStringLiteral("installation"), m_installation));
	}
	if (!m_location.isEmpty()) {
		info_elmt.appendChild(infoToXml(xml_document, QStringLiteral("location"), m_location));
	}
	if (!m_name.isEmpty()) {
		info_elmt.appendChild(infoToXml(xml_document, QStringLiteral("name"), m_name));
	}
	if (!m_comment.isEmpty()) {
		info_elmt.appendChild(infoToXml(xml_document, QStringLiteral("comment"), m_comment));
	}
	if (!m_description.isEmpty()) {
		info_elmt.appendChild(infoToXml(xml_document, QStringLiteral("description"), m_description));
	}
	if (!m_manufacturer.isEmpty()) {
		info_elmt.appendChild(infoToXml(xml_document, QStringLiteral("manufacturer"), m_manufacturer));
	}

	auto upper_cables = m_upper_cables.values();
	upper_cables.sort();
	for (const auto &key : upper_cables) {
		auto cable = xml_document.createElement(QStringLiteral("cable_above"));
		cable.setAttribute(QStringLiteral("key"), key);
		root_elmt.appendChild(cable);
	}

	return root_elmt;
}

bool TerminalStripData::fromXml(const QDomElement &xml_element)
{
	if (xml_element.tagName() != this->xmlTagName())
	{
		qDebug() << "TerminalStripData::fromXml : failed to load from xml " \
					"due to wrong tag name. Expected " << this->xmlTagName() << " used " << xml_element.tagName();
		return false;
	}

	m_uuid = QUuid::fromString(xml_element.attribute(QStringLiteral("uuid")));

	m_upper_cables.clear();
	for (auto cable = xml_element.firstChildElement(QStringLiteral("cable_above"));
		 !cable.isNull();
		 cable = cable.nextSiblingElement(QStringLiteral("cable_above"))) {
		setCableAbove(cable.attribute(QStringLiteral("key")), true);
	}

	for (auto &xml_info :
		 QETXML::findInDomElement(xml_element.firstChildElement(QStringLiteral("informations")),
								  QStringLiteral("information")))
	{
		auto name = xml_info.attribute(QStringLiteral("name"));
		auto value = xml_info.text();

		if (name == QLatin1String("installation"))     { m_installation = value;}
		else if (name == QLatin1String("location"))    {m_location = value;}
		else if (name == QLatin1String("name"))        {m_name = value;}
		else if (name == QLatin1String("comment"))     {m_comment = value;}
		else if (name == QLatin1String("description")) {m_description = value;}
		else if (name == QLatin1String("manufacturer")) {m_manufacturer = value;}
	}

	return true;
}

TerminalStripData &TerminalStripData::operator=(const TerminalStripData &other)
{
	m_installation = other.m_installation;
	m_location = other.m_location;
	m_name = other.m_name;
	m_comment = other.m_comment;
	m_description = other.m_description;
	m_manufacturer = other.m_manufacturer;
	m_uuid = other.m_uuid;
	m_upper_cables = other.m_upper_cables;

	return *this;
}

bool TerminalStripData::cableAbove(const QString &key) const
{
	return m_upper_cables.contains(key);
}

void TerminalStripData::setCableAbove(const QString &key, bool above)
{
	if (key.isEmpty()) return;
	if (above) {
		m_upper_cables.insert(key);
	} else {
		m_upper_cables.remove(key);
	}
}

QDomElement TerminalStripData::infoToXml(QDomDocument &xml_doc, const QString &name, const QString &value)
{
	auto xml_elmt = xml_doc.createElement(QStringLiteral("information"));
	xml_elmt.setAttribute(QStringLiteral("name"), name);
	xml_elmt.appendChild(xml_doc.createTextNode(value));

	return xml_elmt;
}
