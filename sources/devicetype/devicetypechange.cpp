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
#include "devicetypechange.h"

#include "../diagram.h"
#include "../diagramposition.h"
#include "../materiallist/materiallist.h"
#include "../properties/elementdata.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetinformation.h"
#include "../qetproject.h"
#include "../undocommand/changeelementinformationcommand.h"
#include "../undocommand/setdevicetypecommand.h"
#include "devicetypeassign.h"

#include <QObject>
#include <QUndoStack>

namespace DeviceTypeChange
{
	namespace
	{
		QStringList statesOf(const DeviceType::Part &part)
		{
			if (!part.state.isEmpty()) return {part.state};
			QStringList states;
			for (const DeviceType::Symbol &way : part.symbols)
				if (!way.state.isEmpty()) states << way.state;
			return states;
		}

			/// Whether @p part is like @p old: the same role, kind and states
		bool alike(const DeviceType::Part &part, const DeviceType::Part &old)
		{
			return part.role == old.role && part.kind == old.kind && statesOf(part) == statesOf(old);
		}

			/// Whether @p symbol, drawing no part now, may draw @p part: a
			/// master the master part, a contact a slave part of its kind
			/// and state
		bool suits(const DeviceType::Part &part, Element *symbol)
		{
			if (symbol->linkType() & Element::Master) return part.role == QLatin1String("master");
			if (!(symbol->linkType() & Element::Slave)) return part.role != QLatin1String("master")
					&& part.role != QLatin1String("slave");
			if (part.role != QLatin1String("slave")) return false;
			const ElementData data = symbol->elementData();
			const QString kind = ElementData::slaveTypeToString(data.m_slave_type);
			const QString state = ElementData::slaveStateToString(data.m_slave_state);
			const QStringList states = statesOf(part);
			return (part.kind.isEmpty() || part.kind == kind)
					&& (states.isEmpty() || states.contains(state));
		}

		QStringList pinsOf(Element *symbol)
		{
			QStringList names;
			for (Terminal *terminal : symbol->terminals()) names << terminal->baseName().trimmed();
			return names;
		}
	}

	QList<int> automatic(const DeviceTypeCheck::Device &device, const DeviceType &type)
	{
		QList<int> parts;
		QList<bool> taken(type.parts.size(), false);
		for (int i = 0 ; i < device.symbols.size() ; ++i) {
			Element *symbol = device.symbols.at(i);
			const int old = device.drawn.value(i, DeviceTypeCheck::Beyond);
			int found = Beyond;
			if (symbol) {
				for (int part = 0 ; part < type.parts.size() && found == Beyond ; ++part) {
					if (taken.at(part)) continue;
					const DeviceType::Part &candidate = type.parts.at(part);
					if (old >= 0 && device.known()) {
						if (alike(candidate, device.type.parts.at(old))) found = part;
					}
					else if (suits(candidate, symbol)) found = part;
				}
					//Nothing alike: any free part it may draw
				for (int part = 0 ; part < type.parts.size() && found == Beyond && symbol ; ++part)
					if (!taken.at(part) && suits(type.parts.at(part), symbol)) found = part;
			}
			if (found >= 0) taken[found] = true;
			parts << found;
		}
		return parts;
	}

	QString symbolText(const DeviceTypeCheck::Device &device, int symbol)
	{
		Element *element = device.symbols.value(symbol);
		if (!element) return QString();
		const QString label = element->elementInformations().value(QETInformation::ELMT_LABEL).toString();
		QString where;
		if (Diagram *diagram = element->diagram())
			//: %1 is a folio's number, %2 a position on it such as "C5"
			where = QObject::tr("folio %1, %2").arg(diagram->folioIndex() + 1)
					.arg(diagram->convertPosition(element->scenePos()).toString());
		QString text = element->name();
		if (!label.isEmpty()) text += QStringLiteral(" ") + label;
		const QStringList pins = pinsOf(element);
		if (!pins.join(QString()).isEmpty()) text += QStringLiteral(" ") + pins.join(QLatin1Char(' '));
		return where.isEmpty() ? text : QObject::tr("%1 (%2)").arg(text, where);
	}

	bool apply(const DeviceTypeCheck::Device &device, const DeviceType &type,
			   const QList<int> &parts, QString *error)
	{
		Element *head = device.symbols.value(0);
		Diagram *diagram = head ? head->diagram() : nullptr;
		QETProject *project = diagram ? diagram->project() : nullptr;
		if (!project || type.isNull()) {
			if (error) *error = QObject::tr("This device cannot be changed.");
			return false;
		}
		QUndoStack &stack = diagram->undoStack();
		//: %1 is a device type's name, such as "CONTR10"
		stack.beginMacro(QObject::tr("Change the device type to %1").arg(type.name));
		if (project->deviceType(type.name).isNull())
			stack.push(new SetDeviceTypeCommand(project, type));

			//Every symbol of the device names the new type
		QMap<QPointer<Element>, QPair<DiagramContext, DiagramContext>> infos;
		for (const auto &symbol : device.symbols) {
			if (!symbol) continue;
			const DiagramContext old_info = symbol->elementInformations();
			DiagramContext new_info = old_info;
			new_info.addValue(QETInformation::ELMT_DEVICE_TYPE, type.name);
			if (old_info != new_info) infos.insert(symbol, qMakePair(old_info, new_info));
		}
		if (!infos.isEmpty()) stack.push(new ChangeElementInformationCommand(infos));

			//Each symbol takes the terminals of its part of the new type
		bool done = true;
		for (int i = 0 ; i < device.symbols.size() ; ++i) {
			const int part = parts.value(i, Beyond);
			Element *symbol = device.symbols.at(i);
			if (part < 0 || !symbol) continue;
			bool fits = false;
			for (const DeviceType::Symbol &way : type.parts.at(part).symbols)
				if (way.pins == pinsOf(symbol)) fits = true;
			if (fits || type.parts.at(part).symbols.isEmpty()) continue;
			const DeviceTypeCheck::Device now = DeviceTypeAssign::deviceOf(symbol, type);
			bool applied = false;
			for (const DeviceTypeAssign::Choice &choice : DeviceTypeAssign::choices(symbol, now))
				if (choice.part == part && choice.possible && choice.own_drawing) {
					applied = DeviceTypeAssign::apply(symbol, now, choice, error) != nullptr;
					break;
				}
			done = done && applied;
		}
		stack.endMacro();
		return done;
	}

	QString partNumberType(Element *head)
	{
		if (!head) return QString();
		const QString part = head->elementInformations()
				.value(QETInformation::ELMT_MANUFACTURER_REF).toString().trimmed();
		if (part.isEmpty()) return QString();
		QString path = MaterialList::configuredPath();
		if (path.isEmpty()) path = MaterialList::defaultPath();
		MaterialListData data;
		if (!MaterialList::load(path, &data)) return QString();
		for (const MaterialRecord &record : std::as_const(data.records))
			if (record.value(QStringLiteral("manufacturer_reference")).trimmed() == part)
				return record.value(QStringLiteral("device_type")).trimmed();
		return QString();
	}
}
