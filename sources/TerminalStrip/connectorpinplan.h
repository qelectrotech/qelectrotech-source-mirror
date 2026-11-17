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

class PhysicalTerminal;
class TerminalStrip;

/**
	@brief The ConnectorPinPlan namespace
	What the plan of a connector shows, as data: one row for each pin. The
	plan drawer is given these rows and knows nothing of device types.

	The rows of a connector (a terminal strip of kind plug or socket) are
	its terminals in the strip's order, then, when the connector has a
	device type, one spare row for each pin of the type drawn nowhere (the
	free pins). A pin is named by the label of its terminal after the colon:
	X1:L1 is the pin L1.
*/
namespace ConnectorPinPlan
{
	struct Row
	{
		QString pin;            ///< L1
		QString label;          ///< what the plan writes: X1:L1, or "X1:C2 (spare)"
		bool spare = false;     ///< a free pin of the device type, drawn nowhere
		QString xref;           ///< where the pin is drawn, when it is not a terminal of a strip
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
}

#endif // CONNECTORPINPLAN_H
