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
#include "devicetypeassign.h"
#include "../qetinformation.h"

#include "devicetypeplacement.h"
#include "../ElementsCollection/elementslocation.h"
#include "../diagram.h"
#include "../conductorprofile.h"
#include "../diagramcontent.h"
#include "../factory/elementfactory.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/dynamicelementtextitem.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetproject.h"
#include "../undocommand/addgraphicsobjectcommand.h"
#include "../undocommand/deleteqgraphicsitemcommand.h"
#include "../undocommand/linkelementcommand.h"

#include <QDomDocument>
#include <QObject>
#include <QUndoCommand>

namespace DeviceTypeAssign
{
	namespace
	{
			/// Can a drawing with these terminal names take these pins?
		bool pinsFit(const QStringList &names, const QStringList &pins)
		{
			if (pins.isEmpty()) return false;
			if (names.size() == pins.size()) return true;
			QStringList distinct = names;
			distinct.removeDuplicates();
			return distinct.size() == pins.size();
		}

		QStringList namesOf(Element *symbol)
		{
			QStringList names;
			for (Terminal *terminal : symbol->terminals())
				names << terminal->baseName().trimmed();
			return names;
		}

			/// Each wired terminal of @p from to one of @p to: its name, else its place in the list
		bool mapTerminals(Element *from, Element *to, QHash<Terminal *, Terminal *> *map)
		{
			const QList<Terminal *> old_ones = from->terminals();
			const QList<Terminal *> new_ones = to->terminals();
			QList<Terminal *> free = new_ones;
			QStringList names_old, names_new;
			for (Terminal *terminal : old_ones) names_old << terminal->baseName().trimmed();
			for (Terminal *terminal : new_ones) names_new << terminal->baseName().trimmed();
			names_old.sort();
			names_new.sort();
				//The same number of terminals under other names is the same
				//drawing renamed: place by place. Else by name.
			const bool by_place = new_ones.size() == old_ones.size() && names_old != names_new;
			for (Terminal *terminal : old_ones) {
				const QString name = terminal->baseName().trimmed();
				if (name.isEmpty() || by_place) continue;
				for (Terminal *candidate : std::as_const(free))
					if (candidate->baseName().trimmed() == name) {
						map->insert(terminal, candidate);
						free.removeOne(candidate);
						break;
					}
			}
			for (int i = 0 ; i < old_ones.size() ; ++i) {
				if (map->contains(old_ones.at(i))) continue;
				if (new_ones.size() == old_ones.size())
					map->insert(old_ones.at(i), new_ones.at(i));
				else if (!old_ones.at(i)->conductors().isEmpty())
					return false;
			}
			return true;
		}
	}

	/**
		@return the device @p symbol belongs to, of type @p type; for a
		symbol the project sees as no device (no label yet), a device of
		that symbol alone, none of whose parts is drawn
	*/
	DeviceTypeCheck::Device deviceOf(Element *symbol, const DeviceType &type)
	{
		DeviceTypeCheck::Device device = DeviceTypeCheck::checkDevice(symbol);
		if (!device.symbols.isEmpty() && device.type.name == type.name) return device;
		device = DeviceTypeCheck::Device();
		device.typeName = type.name;
		device.type = type;
		device.symbols << symbol;
		device.drawn << DeviceTypeCheck::Beyond;
		for (int part = 0 ; part < type.parts.size() ; ++part) device.missing << part;
		return device;
	}

	/**
		@return what @p symbol can become: each way of each part the
		device does not draw yet, and the part the symbol draws now
	*/
	QList<Choice> choices(Element *symbol, const DeviceTypeCheck::Device &device)
	{
		QList<int> parts = device.missing;
		for (int s = 0 ; s < device.symbols.size() ; ++s)
			if (device.symbols.at(s).data() == symbol && device.drawn.value(s, -1) >= 0)
				parts.prepend(device.drawn.at(s));

		QList<Choice> list;
		const QStringList names = namesOf(symbol);
		for (int part : std::as_const(parts)) {
			const DeviceType::Part &p = device.type.parts.at(part);
			for (int way = 0 ; way < p.symbols.size() ; ++way) {
				const QString what = DeviceTypePlacement::wayText(p, way);
				Choice own;
				own.part = part;
				own.way = way;
				own.own_drawing = true;
				own.text = QObject::tr("%1: keep this drawing", "a part of a device type").arg(what);
				own.possible = pinsFit(names, p.symbols.at(way).pins);
				if (!own.possible)
					own.reason = QObject::tr("This drawing has %n terminal(s), which do not fit this part.", nullptr, int(names.size()));
				list << own;
				if (DeviceTypePlacement::placeableWays(device, part).contains(way)) {
					Choice theirs = own;
					theirs.own_drawing = false;
					theirs.text = QObject::tr("%1: use the type's drawing", "a part of a device type").arg(what);
					theirs.possible = true;
					theirs.reason.clear();
					list << theirs;
				}
			}
		}
		return list;
	}

