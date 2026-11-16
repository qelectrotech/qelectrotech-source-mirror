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
#ifndef DEVICETYPEPLACEMENT_H
#define DEVICETYPEPLACEMENT_H

#include "devicetypecheck.h"

#include <ElementsCollection/elementslocation.h>

#include <QPointF>

class Diagram;
class DiagramEventAddElement;
class QUndoCommand;
class QWidget;

/**
	@brief The DeviceTypePlacement namespace
	Places a part of a device's type which is not drawn: an auxiliary
	contact of a contactor, as NO 53 54.

	The part's symbol is used as it is when it already has the part's pins
	and can be linked; else the project gets a copy of it, of the same name,
	with the part's pins, contact kind and state. Once placed, a contact is
	linked to the device's master; without a master to link to, the symbol
	takes the device's label and place, which makes it one of its symbols.
	PLC points are not placed from here: the PLC Manager does that.

	With askAfterHead() on (off by default), placing the head of a device
	with a type goes on with its parts not drawn yet: one by one, each
	following the mouse, or all side by side to the right of the head, to
	be moved where they belong; or not now, which leaves them free in the
	Device Navigator.
*/
namespace DeviceTypePlacement
{
	QList<int> placeableWays(const DeviceTypeCheck::Device &device, int part);
	QString wayText(const DeviceType::Part &part, int way);
	ElementsLocation symbolFor(const DeviceTypeCheck::Device &device, int part, int way,
							   const ElementsLocation &own_drawing = ElementsLocation());
	void finish(Element *placed, const DeviceTypeCheck::Device &device, int part,
				int way, QUndoCommand *undo);
	DiagramEventAddElement *begin(const DeviceTypeCheck::Device &device, int part,
								  int way, Diagram *diagram, QPointF pos = QPointF());

		/// The parts not drawn which can be placed here, in the type's order
	QList<int> freeParts(const DeviceTypeCheck::Device &device);
	QList<Element *> placeSideBySide(const DeviceTypeCheck::Device &device);
	bool placeOneByOne(const DeviceTypeCheck::Device &device, Diagram *diagram,
					   QPointF pos = QPointF());
	void offerParts(Element *head, QWidget *parent = nullptr);

	bool askAfterHead();
	void setAskAfterHead(bool ask);
}

#endif // DEVICETYPEPLACEMENT_H
