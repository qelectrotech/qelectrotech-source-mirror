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
#ifndef DEVICETYPECHECK_H
#define DEVICETYPECHECK_H

#include "devicetype.h"

#include <QList>
#include <QPointer>
#include <QString>
#include <QVector>

class Element;
class DiagramContext;
class QETProject;

/**
	@brief The DeviceTypeCheck namespace
	Is each device drawn the way its type says? A device is the symbols one
	reference designation names (=plant+location-label, as in the device
	list): a master with the symbols linked to it, and the symbols of the
	same designation nothing links; the terminals X1:1, X1:2... of a strip
	are the device X1. Its type is the project's device type named by its
	device type field (the master's, else the first symbol's which has
	one; typeNameOf()).

	Each symbol is matched to one part of the type: drawn with the part's
	symbol (the same element, or a copy of it with the same name) and the
	same pins first, then with the part's symbol, then with the same pins.
	What is left are the parts not drawn and the symbols beyond the type;
	a symbol without terminals no part matches (a table of the contacts
	beside a coil, a picture) is neither.
	check() reads the whole project, building its lazy folios; changes
	nothing.
*/
namespace DeviceTypeCheck
{
		/// Device::drawn of a symbol which draws no part of the type
	enum : int {
		Beyond = -1,    ///< a symbol of the device the type has no part for
		NotAPart = -2   ///< a drawing without terminals (a table of the contacts)
	};

	struct Device
	{
		QString designation;            ///< =plant+location-label
		QString label;                  ///< its label, X1 for the terminals of a strip
		QString typeName;               ///< its device type field: the type's name
		DeviceType type;                ///< null when the project has no such type
		QList<QPointer<Element>> symbols;   ///< its symbols, the head first
		QList<int> drawn;               ///< per symbol, the part it draws; Beyond, NotAPart
		QList<int> missing;             ///< the parts not drawn, in the type's order

		bool known() const {return !type.isNull();}
		bool complete() const {return known() && missing.isEmpty() && !drawn.contains(Beyond);}
		int beyond() const {return int(drawn.count(Beyond));}
	};

		/// The device type a symbol's informations name (its "device_type")
	QString typeNameOf(const DiagramContext &info);

	QVector<Device> check(QETProject *project);
	Device checkDevice(Element *element);
	QString partText(const DeviceType::Part &part);
}

#endif // DEVICETYPECHECK_H
