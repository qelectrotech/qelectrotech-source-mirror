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
#include "devicetypecheck.h"

#include "../ElementsCollection/elementslocation.h"
#include "../diagram.h"
#include "../duplicatelabels.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetinformation.h"
#include "../qetproject.h"

#include <QHash>
#include <QSet>
#include <QObject>

namespace DeviceTypeCheck
{
	namespace
	{
			/// What a symbol is drawn with: its element and that element's name
		struct Drawing
		{
			QString location;
			QString name;
			QStringList pins;   ///< sorted, named ones only
		};

		QStringList sortedPins(QStringList pins)
		{
			pins.removeAll(QString());
			pins.sort();
			return pins;
		}

		Drawing drawingOf(Element *element)
		{
			Drawing d;
			d.location = element->location().collectionPath();
			d.name = element->name();
			QStringList pins;
			for (Terminal *terminal : element->terminals())
				pins << terminal->baseName().trimmed();
			d.pins = sortedPins(pins);
			return d;
		}

			/// A symbol's head: the master it is linked to, else itself
		Element *headOf(Element *element)
		{
			if (element->linkType() & Element::Slave)
				for (Element *linked : element->linkedElements())
					if (linked->linkType() & Element::Master)
						return linked;
			return element;
		}

			/// The label naming a symbol's device: a strip terminal X1:2 is X1
		QString deviceLabel(Element *head)
		{
			QString label = DuplicateLabels::identifier(
						DuplicateLabels::displayedLabel(head));
			if (head->linkType() & Element::Terminale) {
				const int colon = label.indexOf(QLatin1Char(':'));
				if (colon > 0) label = label.left(colon).trimmed();
			}
			return label;
		}

			/// A device's designation: its place, then its label with one
			/// '-', whether or not the label was typed with it (-QA1, QA1)
		QString designationOf(const QString &place, const QString &label)
		{
			if (label.startsWith(QLatin1Char('-')))
				return place + label;
			return place + QLatin1Char('-') + label;
		}

		bool counted(Element *element)
		{
			return !(element->linkType()
					 & (Element::AllReport | Element::ConductorDefinition));
		}

			/// The project's types, each part's symbols as drawings
		class Types
		{
			public:
				explicit Types(QETProject *project) : m_project(project) {}
				const QList<QList<Drawing>> &parts(const DeviceType &type)
				{
					if (!m_parts.contains(type.name)) {
						QList<QList<Drawing>> parts;
						for (const DeviceType::Part &part : type.parts) {
							QList<Drawing> ways;
							for (const DeviceType::Symbol &symbol : part.symbols) {
								Drawing d;
								d.location = symbol.location;
								if (!symbol.location.isEmpty())
									d.name = nameOf(symbol.location);
								d.pins = sortedPins(symbol.pins);
								ways << d;
							}
							parts << ways;
						}
						m_parts.insert(type.name, parts);
					}
					return m_parts[type.name];
				}

			private:
				QString nameOf(const QString &location)
				{
					if (!m_names.contains(location))
						m_names.insert(location,
									   symbolLocation(location, m_project).name());
					return m_names.value(location);
				}
				QETProject *m_project;
				QHash<QString, QList<QList<Drawing>>> m_parts;
				QHash<QString, QString> m_names;
		};

