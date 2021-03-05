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
#include "propertiesinterface.h"
#include <QDebug>

/*!
 * Available property types
 */
namespace  {
	const QString integerS = "int";
	const QString doubleS = "double";
	const QString boolS = "bool";
	const QString stringS = "string";
	const QString uuidS = "uuid";
	const QString colorS = "color";

    const QString userPropertiesS = "userProperties";

    PropertiesInterface::PropertyFlags debug(PropertiesInterface::PropertyFlags flag, const QDomElement &e, const QString& attribute_name, const QString& attr, const QString& type)
    {
        if (flag == PropertiesInterface::PropertyFlags::NoValidConversion)
                    qDebug() << "\t\t\t" << "Tagname: " << e.tagName() << ". " << "No valid Conversion: " << attribute_name << ". type: " << type << ". value: " << attr;

        return flag;
    }
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

QDomElement PropertiesInterface::createXmlProperty(const QString& name, const QString value) {
    QDomDocument doc;
    QDomElement p = doc.createElement("property");
	p.setAttribute("name", name);
	p.setAttribute("type", stringS);
	p.setAttribute("value", value);
	return p;
}

QDomElement PropertiesInterface::createXmlProperty(const QString& name, const char* value) {
    QDomDocument doc;
    QDomElement p = doc.createElement("property");
	p.setAttribute("name", name);
	p.setAttribute("type", stringS);
	p.setAttribute("value", value);
	return p;
}

QDomElement PropertiesInterface::createXmlProperty(const QString& name, const int value) {
    QDomDocument doc;
    QDomElement p = doc.createElement("property");
	p.setAttribute("name", name);
	p.setAttribute("type", integerS);
	p.setAttribute("value", QString::number(value));
	return p;
}

QDomElement PropertiesInterface::createXmlProperty(const QString& name, const double value) {
    QDomDocument doc;
    QDomElement p = doc.createElement("property");
	p.setAttribute("name", name);
	p.setAttribute("type", doubleS);
	p.setAttribute("value", QString::number(value));
	return p;
}

QDomElement PropertiesInterface::createXmlProperty(const QString& name, const bool value) {
    QDomDocument doc;
    QDomElement p = doc.createElement("property");
	p.setAttribute("name", name);
	p.setAttribute("type", boolS);
	p.setAttribute("value", QString::number(value));
	return p;
}

QDomElement PropertiesInterface::createXmlProperty(const QString& name, const QUuid value) {
    QDomDocument doc;
    QDomElement p = doc.createElement("property");
	p.setAttribute("name", name);
	p.setAttribute("type", uuidS);
	p.setAttribute("value", value.toString());
	return p;
}

QDomElement PropertiesInterface::createXmlProperty(const QString& name, const QColor value) {
    QDomDocument doc;
    QDomElement p = doc.createElement("property");
	p.setAttribute("name", name);
	p.setAttribute("type", colorS);
	p.setAttribute("value", value.name());
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
 * Returns the property with the name \p attribute_name and type \p type
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
		if (!e.hasAttribute(attribute_name)) {
			qDebug() << "\t\t\t" << "Tagname: " << e.tagName() << ". " << "Property " << attribute_name << "is not available";
			return false;
		}

		*attr = e.attribute(attribute_name);

	} else {
		if (p.attribute("type") != type) {
			qDebug() << "\t\t\t" << "Tagname: " << e.tagName() << ", Property: " << attribute_name << "(" << p.attribute("type") << ") has not type: " << type;
			return false;
		}

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
 *		  \p entier is not valid and the return value is False
 */
PropertiesInterface::PropertyFlags PropertiesInterface::propertyInteger(const QDomElement &e, const QString& attribute_name, int* entier) {

	QString attr;

	if (!attribute(e, attribute_name, integerS, &attr)) {
		return PropertyFlags::NotFound;
	}

    return debug(propertyInteger(attr, entier), e, attribute_name, attr, integerS);
}

PropertiesInterface::PropertyFlags PropertiesInterface::propertyInteger(const QString& value, int* entier) {
    // verifie la validite de l'attribut
    bool ok;
    int tmp = value.toInt(&ok);
    if (!ok) {
        return PropertyFlags::NoValidConversion;
    }

    if (entier != nullptr)
        *entier = tmp;

    return PropertyFlags::Success;
}

PropertiesInterface::PropertyFlags PropertiesInterface::propertyDouble(const QDomElement &e, const QString& attribute_name, double* reel) {

	QString attr;

	if (!attribute(e, attribute_name, doubleS, &attr)) {
		return PropertyFlags::NotFound;
	}

    return debug(propertyDouble(attr, reel), e, attribute_name, attr, doubleS);
}

PropertiesInterface::PropertyFlags PropertiesInterface::propertyDouble(const QString& value, double* reel)
{
    // verifie la validite de l'attribut
    bool ok;
    double tmp = value.toDouble(&ok);
    if (!ok) {
        return PropertyFlags::NoValidConversion;
    }

    if (reel != nullptr)
        *reel = tmp;

    return PropertyFlags::Success;
}

PropertiesInterface::PropertyFlags PropertiesInterface::propertyBool(const QDomElement &e, const QString& attribute_name, bool* boolean) {

	QString attr;

	if (!attribute(e, attribute_name, boolS, &attr)) {
		return PropertyFlags::NotFound;
	}

    return debug(propertyBool(attr, boolean), e, attribute_name, attr, boolS);
}

PropertiesInterface::PropertyFlags PropertiesInterface::propertyBool(const QString& value, bool* boolean)
{
    // verifie la validite de l'attribut
    bool ok;
    bool tmp = value.toInt(&ok);
    if (!ok) {
        if (value == "true" || value == "1")
            tmp = true;
        else if (value == "false" || value == "0")
            tmp = false;
        else {
            return PropertyFlags::NoValidConversion;
        }
    }

    if (boolean != nullptr)
        *boolean = tmp;

    return PropertyFlags::Success;
}

PropertiesInterface::PropertyFlags PropertiesInterface::propertyColor(const QDomElement &e, const QString& attribute_name, QColor* color) {

	QString attr;

	if (!attribute(e, attribute_name, colorS, &attr)) {
		return PropertyFlags::NotFound;
	}

    return debug(propertyColor(attr, color), e, attribute_name, attr, colorS);
}

PropertiesInterface::PropertyFlags PropertiesInterface::propertyColor(const QString& value, QColor* color)
{
    // verifie la validite de l'attribut
    QColor tmp = QColor(value);
    if (!tmp.isValid()) {
        return PropertyFlags::NoValidConversion;
    }

    if (color != nullptr)
        *color = tmp;

    return PropertyFlags::Success;
}

PropertiesInterface::PropertyFlags PropertiesInterface::propertyUuid(const QDomElement &e, const QString& attribute_name, QUuid* uuid) {
	QString attr;

	if (!attribute(e, attribute_name, uuidS, &attr)) {
		return PropertyFlags::NotFound;
	}

    return debug(propertyUuid(attr, uuid), e, attribute_name, attr, uuidS);
}

PropertiesInterface::PropertyFlags PropertiesInterface::propertyUuid(const QString& value, QUuid* uuid)
{
    if (QUuid(value).isNull()){
        return PropertyFlags::NoValidConversion;
    }


    if (uuid != nullptr)
        *uuid = QUuid(value);

    return PropertyFlags::Success;
}

PropertiesInterface::PropertyFlags PropertiesInterface::propertyString(const QDomElement& e, const QString& attribute_name, QString* string) {

	QString attr;
	if (!attribute(e, attribute_name, stringS, &attr)) {
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

	return true;
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
            up.appendChild(createXmlProperty(i.key(), i.value().toString())); break;
        case QVariant::Type::Int:
            up.appendChild(createXmlProperty(i.key(), i.value().toInt())); break;
        case QVariant::Type::Double:
            up.appendChild(createXmlProperty(i.key(), i.value().toDouble())); break;
        case QVariant::Type::Bool:
            up.appendChild(createXmlProperty(i.key(), i.value().toBool())); break;
        case QVariant::Type::Color:
            up.appendChild(createXmlProperty(i.key(), QColor(i.value().value<QColor>()))); break;
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

            if (type == integerS)
            {
                int i;
                if (propertyInteger(value, &i) == PropertyFlags::Success)
                    properties[name] = QVariant(i);
                else
                    return false;
            }
            else if (type == doubleS)
            {
                double d;
                if (propertyDouble(value, &d) == PropertyFlags::Success)
                    properties[name] = QVariant(d);
                else
                    return false;
            }
            else if (type == boolS)
            {
                bool b;
                if (propertyBool(value, &b) == PropertyFlags::Success)
                    properties[name] = QVariant(b);
                else
                    return false;
            }
            else if (type == uuidS)
            {
                QUuid u;
                if (propertyUuid(value, &u) == PropertyFlags::Success)
                    properties[name] = QVariant(u);
                else
                    return false;
            }
            else if (type == colorS)
            {
                QColor c;
                if (propertyColor(value, &c) == PropertyFlags::Success)
                    properties[name] = QVariant(c);
                else
                    return false;
            }
            else if (type == stringS)
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
