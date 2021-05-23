/*
    Copyright 2006-2020 The QElectroTech Team
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
#include "propertiesinterface.h"
#include <QDebug>
#include "../qetxml.h"

/*!
 * Available property types
 */
namespace  {

    const QString userPropertiesS = "userProperties";
}

/**
    @brief PropertiesInterface::PropertiesInterface
*/
PropertiesInterface::PropertiesInterface(const QString &tagname):
    mTagName(tagname)
{
}
/**
    @brief PropertiesInterface::~PropertiesInterface
*/
PropertiesInterface::~PropertiesInterface()
{
}

void PropertiesInterface::setTagName(const QString& tagname)
{
    mTagName = tagname;
}

QString PropertiesInterface::tagName() const
{
    return mTagName;
}

QDomElement PropertiesInterface::toXml (QDomDocument &xml_document) const
{
    QDomElement element = xml_document.createElement(mTagName);
    toXmlPriv(element);
    propertiesToXml(element);

    return element;
}

bool PropertiesInterface::fromXml (const QDomElement &xml_element)
{
    if (!fromXmlPriv(xml_element))
        return false;

    if (!propertiesFromXml(xml_element))
        return false;

    return true;
}

bool PropertiesInterface::valideXml(QDomElement& element) {
	qDebug(QString("ValideXml() is not implemented. File: %1, Line: %2").arg(__FILE__).arg(__LINE__).toStdString().data());
	return false;
}

/**
	Permet de convertir une chaine de caracteres ("n", "s", "e" ou "w")
	en orientation. Si la chaine fait plusieurs caracteres, seul le
	premier est pris en compte. En cas d'incoherence, Qet::North est
	retourne.
	@param s Chaine de caractere cense representer une orientation
	@return l'orientation designee par la chaine de caractere
*/
Qet::Orientation PropertiesInterface::orientationFromString(const QString &s) {
	QChar c = s[0];
	// in some cases/ old projects? (affuteuse_250h.qet) numbers instead of characters are
	// used for the orientation
	if (c == 'e' || c == '1') return(Qet::East);
	else if (c == 's' || c == '2') return(Qet::South);
	else if (c == 'w' || c == '3') return (Qet::West);
	else return(Qet::North); // c == '0'
}

/**
	@param o une orientation
	@return une chaine de caractere representant l'orientation
*/
QString PropertiesInterface::orientationToString(Qet::Orientation o) {
	QString ret;
	switch(o) {
		case Qet::North: ret = "n"; break;
		case Qet::East : ret = "e"; break;
		case Qet::South: ret = "s"; break;
		case Qet::West : ret = "w"; break;
	}
	return(ret);
}

void PropertiesInterface::deleteUserProperties()
{
    properties.clear();
}

int PropertiesInterface::userPropertiesCount() const
{
    return properties.count();
}

void PropertiesInterface::setUserProperty(const QString& key, const QVariant& value)
{
    properties[key] = value;
}

bool PropertiesInterface::existUserProperty(const QString& key) const
{
    return properties.contains(key);
}

QVariant PropertiesInterface::userPropertyValue(const QString& key)
{
    if (!existUserProperty(key))
        return QVariant();

    return properties[key];
}

void PropertiesInterface::propertiesToXml(QDomElement& e) const
{    
    if (properties.count() == 0)
        return;

    QDomDocument doc = e.ownerDocument();
    auto up = doc.createElement(userPropertiesS);
    for (auto i = properties.begin(); i != properties.end(); ++i)
    {
        auto type = i.value().type();
        switch(type) {
        case QVariant::Type::String:
            up.appendChild(QETXML::createXmlProperty(i.key(), i.value().toString())); break;
        case QVariant::Type::Int:
            up.appendChild(QETXML::createXmlProperty(i.key(), i.value().toInt())); break;
        case QVariant::Type::Double:
            up.appendChild(QETXML::createXmlProperty(i.key(), i.value().toDouble())); break;
        case QVariant::Type::Bool:
            up.appendChild(QETXML::createXmlProperty(i.key(), i.value().toBool())); break;
        case QVariant::Type::Color:
            up.appendChild(QETXML::createXmlProperty(i.key(), QColor(i.value().value<QColor>()))); break;
        default:
            break;
        }
    }
    e.appendChild(up);
}

/*!
 * \brief PropertiesInterface::propertiesFromXml
 * Read all user properties from the DomElement& e
 * \param e
 * \return
 */
bool PropertiesInterface::propertiesFromXml(const QDomElement& e)
{
    QDomNodeList l = e.childNodes();
    for (int i=0; i < l.count(); i++)
    {
        QDomElement userProperties = l.at(i).toElement();
        if (userProperties.tagName() != userPropertiesS)
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
                    properties[name] = QVariant(i);
                else
                    return false;
            }
            else if (type == QETXML::doubleS)
            {
                double d;
                if (QETXML::propertyDouble(value, &d) == QETXML::PropertyFlags::Success)
                    properties[name] = QVariant(d);
                else
                    return false;
            }
            else if (type == QETXML::boolS)
            {
                bool b;
                if (QETXML::propertyBool(value, &b) == QETXML::PropertyFlags::Success)
                    properties[name] = QVariant(b);
                else
                    return false;
            }
            else if (type == QETXML::uuidS)
            {
                QUuid u;
                if (QETXML::propertyUuid(value, &u) == QETXML::PropertyFlags::Success)
                    properties[name] = QVariant(u);
                else
                    return false;
            }
            else if (type == QETXML::colorS)
            {
                QColor c;
                if (QETXML::propertyColor(value, &c) == QETXML::PropertyFlags::Success)
                    properties[name] = QVariant(c);
                else
                    return false;
            }
            else if (type == QETXML::stringS)
            {
                properties[name] = QVariant(value);
            }
            else
            {
                qDebug() << "Not a valid property type!";
            }
        }
    }

    return true;
}
