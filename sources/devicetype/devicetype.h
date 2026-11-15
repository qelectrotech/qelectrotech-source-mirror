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
#ifndef DEVICETYPE_H
#define DEVICETYPE_H

#include <QDomElement>
#include <QList>
#include <QMap>
#include <QPair>
#include <QString>
#include <QStringList>

/**
	@brief The DeviceType class
	What a kind of device is made of: a contactor "CONTR10" is a coil, three
	power contacts and an auxiliary NO contact. A device takes a type
	through its device type field (element information "device_type" = the
	type's name).

	Parts, in order: the first master part is the device's head. A part may
	be drawn in several ways (a relay contact as NO, NC or changeover):
	each is a Symbol, with the element it is drawn with and the names of
	its terminals. A part without a symbol is still part of the type.

	Kept in a project (<devicetypes>, QETProject::deviceTypes()) and in the
	user's library (DeviceTypeLibrary), as a <devicetype> element:
	@code
	<devicetype name="CONTR10" version="1">
	  <names><name lang="en">MOTOR CONTR, 1 N/O CONTACT</name></names>
	  <informations><information name="description">MOTOR CONTR, 1 N/O CONTACT</information></informations>
	  <parts>
	    <part role="master" kind="coil">
	      <symbol location="embed://import/CONTR-COIL.elmt" pins="A1 A2"/>
	    </part>
	    <part role="slave" kind="simple" state="NO">...</part>
	  </parts>
	</devicetype>
	@endcode
*/
class DeviceType
{
	public:
		struct Symbol
		{
			QString location;       ///< the element, e.g. embed://import/CONTR-COIL.elmt
			QStringList pins;       ///< its terminals' names, in its order
			QString state;          ///< NO, NC, SW for a contact drawn this way; empty else
			bool operator==(const Symbol &o) const
			{return location == o.location && pins == o.pins && state == o.state;}
		};
		struct Part
		{
			QString role;           ///< master, slave, simple, terminal
			QString kind;           ///< coil, protection, plc...; simple, power... for a slave
			QString state;          ///< NO, NC, SW; empty when its symbols say
			QList<Symbol> symbols;  ///< the ways it may be drawn; may be empty
			bool operator==(const Part &o) const
			{return role == o.role && kind == o.kind && state == o.state && symbols == o.symbols;}
		};

		QString name;
		int version = 1;
		QMap<QString, QString> names;                         ///< lang -> text
		QList<QPair<QString, QString>> informations;          ///< given to its devices, in order
		QList<Part> parts;

		bool isNull() const {return name.isEmpty();}
		QString text(const QString &lang = QStringLiteral("en")) const;
		bool operator==(const DeviceType &o) const;
		bool operator!=(const DeviceType &o) const {return !(*this == o);}

		QDomElement toXml(QDomDocument &document) const;
		static DeviceType fromXml(const QDomElement &element);
};

/**
	@brief The DeviceTypeLibrary namespace
	The user's device types: one <name>.qetdt file each in a folder of the
	configuration directory, where types are made and shared. A project
	keeps its own copy of the types its devices use.
*/
namespace DeviceTypeLibrary
{
	QString folder();
	QString fileName(const QString &type_name);
	QStringList names(const QString &folder = QString());
	DeviceType load(const QString &type_name, const QString &folder = QString());
	bool save(const DeviceType &type, const QString &folder = QString());
	bool remove(const QString &type_name, const QString &folder = QString());
}

#endif // DEVICETYPE_H
