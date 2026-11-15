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
#include "devicetypelink.h"

#include "../diagram.h"
#include "../properties/elementdata.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetproject.h"
#include "devicetypeassign.h"
#include "devicetypecheck.h"

#include <QObject>

namespace DeviceTypeLink
{
	namespace
	{
		QETProject *projectOf(Element *element)
		{
			return element && element->diagram() ? element->diagram()->project() : nullptr;
		}

			/// The states a part may be drawn in: its own, else its ways'
		QStringList statesOf(const DeviceType::Part &part)
		{
			if (!part.state.isEmpty()) return {part.state};
			QStringList states;
			for (const DeviceType::Symbol &symbol : part.symbols)
				if (!symbol.state.isEmpty() && !states.contains(symbol.state))
					states << symbol.state;
			return states;
		}

			/// Whether @p slave may draw @p part: a slave part of its kind and
			/// state, a kind or state left open matching any
		bool fits(const DeviceType::Part &part, Element *slave)
		{
			if (part.role != QLatin1String("slave")) return false;
			const ElementData data = slave->elementData();
			const QString kind = ElementData::slaveTypeToString(data.m_slave_type);
			if (!part.kind.isEmpty() && !kind.isEmpty() && part.kind != kind) return false;
			const QStringList states = statesOf(part);
			const QString state = ElementData::slaveStateToString(data.m_slave_state);
			return states.isEmpty() || state.isEmpty() || states.contains(state);
		}
	}

	bool limits(Element *master)
	{
		QETProject *project = projectOf(master);
		if (!project || !project->followDeviceTypes()) return false;
		return DeviceTypeCheck::checkDevice(master).known();
	}

	QList<int> freeParts(Element *master, Element *slave)
	{
		QList<int> parts;
		if (!master || !slave) return parts;
		const DeviceTypeCheck::Device device = DeviceTypeCheck::checkDevice(master);
		if (!device.known()) return parts;
		QList<int> open = device.missing;
			//What the slave draws already, as a symbol of this device
		const int at = int(device.symbols.indexOf(slave));
		if (at >= 0 && device.drawn.value(at, DeviceTypeCheck::Beyond) >= 0)
			open << device.drawn.at(at);
		std::sort(open.begin(), open.end());
		for (int part : std::as_const(open))
			if (fits(device.type.parts.at(part), slave)) parts << part;
		return parts;
	}

	bool refuses(Element *master, Element *slave)
	{
		return limits(master) && freeParts(master, slave).isEmpty();
	}

	bool full(Element *master)
	{
		const DeviceTypeCheck::Device device = DeviceTypeCheck::checkDevice(master);
		if (!device.known()) return false;
		for (int part : device.missing)
			if (device.type.parts.at(part).role == QLatin1String("slave")) return false;
		return true;
	}

	Element *takeNextFreePart(Element *slave, QString *error)
	{
		Element *master = nullptr;
		if (slave)
			for (Element *linked : slave->linkedElements())
				if (linked->linkType() & Element::Master) {
					master = linked;
					break;
				}
		if (!master || !limits(master)) return slave;
		const QList<int> free = freeParts(master, slave);
		if (free.isEmpty()) {
			if (error) *error = QObject::tr("This device has no free contact of this kind.");
			return nullptr;
		}
		const DeviceTypeCheck::Device device = DeviceTypeCheck::checkDevice(master);
		const int at = int(device.symbols.indexOf(slave));
		const int drawn = at >= 0 ? device.drawn.value(at, DeviceTypeCheck::Beyond)
								  : int(DeviceTypeCheck::Beyond);
			//It draws a free part already, with that part's terminals:
			//nothing to do
		if (free.contains(drawn)) {
			QStringList names;
			for (Terminal *terminal : slave->terminals()) names << terminal->baseName().trimmed();
			for (const DeviceType::Symbol &way : device.type.parts.at(drawn).symbols)
				if (way.pins == names) return slave;
		}
			//The part it draws first, else the first free one
		QList<int> order = free;
		if (free.contains(drawn)) {
			order.removeAll(drawn);
			order.prepend(drawn);
		}
		const auto choices = DeviceTypeAssign::choices(slave, device);
		for (int part : std::as_const(order))
			for (const DeviceTypeAssign::Choice &choice : choices)
				if (choice.part == part && choice.possible && choice.own_drawing)
					return DeviceTypeAssign::apply(slave, device, choice, error);
		if (error) *error = QObject::tr("The free part's terminals do not fit this symbol.");
		return nullptr;
	}
}
