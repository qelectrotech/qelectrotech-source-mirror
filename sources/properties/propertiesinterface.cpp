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

/*!
 * Available property types
 */
namespace  {
    const QString integerS = "int";
    const QString doubleS = "double";
    const QString boolS = "bool";
    const QString stringS = "string";
    const QString uuidS = "uuid";
}

PropertiesInterface::PropertiesInterface()
{
}

QDomElement PropertiesInterface::createXmlProperty(QDomDocument &doc, const QString& name, const QString value) const {
    QDomElement p = doc.createElement("property");
    p.setAttribute("name", name);
    p.setAttribute("type", stringS);
    p.setAttribute("value", value);
    return p;
}

QDomElement PropertiesInterface::createXmlProperty(QDomDocument& doc, const QString& name, const int value) const {
    QDomElement p = doc.createElement("property");
    p.setAttribute("name", name);
    p.setAttribute("type", integerS);
    p.setAttribute("value", value);
    return p;
}

QDomElement PropertiesInterface::createXmlProperty(QDomDocument& doc, const QString& name, const double value) const {
    QDomElement p = doc.createElement("property");
    p.setAttribute("name", name);
    p.setAttribute("type", doubleS);
    p.setAttribute("value", value);
    return p;
}

QDomElement PropertiesInterface::createXmlProperty(QDomDocument& doc, const QString& name, const bool value) const {
    QDomElement p = doc.createElement("property");
    p.setAttribute("name", name);
    p.setAttribute("type", boolS);
    p.setAttribute("value", value);
    return p;
}

QDomElement PropertiesInterface::createXmlProperty(QDomDocument& doc, const QString& name, const QUuid value) const {
    QDomElement p = doc.createElement("property");
    p.setAttribute("name", name);
    p.setAttribute("type", uuidS);
    p.setAttribute("value", value.toString());
    return p;
}

QDomElement PropertiesInterface::property(const QDomElement& e, const QString& name) {
    for (int i=0; i < e.childNodes().count(); i++) {
        QDomElement child = e.childNodes().at(i).toElement();
        if (!validXmlProperty(child))
            continue; // there might also non property childs

        if (child.attribute("name") == name)
            return child;
    }
    return QDomElement();
}

/*!
 * \brief PropertiesInterface::attribute
 * Returns the property with the name \p attribute_name
 * \param e Xml element which contains the property
 * \param attribute_name
 * \param type Type of the property
 * \param attr
 * \return
 */
bool PropertiesInterface::attribute(const QDomElement& e, const QString& attribute_name, const QString& type, QString* attr) {
    QDomElement p = property(e, attribute_name);
    if (p.isNull()) {
        // check if legacy property is available,
        // where the property is inside the element as attribute
        if (!e.hasAttribute(attribute_name))
            return false;

        *attr = e.attribute(attribute_name);

    } else {
        if (p.attribute("type") != type)
            return false;

        *attr = p.attribute("value");

    }
    return true;
}

/*!
 * \brief PropertiesInterface::propertyInteger
 * Reads an interger from the XML element.
 * \param e DomElement which contains the property attribute
 * \param attribute_name Name of the attribute
 * \param entier Return value if success
 * \return True if reading an integer was successful, else False. If the attribute was not found,
 *          \p entier is not valid and the return value is False
 */
PropertiesInterface::PropertyFlags PropertiesInterface::propertyInteger(const QDomElement &e, const QString& attribute_name, int* entier, int defaultValue) {

    QString attr;

    if (!attribute(e, attribute_name, integerS, &attr)) {
        *entier = defaultValue;
        return PropertyFlags::NotFound;
    }

    // verifie la validite de l'attribut
    bool ok;
    int tmp = attr.toInt(&ok);
    if (!ok)
        return PropertyFlags::NoValidConversion;

    if (entier != nullptr)
        *entier = tmp;

    return PropertyFlags::Success;
}

PropertiesInterface::PropertyFlags PropertiesInterface::propertyDouble(const QDomElement &e, const QString& attribute_name, double* reel, double defaultValue) {

    QString attr;

    if (!attribute(e, attribute_name, doubleS, &attr)) {
        *reel = defaultValue;
        return PropertyFlags::NotFound;
    }

    // verifie la validite de l'attribut
    bool ok;
    double tmp = attr.toDouble(&ok);
    if (!ok)
        return PropertyFlags::NoValidConversion;

    if (reel != nullptr)
        *reel = tmp;

    return PropertyFlags::Success;
}

PropertiesInterface::PropertyFlags PropertiesInterface::propertyBool(const QDomElement &e, const QString& attribute_name, bool* boolean, bool defaultValue) {

    QString attr;

    if (!attribute(e, attribute_name, integerS, &attr)) {
        *boolean = defaultValue;
        return PropertyFlags::NotFound;
    }

    // verifie la validite de l'attribut
    bool ok;
    bool tmp = attr.toInt(&ok);
    if (!ok)
        return PropertyFlags::NoValidConversion;

    if (boolean != nullptr)
        *boolean = tmp;

    return PropertyFlags::Success;
}

PropertiesInterface::PropertyFlags PropertiesInterface::propertyUuid(const QDomElement &e, const QString& attribute_name, QUuid* uuid, QUuid defaultValue) {
    QString attr;

    if (!attribute(e, attribute_name, uuidS, &attr)) {
        *uuid = defaultValue;
        return PropertyFlags::NotFound;
    }

    if (uuid != nullptr)
        *uuid = QUuid(attr);

    return PropertyFlags::Success;
}

PropertiesInterface::PropertyFlags PropertiesInterface::propertyString(const QDomElement& e, const QString& attribute_name, QString* string, QString defaultValue) {

    QString attr;
    if (!attribute(e, attribute_name, stringS, &attr)) {
        *string = defaultValue;
        return PropertyFlags::NotFound;
    }

    // verifie la validite de l'attribut
    if (string != nullptr)
        *string = attr;

    return PropertyFlags::Success;
}

/*!
 * \brief PropertiesInterface::validXmlProperty
 * Check if the Xml element contains the needed fields
 * \param e Xml Property
 * \return True if name, type, value attribute are available, else false
 */
bool PropertiesInterface::validXmlProperty(const QDomElement& e) {
    if (!e.hasAttribute("name"))
        return false;

    if (!e.hasAttribute("type"))
        return false;

    if (!e.hasAttribute("value"))
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
    if (c == 'e') return(Qet::East);
    else if (c == 's') return(Qet::South);
    else if (c == 'w') return (Qet::West);
    else return(Qet::North);
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
