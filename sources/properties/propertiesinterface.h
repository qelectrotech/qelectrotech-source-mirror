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
#ifndef PROPERTIESINTERFACE_H
#define PROPERTIESINTERFACE_H

#include <QString>
#include <QSettings>
#include <QColor>
#include <QDomElement>
#include <limits>
#include "qet.h"
#include <QUuid>

/**
 * @brief The PropertiesInterface class
 * This class is an interface for have common way to use properties in QElectroTech
 */
class PropertiesInterface
{
	public:
	PropertiesInterface();
	// Save/load properties to setting file. QString is use for prefix a word befor the name of each paramètre
    virtual void toSettings	  (QSettings &settings, const QString& = QString()) const {Q_UNUSED(settings)};
    virtual void fromSettings (const QSettings &settings, const QString& = QString()) {Q_UNUSED(settings)};
	// Save/load properties to xml element
    virtual QDomElement toXml		  (QDomDocument &xml_document) const =0;
    virtual bool fromXml	  (const QDomElement &xml_element) =0;
    static bool valideXml(QDomElement& element);

    /*!
     * Use this functions to add properties to the xml document
     */
    QDomElement createXmlProperty(QDomDocument& doc, const QString& name, const QString value) const;
    QDomElement createXmlProperty(QDomDocument &doc, const QString& name, const char* value) const;
    QDomElement createXmlProperty(QDomDocument& doc, const QString& name, const int value) const;
    QDomElement createXmlProperty(QDomDocument& doc, const QString& name, const double value) const;
    QDomElement createXmlProperty(QDomDocument& doc, const QString& name, const bool value) const;
    QDomElement createXmlProperty(QDomDocument& doc, const QString& name, const QUuid value) const;
    QDomElement createXmlProperty(QDomDocument& doc, const QString& name, const QColor value) const;

    static QDomElement property(const QDomElement& e, const QString& name);
    static bool attribute(const QDomElement& e, const QString& attribute_name, const QString& type, QString* attr);

    enum PropertyFlags {
        Success = 0,
        NotFound = 1,
        NoValidConversion = 2,
        // = 4
    };

    /*!
     * Try not using the default Value feature. It is better to initialize the class members in the class definition!
     */
    static PropertyFlags propertyInteger(const QDomElement &e, const QString& attribute_name, int *entier = nullptr);
    static PropertyFlags propertyDouble(const QDomElement &e, const QString& attribute_name, double *reel = nullptr);
    static PropertyFlags propertyString(const QDomElement& e, const QString& attribute_name, QString* string = nullptr);
    static PropertyFlags propertyBool(const QDomElement &e, const QString& attribute_name, bool* boolean = nullptr);
    static PropertyFlags propertyUuid(const QDomElement &e, const QString& attribute_name, QUuid* uuid = nullptr);
    static PropertyFlags propertyColor(const QDomElement &e, const QString& attribute_name, QColor* color = nullptr);


    static bool validXmlProperty(const QDomElement& e);

    QVariant XmlProperty(const QDomElement& element);

    /**
        Permet de convertir une chaine de caracteres ("n", "s", "e" ou "w")
        en orientation. Si la chaine fait plusieurs caracteres, seul le
        premier est pris en compte. En cas d'incoherence, Qet::North est
        retourne.
        @param s Chaine de caractere cense representer une orientation
        @return l'orientation designee par la chaine de caractere
    */
    static Qet::Orientation orientationFromString(const QString &s);

    /**
        @param o une orientation
        @return une chaine de caractere representant l'orientation
    */
    static QString orientationToString(Qet::Orientation o);
};

#endif // PROPERTIESINTERFACE_H
