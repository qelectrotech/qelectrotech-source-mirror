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
#include <QDomElement>

/**
	@brief The PropertiesInterface class
	This class is an interface for have common way
	to use properties in QElectroTech
*/
class PropertiesInterface
{
	public:
		PropertiesInterface();
		virtual ~PropertiesInterface();
		/**
			@brief toSettings
			Save properties to setting file.
			@param settings : is use for prefix a word
			befor the name of each paramètre
			@param QString
		*/
		virtual void toSettings (QSettings &settings,
					 const QString = QString()) const =0;
		/**
			@brief fromSettings
			load properties to setting file.
			@param settings : is use for prefix a word
			befor the name of each paramètre
			@param QString
		*/
		virtual void fromSettings (const QSettings &settings,
					   const QString = QString()) =0;
		/**
			@brief toXml
			Save properties to xml element
			@param xml_document
			@return QDomElement
		*/
		virtual QDomElement toXml (QDomDocument &xml_document) const =0;
		/**
			@brief fromXml
			load properties to xml element
			@param xml_element
			@return true / false
		*/
		virtual bool fromXml (const QDomElement &xml_element) =0;
};

#endif // PROPERTIESINTERFACE_H
