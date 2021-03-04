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
#ifndef PROPERTIESINTERFACE_H
#define PROPERTIESINTERFACE_H

#include <QString>
#include <QSettings>
#include <QColor>
#include <QDomElement>
#include <limits>
#include "sources/qet.h"
#include <QUuid>

//struct Property {
//    enum class Type {
//        String,
//        Int,
//        Double,
//        Bool,
//        Uuid,
//        Color
//    };
//    Property(enum Type type, QVariant& value): type(type), value(value) {};
//   enum Type type;
//   QVariant value;
//};

//class Property_T
//{
//public:
//    enum class Type {
//        String,
//        Int,
//        Double,
//        Bool,
//        Uuid,
//        Color
//    };
//    Property_T(enum Type type): mType(type)
//    {}
//public:
//    enum Type mType;
//};

//class PropertyDouble: Property_T
//{
//public:
//    PropertyDouble(double& value): Property_T(Property_T::Type::Double), mValue(value) {};
//public:
//    double mValue;
//};

class DomElement: public QDomElement
{
public:
    DomElement(): QDomElement() {};
    DomElement(QDomElement& e): QDomElement(e) {};
};

/**
	@brief The PropertiesInterface class
	This class is an interface for have common way
	to use properties in QElectroTech
*/
class PropertiesInterface
{
	public:
    PropertiesInterface(const QString& tagname = "Properties");
		virtual ~PropertiesInterface();
		/**
			@brief toSettings
			Save properties to setting file.
			@param settings : is use for prefix a word
			befor the name of each paramètre
			@param QString
		*/
        virtual void toSettings(QSettings &,
                                const QString & = QString()) const =0;
		/**
			@brief fromSettings
			load properties to setting file.
			@param settings : is use for prefix a word
			befor the name of each paramètre
			@param QString
		*/
        virtual void fromSettings(QSettings &,
                                  const QString & = QString()) = 0;
		/**
			@brief toXml
			Save properties to xml element
			@param xml_document
			@return QDomElement
		*/
        virtual QDomElement toXml (QDomDocument &xml_document) const;
		/**
			@brief fromXml
			load properties to xml element
			@param xml_element
			@return true / false
		*/
        virtual bool fromXml (const QDomElement &xml_element);
	static bool valideXml(QDomElement& element);

	/*!
	 * Use this functions to add properties to the xml document
	 */
	static QDomElement createXmlProperty(const QString& name, const QString value);
    static QDomElement createXmlProperty(const QString& name, const char* value);
	static QDomElement createXmlProperty(const QString& name, const int value);
	static QDomElement createXmlProperty(const QString& name, const double value);
	static QDomElement createXmlProperty(const QString& name, const bool value);
	static QDomElement createXmlProperty(const QString& name, const QUuid value);
	static QDomElement createXmlProperty(const QString& name, const QColor value);

	static QDomElement property(const QDomElement& e, const QString& name);
	static bool attribute(const QDomElement& e, const QString& attribute_name, const QString& type, QString* attr);

	enum PropertyFlags {
		Success = 0,
		NotFound = 1,
		NoValidConversion = 2,
		// = 4
	};

    static PropertyFlags propertyInteger(const QString& value, int* entry = nullptr);
	static PropertyFlags propertyInteger(const QDomElement &e, const QString& attribute_name, int *entier = nullptr);
    static PropertyFlags propertyDouble(const QString& value, double* entry = nullptr);
	static PropertyFlags propertyDouble(const QDomElement &e, const QString& attribute_name, double *reel = nullptr);
	static PropertyFlags propertyString(const QDomElement& e, const QString& attribute_name, QString* string = nullptr);
    static PropertyFlags propertyBool(const QString& value, bool* entry = nullptr);
	static PropertyFlags propertyBool(const QDomElement &e, const QString& attribute_name, bool* boolean = nullptr);
    static PropertyFlags propertyUuid(const QString& value, QUuid* entry = nullptr);
	static PropertyFlags propertyUuid(const QDomElement &e, const QString& attribute_name, QUuid* uuid = nullptr);
    static PropertyFlags propertyColor(const QString& value, QColor* entry = nullptr);
	static PropertyFlags propertyColor(const QDomElement &e, const QString& attribute_name, QColor* color = nullptr);


	static bool validXmlProperty(const QDomElement& e);

	QVariant XmlProperty(const QDomElement& element);
    void setTagName(const QString& tagname);

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

private:
    virtual void toXmlPriv (QDomElement &e) const =0;
    virtual bool fromXmlPriv (const QDomElement &e) =0;
    void propertiesToXml(QDomElement& e) const;
    bool propertiesFromXml (const QDomElement &e);

    QHash<QString, QVariant> properties;
    QString mTagName{""};
};

#endif // PROPERTIESINTERFACE_H
