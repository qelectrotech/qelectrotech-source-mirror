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
#include "connectorpinplan.h"

#include "../devicetype/devicetypecheck.h"
#include "../qetapp.h"
#include "../qetproject.h"
#include "physicalterminal.h"
#include "realterminal.h"
#include "terminalstrip.h"

#include <QObject>
#include <QSet>

namespace ConnectorPinPlan
{
	QString Plan::header() const
	{
		QStringList lines;
		lines << name;
		if (!kind.isEmpty() && !typeName.isEmpty())
			//: %1 is the kind of a connector, Plug or Socket, %2 its device type
			lines << QObject::tr("%1 %2", "kind and device type of a connector").arg(kind, typeName);
		else if (!kind.isEmpty() || !typeName.isEmpty())
			lines << kind + typeName;
		if (!description.isEmpty()) lines << description;
		return lines.join(QLatin1Char('\n'));
	}

	int Plan::spareCount() const
	{
		int count = 0;
		for (const Row &row : rows)
			if (row.spare) ++count;
		return count;
	}

	QString pinOfLabel(const QString &label)
	{
		const int colon = label.indexOf(QLatin1Char(':'));
		return colon < 0 ? QString() : label.mid(colon + 1).trimmed();
	}

	Plan ofStrip(const TerminalStrip *strip)
	{
		Plan plan;
		if (!strip || !strip->isConnector()) return plan;
		plan.name = strip->name();
		plan.kind = strip->kind() == TerminalStripData::Kind::Plug
				? QObject::tr("Plug") : QObject::tr("Socket");

		QSet<QString> drawn;
		for (const auto &physical : strip->physicalTerminal()) {
			Row row;
			row.physical = physical;
			const auto reals = physical->realTerminals();
			row.label = reals.isEmpty() ? QString() : reals.first()->label();
			row.pin = pinOfLabel(row.label);
			if (!row.pin.isEmpty()) drawn.insert(row.pin);
			plan.rows << row;
		}

		const DeviceType type = strip->project() && !strip->deviceType().isEmpty()
				? strip->project()->deviceType(strip->deviceType()) : DeviceType();
		if (!type.isNull()) {
			plan.typeName = type.name;
			plan.description = type.text(QETApp::langFromSetting());
				//The free pins, in the order of the type
			for (int part : DeviceTypeCheck::pinParts(type)) {
				for (const QString &pin : DeviceTypeCheck::pinsOf(type.parts.at(part))) {
					if (drawn.contains(pin)) continue;
					drawn.insert(pin);
					Row row;
					row.pin = pin;
					row.spare = true;
					//: %1 is a pin of a connector, such as "X1:C2": a pin of the device type drawn nowhere
					row.label = QObject::tr("%1 (spare)").arg(plan.name + QLatin1Char(':') + pin);
					plan.rows << row;
				}
			}
		}
		return plan;
	}
}