			/// Each symbol to one part: same symbol and pins, then same
			/// symbol, then same pins
		void match(Device &device, Types &types)
		{
			const auto &parts = types.parts(device.type);
			QList<Drawing> drawings;
			for (const auto &symbol : std::as_const(device.symbols))
				drawings << drawingOf(symbol.data());
			device.drawn = QList<int>(drawings.size(), -1);
			QList<bool> taken(parts.size(), false);

			const auto same_symbol = [](const Drawing &way, const Drawing &d) {
				return (!way.location.isEmpty() && way.location == d.location)
						|| (!way.name.isEmpty() && way.name == d.name);
			};
			const auto same_pins = [](const Drawing &way, const Drawing &d) {
				return !d.pins.isEmpty() && way.pins == d.pins;
			};
			for (int stage = 0 ; stage < 3 ; ++stage) {
				for (int s = 0 ; s < drawings.size() ; ++s) {
					if (device.drawn.at(s) >= 0) continue;
					for (int p = 0 ; p < parts.size() && device.drawn.at(s) < 0 ; ++p) {
						if (taken.at(p)) continue;
						for (const Drawing &way : parts.at(p)) {
							const bool hit = stage == 0
									? same_symbol(way, drawings.at(s)) && same_pins(way, drawings.at(s))
									: stage == 1 ? same_symbol(way, drawings.at(s))
												 : same_pins(way, drawings.at(s));
							if (hit) {
								device.drawn[s] = p;
								taken[p] = true;
								break;
							}
						}
					}
				}
			}
				//A drawing without terminals which is no part (a table of the
				//contacts, a picture) connects nothing: not beyond the type
			for (int s = 0 ; s < drawings.size() ; ++s)
				if (device.drawn.at(s) < 0 && device.symbols.at(s)->terminals().isEmpty())
					device.drawn[s] = NotAPart;
			device.missing.clear();
			for (int p = 0 ; p < parts.size() ; ++p)
				if (!taken.at(p)) device.missing << p;
		}

			/// A terminal strip: every symbol of the device is a terminal
		bool isStrip(const Device &device)
		{
			if (device.symbols.isEmpty()) return false;
			for (const auto &symbol : device.symbols)
				if (!symbol || symbol->linkType() != Element::Terminale) return false;
			return true;
		}

			/// A strip is checked terminal by terminal: each terminal is one
			/// device of its own type (a feed-through terminal, an earth
			/// terminal...), so the strip is complete when every terminal
			/// draws its type. A terminal whose type is none of the
			/// project's is beyond it.
		void matchStrip(Device &device, QETProject *project, Types &types)
		{
			device.drawn.clear();
			device.missing.clear();
			for (const auto &symbol : std::as_const(device.symbols)) {
				const QString name = typeNameOf(symbol->elementInformations());
				Device one;
				one.symbols << symbol;
				one.type = project->deviceType(name);
				if (name.isEmpty() || !one.known()) {
					device.drawn << Beyond;
					continue;
				}
				match(one, types);
				device.drawn << one.drawn.value(0, Beyond);
			}
		}

			/// The devices of the project, or only the one named by @p only;
			/// with @p unlinked, the contacts linked to no master, in the
			/// same pass over the folios
		QVector<Device> devices(QETProject *project, Element *only,
								QList<Element *> *unlinked = nullptr)
		{
			QVector<Device> list;
			if (!project) return list;
			const QString only_label = only && counted(only)
					? deviceLabel(headOf(only)) : QString();
			if (only && only_label.isEmpty()) return list;

			QHash<QString, int> index;
			QHash<Element *, QString> place_of;
			const auto placeOf = [&place_of](Element *head) {
				if (!place_of.contains(head))
					place_of.insert(head, DuplicateLabels::place(head));
				return place_of.value(head);
			};
			const bool all = project->followDeviceTypes();
			for (Diagram *diagram : project->diagrams()) {
				for (Element *element : diagram->elements()) {
					if (!counted(element)) continue;
					Element *head = headOf(element);
					if (unlinked && head == element && (element->linkType() & Element::Slave)
						&& (all || !typeNameOf(element->elementInformations()).isEmpty()))
						*unlinked << element;
					const QString label = deviceLabel(head);
					if (label.isEmpty()) continue;
					if (only && label != only_label) continue;
					const QString designation = designationOf(placeOf(head), label);
					int i = index.value(designation, -1);
					if (i < 0) {
						i = list.size();
						index.insert(designation, i);
						Device device;
						device.designation = designation;
						device.label = label;
						list << device;
					}
					Device &device = list[i];
						//The head first: a master, else the first symbol
					if (element->linkType() & Element::Master)
						device.symbols.prepend(element);
					else
						device.symbols.append(element);
				}
			}
			if (only) {
				const QString wanted = designationOf(placeOf(headOf(only)), only_label);
				QVector<Device> one;
				if (index.contains(wanted)) one << list.at(index.value(wanted));
				list = one;
			}

			Types types(project);
			for (Device &device : list) {
				for (const auto &symbol : std::as_const(device.symbols)) {
					device.typeName = typeNameOf(symbol->elementInformations());
					if (!device.typeName.isEmpty()) break;
				}
				device.type = project->deviceType(device.typeName);
				if (isStrip(device))
					matchStrip(device, project, types);
				else if (device.known())
					match(device, types);
			}
			return list;
		}
	}