	/**
		@brief apply
		@return the symbol now drawing the part: a new one, or @p symbol
		when it already draws it so; nullptr, with @p error, when it cannot
		be done (nothing is changed)
	*/
	Element *apply(Element *symbol, const DeviceTypeCheck::Device &device,
				   const Choice &choice, QString *error)
	{
		const auto fail = [error](const QString &why) {
			if (error) *error = why;
			return nullptr;
		};
		Diagram *diagram = symbol ? symbol->diagram() : nullptr;
		if (!diagram || diagram->isReadOnly())
			return fail(QObject::tr("This folio cannot be changed."));
		ElementsLocation target = DeviceTypePlacement::symbolFor(
					device, choice.part, choice.way,
					choice.own_drawing ? symbol->location() : ElementsLocation());
			//A drawing of the user's collection: the project keeps a copy
		if (!target.toString().isEmpty() && !target.isProject())
			target = diagram->project()->importElement(target);
		if (target.toString().isEmpty())
			return fail(QObject::tr("The device type gives no usable drawing for this part."));
		if (target == symbol->location()) return symbol;

		int state = 0;
		Element *fresh = ElementFactory::Instance()->createElement(target, nullptr, &state);
		if (state || !fresh) {
			delete fresh;
			return fail(QObject::tr("The drawing of this part could not be read."));
		}

		QHash<Terminal *, Terminal *> terminals;
		if (!mapTerminals(symbol, fresh, &terminals)) {
			delete fresh;
			return fail(QObject::tr("This drawing has other terminals than the symbol, and the symbol is wired: "
									"disconnect its wires first."));
		}
		DiagramContent removed;
		removed.m_elements << symbol;
		if (DeleteQGraphicsItemCommand::hasNonDeletableTerminal(removed)) {
			delete fresh;
			return fail(QObject::tr("A terminal of this symbol belongs to a bridge or a strip, so it cannot be replaced."));
		}

		fresh->setRotation(symbol->rotation());
		fresh->setHorizontalMirror(symbol->hasHorizontalMirror());
		fresh->setVerticalMirror(symbol->hasVerticalMirror());

			//The symbol is of the type; what else the type gives fills only
			//what the symbol has not got
		DiagramContext info = symbol->elementInformations();
		info.addValue(QETInformation::ELMT_DEVICE_TYPE, device.type.name);
		for (const auto &given : device.type.informations) {
			if (info.value(given.first).toString().trimmed().isEmpty())
				info.addValue(given.first, given.second);
		}
		fresh->setElementInformations(info);

		const auto copied_texts = fresh->dynamicTextItems();
		if (!symbol->dynamicTextItems().isEmpty()) {
			for (DynamicElementTextItem *text : copied_texts) {
				fresh->removeDynamicTextItem(text);
				delete text;
			}
			QDomDocument document;
			for (DynamicElementTextItem *text : symbol->dynamicTextItems()) {
				auto *copy = new DynamicElementTextItem(fresh);
				copy->fromXml(text->toXml(document));
				fresh->addDynamicTextItem(copy);
			}
		}

		struct Wire {Terminal *one; Terminal *two; ConductorProperties properties; ConductorProfilesGroup profiles;};
		QList<Wire> wires;
		const auto ends = [&terminals](Terminal *t) {return terminals.value(t, t);};
		DiagramContent old_wires;
		for (Conductor *conductor : symbol->conductors()) {
			if (old_wires.m_other_conductors.contains(conductor)) continue;
			old_wires.m_other_conductors << conductor;
			wires << Wire{ends(conductor->terminal1), ends(conductor->terminal2),
						  conductor->properties(), conductor->profiles()};
		}
		const QList<Element *> linked = symbol->linkedElements();
		const bool was_head = !device.symbols.isEmpty() && device.symbols.first().data() == symbol;

		const QString text = QObject::tr("Give a symbol the terminals of its part");
		diagram->undoStack().beginMacro(text);
			//Its wires go first: deleting a symbol with several wires on a
			//terminal joins their other ends, as the new ones will be
		if (!old_wires.m_other_conductors.isEmpty())
			diagram->undoStack().push(new DeleteQGraphicsItemCommand(diagram, old_wires));
		auto *undo = new QUndoCommand(text);
		new DeleteQGraphicsItemCommand(diagram, removed, undo);
		new AddGraphicsObjectCommand(fresh, diagram, symbol->pos(), undo);
		for (const Wire &wire : std::as_const(wires)) {
			auto *conductor = new Conductor(wire.one, wire.two);
			conductor->setProperties(wire.properties);
			conductor->setProfiles(wire.profiles);
			new AddGraphicsObjectCommand(conductor, diagram, QPointF(), undo);
		}
			//The links come after: a contact can link to the new symbol
			//only once the one it was linked to has gone. One undo step.
		diagram->undoStack().push(undo);
		auto *links = new QUndoCommand(text);
		if (!linked.isEmpty() && fresh->linkType() == symbol->linkType()) {
			auto *link = new LinkElementCommand(fresh, links);
			link->setLink(linked);
		} else if (!was_head) {
			DeviceTypePlacement::finish(fresh, device, choice.part, choice.way, links);
		}
		if (links->childCount())
			diagram->undoStack().push(links);
		else
			delete links;
		diagram->undoStack().endMacro();
		return fresh;
	}
}
