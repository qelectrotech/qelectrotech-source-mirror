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
#ifndef DEVICETYPECHANGE_H
#define DEVICETYPECHANGE_H

#include "devicetypecheck.h"

#include <QList>
#include <QString>

/**
	@brief The DeviceTypeChange namespace
	A device takes another device type (a relay needing one more contact
	takes a bigger type): each of its symbols is given a part of the new
	type, and takes that part's terminals, keeping its drawing.
*/
namespace DeviceTypeChange
{
	enum : int {
		Beyond = -1     ///< a symbol the new type has no part for
	};

		/// The part of @p type each symbol of @p device draws, in the order
		/// of its symbols: the head the type's first master part, the others
		/// a free part of the same role, kind and state as the part they
		/// draw now (or of their own kind and state); Beyond when none is left
	QList<int> automatic(const DeviceTypeCheck::Device &device, const DeviceType &type);
		/// How a symbol of @p device reads in a list: its drawing, its label
		/// and where it is
	QString symbolText(const DeviceTypeCheck::Device &device, int symbol);
		/// Give @p device the type @p type, its symbols the parts @p parts
		/// (Beyond: left as it is), as one undo step; the project takes a
		/// copy of a type it has not got
	bool apply(const DeviceTypeCheck::Device &device, const DeviceType &type,
			   const QList<int> &parts, QString *error = nullptr);
		/// The device type the materials list gives the part number of
		/// @p head (its device_type column); empty if none
	QString partNumberType(Element *head);
}

#endif // DEVICETYPECHANGE_H
