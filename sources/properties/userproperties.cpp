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
#include "userproperties.h"
#include "../qetxml.h"

#include <QUuid>
#include <QDebug>

/**
 * @brief UserProperties::UserProperties
 * @param tag_name
 */
UserProperties::UserProperties(const QString tag_name) :
	m_tag_name(tag_name)
{}

/**
 * @brief UserProperties::toXml
 * @param xml_document
 * @return A QDomElement who contain one child for each user properties
 * the tag name of the returned domElement is "userProperties" by default
 * If you want to change the tag name, see setTagName and tagName function.
 */
QDomElement UserProperties::toXml(QDomDocument &xml_document) const
{
	QDomElement up = xml_document.createElement(m_tag_name);
	if (m_properties.count() == 0)
		return up;

	for (auto i = m_properties.begin(); i != m_properties.end(); ++i)
	{
		auto type = i.value().typeId();
		switch(type) {
		case QMetaType::QString:
			up.appendChild(QETXML::createXmlProperty(i.key(), i.value().toString())); break;
		case QMetaType::Int:
			up.appendChild(QETXML::createXmlProperty(i.key(), i.value().toInt())); break;
		case QMetaType::Double:
			up.appendChild(QETXML::createXmlProperty(i.key(), i.value().toDouble())); break;
		case QMetaType::Bool:
			up.appendChild(QETXML::createXmlProperty(i.key(), i.value().toBool())); break;
		case QMetaType::QColor:
			up.appendChild(QETXML::createXmlProperty(i.key(), QColor(i.value().value<QColor>()))); break;
		default:
			break;
		}
	}

	return up;
}

/**
 * @brief UserProperties::fromXml
 * @param xml_element
 * @return load user properties from xml
 * Take care before using this function that the tagName of this class
 * is the same as the tag of xml_element given in parameter.
 */
bool UserProperties::fromXml(const QDomElement &xml_element)
{
	QDomNodeList l = xml_element.childNodes();
	for (int i=0; i < l.count(); i++)
	{
		QDomElement userProperties = l.at(i).toElement();
		if (userProperties.tagName() != m_tag_name)
			continue;

		QDomElement userProperty;
		for (int up_index = 0; up_index < userProperties.childNodes().length(); up_index++)
		{
			userProperty = userProperties.childNodes().at(up_index).toElement();

			QString name = userProperty.attribute("name");
			QString type = userProperty.attribute("type");
			QString value = userProperty.attribute("value");

			if (type == QETXML::integerS)
			{
				int i;
				if (QETXML::propertyInteger(value, &i) == QETXML::PropertyFlags::Success)
					m_properties[name] = QVariant(i);
				else
					return false;
			}
			else if (type == QETXML::doubleS)
			{
				double d;
				if (QETXML::propertyDouble(value, &d) == QETXML::PropertyFlags::Success)
					m_properties[name] = QVariant(d);
				else
					return false;
			}
			else if (type == QETXML::boolS)
			{
				bool b;
				if (QETXML::propertyBool(value, &b) == QETXML::PropertyFlags::Success)
					m_properties[name] = QVariant(b);
				else
					return false;
			}
			else if (type == QETXML::uuidS)
			{
				QUuid u;
				if (QETXML::propertyUuid(value, &u) == QETXML::PropertyFlags::Success)
					m_properties[name] = QVariant(u);
				else
					return false;
			}
			else if (type == QETXML::colorS)
			{
				QColor c;
				if (QETXML::propertyColor(value, &c) == QETXML::PropertyFlags::Success)
					m_properties[name] = QVariant(c);
				else
					return false;
			}
			else if (type == QETXML::stringS)
			{
				m_properties[name] = QVariant(value);
			}
			else
			{
				qDebug() << "Not a valid property type!";
			}
		}
	}

	return true;
}

/**
 * @brief UserProperties::deleteUserProperties
 * Delete all userproperties
 */
void UserProperties::deleteUserProperties() {
	m_properties.clear();
}

/**
 * @brief UserProperties::userPropertiesCount
 * @return Returns the number of user properties
 */
int UserProperties::userPropertiesCount() const {
	return m_properties.count();
}

/**
 * @brief UserProperties::setUserProperty
 * Adds a new property if \p key does not exist in the \p properties member,
 * otherwise overwrite the value
 * @param key
 * @param value
 */
void UserProperties::setUserProperty(const QString &key, const QVariant &value) {
	m_properties[key] = value;
}

/**
 * @brief UserProperties::existUserProperty
 * Checks if a user property with key \p key is available or not
 * @param key
 * @return true if property exist
 */
bool UserProperties::existUserProperty(const QString &key) const {
	return m_properties.contains(key);
}

/**
 * @brief UserProperties::userPropertyValue
 * Returns the value of a user property with key \p key
 * If \p key is not found, an invalid QVariant is returned.
 * Use QVariant::type() to get the type of the value
 * @param key
 * @return
 */
QVariant UserProperties::userPropertyValue(const QString &key)
{
	if (!existUserProperty(key))
		return QVariant();

	return m_properties[key];
}

/**
 * @brief UserProperties::setTagName
 * Set the tag name use to store in xml
 * @param tag_name
 */
void UserProperties::setTagName(const QString &tag_name) {
	m_tag_name = tag_name;
}

QString UserProperties::tagName() const {
	return m_tag_name;
}
