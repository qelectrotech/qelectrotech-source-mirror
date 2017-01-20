/*
	Copyright 2006-2017 The QElectroTech Team
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
 * @brief The PropertiesInterface class
 * This class is an interface for have common way to use properties in QElectroTech
 */
class PropertiesInterface
{
	public:
	PropertiesInterface();
	// Save/load properties to setting file. QString is use for prefix a word befor the name of each paramètre
	virtual void toSettings	  (QSettings &settings, const QString = QString()) const =0;
	virtual void fromSettings (const QSettings &settings, const QString = QString()) =0;
	// Save/load properties to xml element
	virtual void toXml		  (QDomElement &xml_element) const =0;
	virtual void fromXml	  (const QDomElement &xml_element) =0;
};

#endif // PROPERTIESINTERFACE_H
