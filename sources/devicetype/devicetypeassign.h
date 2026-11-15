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
#ifndef DEVICETYPEASSIGN_H
#define DEVICETYPEASSIGN_H

#include "devicetypecheck.h"

#include <QList>
#include <QString>

class Element;

/**
	@brief The DeviceTypeAssign namespace
	A symbol already placed takes a part of its device type: the part's
	terminal names, either on its own drawing (the project gets a copy of
	that drawing with those names) or on the drawing the type gives the
	part. The symbol is replaced, as one undoable step, by one which keeps
	its place, informations, texts, links and wires.
*/
namespace DeviceTypeAssign
{
	struct Choice
	{
		int part = -1;
		int way = -1;
		bool own_drawing = true;
		bool possible = true;
		QString text;       ///< "contact NO 13 14: keep this drawing"
		QString reason;     ///< why it is not possible
	};

	DeviceTypeCheck::Device deviceOf(Element *symbol, const DeviceType &type);
	QList<Choice> choices(Element *symbol, const DeviceTypeCheck::Device &device);
	Element *apply(Element *symbol, const DeviceTypeCheck::Device &device,
				   const Choice &choice, QString *error = nullptr);
}

#endif // DEVICETYPEASSIGN_H
