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
#ifndef DEVICETYPELINK_H
#define DEVICETYPELINK_H

#include <QList>
#include <QString>

class Element;

/**
	@brief The DeviceTypeLink namespace
	Linking a contact to a device which has a device type, when the
	project follows the device types (QETProject::followDeviceTypes()):
	the contact may take a part of the type not drawn yet, a slave part
	of its kind and state, and takes that part's terminals; a device with
	no such part left is full.
*/
namespace DeviceTypeLink
{
		/// Whether the project of @p master follows the device types and
		/// @p master's device has a type the project knows
	bool limits(Element *master);
		/// The parts of @p master's type, not drawn yet, @p slave may draw:
		/// slave parts of its kind and state. The part @p slave draws already,
		/// when it is a symbol of that device, counts as free for it.
	QList<int> freeParts(Element *master, Element *slave);
		/// Whether linking @p slave to @p master is refused: the project
		/// follows the device types and the type has no free part for it
	bool refuses(Element *master, Element *slave);
		/// Whether @p master's device has no slave part left to draw, by
		/// its type; false when it has no type (limits() is false)
	bool full(Element *master);
		/// Give @p slave, linked to a device with a type, the terminals of
		/// the first part of the type free for it, keeping its drawing (one
		/// undo step); @return the symbol in its place (itself when nothing
		/// changes), nullptr with @p error set when it cannot
	Element *takeNextFreePart(Element *slave, QString *error = nullptr);
}

#endif // DEVICETYPELINK_H
