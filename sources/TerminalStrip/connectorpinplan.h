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
#ifndef CONNECTORPINPLAN_H
#define CONNECTORPINPLAN_H

#include <QList>
#include <QPair>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

class DeviceType;
class Element;
class PhysicalTerminal;
class QETProject;
class TerminalStrip;
namespace DeviceTypeCheck {struct Device;}

/**
	@brief The ConnectorPinPlan namespace
	What the plan of a connector shows, as data: one row for each pin. The
	plan drawer is given these rows and knows nothing of device types.

	The rows of a connector (a terminal strip of kind plug or socket) are
	its terminals in the strip's order, then, when the connector has a
	device type, one spare row for each pin of the type drawn nowhere (the
	free pins). A pin is named by the label of its terminal after the colon:
	X1:L1 is the pin L1.

	A connector may also be drawn as a device: a head symbol with its pins
	(L1 L2 L3 E) and child symbols for the other pins (C1, C2), as a device
	type with master and slave parts says (the plug of a motor). ofDevice()
	gives it the same rows, read from the terminals of its symbols and
	their wires. Which device types are connector types: isConnectorType().
*/
namespace ConnectorPinPlan
{
	struct Row
	{
		QString pin;            ///< L1
		QString label;          ///< what the plan writes: X1:L1
		bool spare = false;     ///< a free pin of the device type, drawn nowhere
		QString xref;           ///< where the pin is drawn, when it is not a terminal of a strip; "Spare" for a spare row
			///The wires on the pin, when it is not a terminal of a strip: what the
			///other end is called, and what the wire is made of
		QList<QPair<QString, QStringList>> wires;
			///The terminal of the strip, none for a spare row
		QSharedPointer<PhysicalTerminal> physical;
	};

	struct Plan
	{
		QString name;           ///< X1
		QString kind;           ///< Plug, Socket (translated)
		QString typeName;       ///< 56P6, empty when the connector has no device type
		QString description;    ///< the text of the device type
		QList<Row> rows;

			/// What the header of the plan reads: the name, the kind and the
			/// type, one on each line
		QString header() const;
		int spareCount() const;
	};

		/// The pin a terminal label names: "X1:L1" is "L1", a label without
		/// colon is no pin name
	QString pinOfLabel(const QString &label);

		/// The plan of the connector @p strip; no rows for a terminal strip
	Plan ofStrip(const TerminalStrip *strip);

		/// Whether @p type is the type of a connector drawn as a device: it
		/// has a part of role terminal with named pins (the pins of a
		/// connector of a strip), or its informations say "connector"
		/// (a plug, or a socket, or anything for a connector of no kind
		/// known); neither adds anything to a device type file
	bool isConnectorType(const DeviceType &type);

		/// The kind the device type says (informations "connector" =
		/// plug or socket), translated; empty when it says none
	QString kindOfType(const DeviceType &type);

		/// The plan of a device of a connector type: a row for each pin of
		/// its type, read from the terminals its symbols draw (the head,
		/// then the children), and the pins no symbol draws as spare rows.
		/// The pins its symbols draw which the type lacks follow.
	Plan ofDevice(const DeviceTypeCheck::Device &device);

		/// The head symbols of the devices of @p project which have a
		/// connector type and are no connector of a strip
	QList<Element *> connectorDevices(QETProject *project);
}

#endif // CONNECTORPINPLAN_H