	ElementsLocation symbolLocation(const QString &location, QETProject *project)
	{
		return location.startsWith(QLatin1String("embed://"))
				? ElementsLocation(location, project) : ElementsLocation(location);
	}

	QString typeNameOf(const DiagramContext &info)
	{
		return info.value(QETInformation::ELMT_DEVICE_TYPE).toString().trimmed();
	}

	/**
		@return the project's devices which have a device type, in folio order
	*/
	QVector<Device> check(QETProject *project, QList<Element *> *unlinked)
	{
		QVector<Device> list;
		if (unlinked) unlinked->clear();
		if (project && project->unloadedFolioCount())
			project->loadFolios();
		for (const Device &device : devices(project, nullptr, unlinked))
			if (!device.typeName.isEmpty()) list << device;
		return list;
	}

	/**
		@return the device @p element is a symbol of; no designation when it
		belongs to none (a folio report, a symbol without label). Reads the
		folios built so far only: quick enough for a properties panel.
	*/
	Device checkDevice(Element *element)
	{
		if (!element || !element->diagram()) return Device();
		const auto list = devices(element->diagram()->project(), element);
		return list.isEmpty() ? Device() : list.first();
	}

	int extraContacts(const Device &device)
	{
		if (!device.known()) return 0;
			//Room for a linked contact: a part one draws (match() pairs by
			//symbol and pins, not by role), or a contact part nothing draws;
			//never the master's part, nor a part another symbol took. So a
			//complete device never has too many.
		QSet<int> by_contact;
		int contacts = 0;
		for (int s = 0 ; s < device.symbols.size() ; ++s) {
			Element *symbol = device.symbols.at(s).data();
			if (!symbol || !(symbol->linkType() & Element::Slave) || headOf(symbol) == symbol)
				continue;
			const int part = device.drawn.value(s, Beyond);
				//A contact table or picture without terminals is no contact
			if (part == NotAPart) continue;
			++contacts;
			if (part >= 0) by_contact.insert(part);
		}
		int room = 0;
		for (int p = 0 ; p < device.type.parts.size() ; ++p) {
			const QString &role = device.type.parts.at(p).role;
			if (role == QLatin1String("master")) continue;
			if (by_contact.contains(p)
				|| (role == QLatin1String("slave") && device.missing.contains(p)))
				++room;
		}
		return qMax(0, contacts - room);
	}

	/**
		@return how a part reads in a list: "contact NO 13 14"
	*/
	QString partText(const DeviceType::Part &part)
	{
		QStringList words;
		words << (part.kind.isEmpty() ? part.role : part.kind);
		QString state = part.state;
		QStringList pins;
		if (!part.symbols.isEmpty()) {
			if (state.isEmpty()) state = part.symbols.first().state;
			pins = part.symbols.first().pins;
		}
		if (!state.isEmpty()) words << state;
		if (!pins.isEmpty()) words << pins.join(QLatin1Char(' '));
		return words.join(QLatin1Char(' '));
	}
}
