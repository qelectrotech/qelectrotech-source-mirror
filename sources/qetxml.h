/*
	Copyright 2006-2025 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef QETXML_H
#define QETXML_H

#include <QDomElement>
#include <QPen>
#include <QUuid>


class QDomDocument;
class QDir;
class QFile;
class QAbstractItemModel;
class QGraphicsItem;

/**
 *This namespace contain some function to use xml with QET.
*/
namespace QETXML
{
	QDomElement penToXml(QDomDocument &parent_document, const QPen& pen);
	QPen penFromXml (const QDomElement &element);

	QDomElement brushToXml (
			QDomDocument &parent_document,
			const QBrush& brush);

	QBrush brushFromXml (const QDomElement &element);

	QDomElement fileSystemDirToXmlCollectionDir (
			QDomDocument &document,
			const QDir &dir,
			const QString& rename = QString());

	QDomElement fileSystemElementToXmlCollectionElement (
			QDomDocument &document,
			QFile &file,
			const QString& rename = QString());

	bool writeXmlFile(
			const QDomDocument &xml_document,
			const QString &file_path,
			QString *error_message = nullptr);

	QDomElement textToDomElement (
			QDomDocument &document,
			const QString& tag_name,
			const QString& value);

	QVector <QDomElement> directChild(
			const QDomElement &element,
			const QString &tag_name);

	QVector <QDomElement> subChild(
			const QDomElement &element,
			const QString parent_tag_name,
			const QString &children_tag_name);

	QDomElement marginsToXml (
			QDomDocument &parent_document,
			const QMargins &margins);

	QMargins marginsFromXml(const QDomElement &element);

	QDomElement modelHeaderDataToXml(
			QDomDocument &parent_document,
			const QAbstractItemModel *model,
			QHash<int,
			QList<int>> horizontal_section_role,
			QHash<int,
			QList<int>> vertical_section_role);

	void modelHeaderDataFromXml(
			const QDomElement &element,
			QAbstractItemModel *model);

	QVector<QDomElement> findInDomElement(const QDomElement &dom_elmt,
										  const QString &tag_name);

	QDomElement qGraphicsItemPosToXml(QGraphicsItem *item, QDomDocument &document);
	bool qGraphicsItemPosFromXml(QGraphicsItem *item, const QDomElement &xml_elmt);

	QString boolToString(bool value);
	bool boolFromString(const QString &value,
					 bool default_value = true,
					 bool *conv_ok = nullptr);

	const QString integerS = "int";
	const QString doubleS = "double";
	const QString boolS = "bool";
	const QString stringS = "string";
	const QString uuidS = "uuid";
	const QString colorS = "color";

	enum PropertyFlags {
			Success = 0,
			NotFound = 1,
			NoValidConversion = 2,
			// = 4
	};

	/*!
	 * Use this functions to add properties to the xml document
	 */
	QDomElement createXmlProperty(const QString& name, const QString value);
	QDomElement createXmlProperty(const QString& name, const char* value);
	QDomElement createXmlProperty(const QString& name, const int value);
	QDomElement createXmlProperty(const QString& name, const double value);
	QDomElement createXmlProperty(const QString& name, const bool value);
	QDomElement createXmlProperty(const QString& name, const QUuid value);
	QDomElement createXmlProperty(const QString& name, const QColor value);

	PropertyFlags propertyInteger(const QString& value, int* entry = nullptr);
	PropertyFlags propertyInteger(const QDomElement &e, const QString& attribute_name, int *entier = nullptr);
	PropertyFlags propertyDouble(const QString& value, double* entry = nullptr);
	PropertyFlags propertyDouble(const QDomElement &e, const QString& attribute_name, double *reel = nullptr);
	PropertyFlags propertyString(const QDomElement& e, const QString& attribute_name, QString* string = nullptr);
	PropertyFlags propertyBool(const QString& value, bool* entry = nullptr);
	PropertyFlags propertyBool(const QDomElement &e, const QString& attribute_name, bool* boolean = nullptr);
	PropertyFlags propertyUuid(const QString& value, QUuid* entry = nullptr);
	PropertyFlags propertyUuid(const QDomElement &e, const QString& attribute_name, QUuid* uuid = nullptr);
	PropertyFlags propertyColor(const QString& value, QColor* entry = nullptr);
	PropertyFlags propertyColor(const QDomElement &e, const QString& attribute_name, QColor* color = nullptr);

	QDomElement property(const QDomElement& e, const QString& name);
	bool attribute(const QDomElement& e, const QString& attribute_name, const QString& type, QString* attr);

	/*!
	 * \brief PropertiesInterface::validXmlProperty
	 * Check if the Xml element contains the needed fields
	 * \param e Xml Property
	 * \return True if name, type, value attribute are available, else false
	 */
	bool validXmlProperty(const QDomElement& e);
}

#endif // QETXML_H
