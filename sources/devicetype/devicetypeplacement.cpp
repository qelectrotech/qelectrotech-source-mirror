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
#include "devicetypeplacement.h"

#include "../ElementsCollection/xmlelementcollection.h"
#include "../diagram.h"
#include "../diagramevent/diagrameventaddelement.h"
#include "../duplicatelabels.h"
#include "../factory/elementfactory.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/structureboxitem.h"
#include "../qetinformation.h"
#include "../qetproject.h"
#include "../undocommand/addgraphicsobjectcommand.h"
#include "../undocommand/linkelementcommand.h"

#include <QCursor>
#include <QDomDocument>
#include <QGraphicsView>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <QUuid>

#include <memory>

namespace DeviceTypePlacement
{
	namespace
	{
		Element *headOf(const DeviceTypeCheck::Device &device)
		{
			return device.symbols.isEmpty() ? nullptr : device.symbols.first().data();
		}

		QETProject *projectOf(const DeviceTypeCheck::Device &device)
		{
			Element *head = headOf(device);
			return head && head->diagram() ? head->diagram()->project() : nullptr;
		}

			/// Does the part become a contact linked to the device's master?
		bool linksToMaster(const DeviceTypeCheck::Device &device, const DeviceType::Part &part)
		{
			Element *head = headOf(device);
			return head && part.role == QLatin1String("slave")
					&& part.kind != QLatin1String("plc")
					&& head->linkType() == Element::Master
					&& head->elementData().m_master_type != ElementData::PLC;
		}

			/// The state of a contact drawn this way; NO when the type does not say
		QString stateOf(const DeviceType::Part &part, const DeviceType::Symbol &symbol)
		{
			if (!symbol.state.isEmpty()) return symbol.state;
			return part.state.isEmpty() ? QStringLiteral("NO") : part.state;
		}

			/// The value of kindInformation @p name of a definition
		QString kindOf(const QDomElement &definition, const QString &name)
		{
			const QDomElement kinds = definition.firstChildElement(QStringLiteral("kindInformations"));
			for (QDomElement k = kinds.firstChildElement(QStringLiteral("kindInformation")) ; !k.isNull()
				 ; k = k.nextSiblingElement(QStringLiteral("kindInformation")))
				if (k.attribute(QStringLiteral("name")) == name) return k.text();
			return QString();
		}

		QList<QDomElement> terminalsOf(const QDomElement &definition)
		{
			QList<QDomElement> list;
			const QDomElement description = definition.firstChildElement(QStringLiteral("description"));
			for (QDomElement t = description.firstChildElement(QStringLiteral("terminal")) ; !t.isNull()
				 ; t = t.nextSiblingElement(QStringLiteral("terminal")))
				list << t;
			return list;
		}

			/**
				The names @p terminals take for @p pins, in their order: one
				pin each, or, when several connection points of the symbol
				share a name (a strip terminal drawn with three), one pin for
				each name. Empty when the pins do not fit the symbol.
			*/
		QStringList pinNames(const QList<QDomElement> &terminals, const QStringList &pins)
		{
			if (pins.isEmpty()) return {};
			if (terminals.size() == pins.size()) return pins;
			QStringList distinct;
			for (const QDomElement &t : terminals)
				if (!distinct.contains(t.attribute(QStringLiteral("name"))))
					distinct << t.attribute(QStringLiteral("name"));
			if (distinct.size() != pins.size()) return {};
			QStringList names;
			for (const QDomElement &t : terminals)
				names << pins.at(distinct.indexOf(t.attribute(QStringLiteral("name"))));
			return names;
		}

			/// Where the mouse is on @p diagram's first view; the origin without one
		QPointF mouseOn(Diagram *diagram)
		{
			QGraphicsView *view = diagram->views().value(0);
			return view ? view->mapToScene(view->mapFromGlobal(QCursor::pos())) : QPointF();
		}

		void setKinds(QDomDocument &document, QDomElement &definition, const QString &type,
					  const QString &state)
		{
			QDomElement old = definition.firstChildElement(QStringLiteral("kindInformations"));
			QDomElement kinds = document.createElement(QStringLiteral("kindInformations"));
			const auto add = [&](const QString &name, const QString &value) {
				QDomElement k = document.createElement(QStringLiteral("kindInformation"));
				k.setAttribute(QStringLiteral("name"), name);
				k.appendChild(document.createTextNode(value));
				kinds.appendChild(k);
			};
			add(QStringLiteral("type"), type);
			add(QStringLiteral("state"), state);
			if (old.isNull())
				definition.insertBefore(kinds, definition.firstChildElement(QStringLiteral("description")));
			else
				definition.replaceChild(kinds, old);
		}
	}

	/**
		@return the ways part @p part of @p device can be placed: its symbols
		which exist in the project. None for a PLC point.
	*/
	QList<int> placeableWays(const DeviceTypeCheck::Device &device, int part)
	{
		QList<int> ways;
		QETProject *project = projectOf(device);
		if (!project || project->isReadOnly() || part < 0 || part >= device.type.parts.size())
			return ways;
		const DeviceType::Part &p = device.type.parts.at(part);
		if (p.kind == QLatin1String("plc")) return ways;
		for (int way = 0 ; way < p.symbols.size() ; ++way) {
			const QString location = p.symbols.at(way).location;
			if (!location.isEmpty() && DeviceTypeCheck::symbolLocation(location, project).exist())
				ways << way;
		}
		return ways;
	}

	/**
		@return how a way of drawing a part reads in a menu: "NC 2 3"
	*/
	QString wayText(const DeviceType::Part &part, int way)
	{
		const DeviceType::Symbol &symbol = part.symbols.at(way);
		QStringList words;
		if (!symbol.state.isEmpty()) words << symbol.state;
		words << symbol.pins;
		return words.join(QLatin1Char(' '));
	}

	/**
		@return the element to place for this way of drawing the part: its
		symbol, or the project's copy of it with the part's pins, contact
		kind and state. An invalid location when it cannot be placed.
		@param own_drawing : when valid, the drawing to take instead of the
		part's: the project gets a copy of it with the part's pins
	*/
	ElementsLocation symbolFor(const DeviceTypeCheck::Device &device, int part, int way,
							   const ElementsLocation &own_drawing)
	{
		const bool own = !own_drawing.toString().isEmpty();
		if (part < 0 || part >= device.type.parts.size()
				|| way < 0 || way >= device.type.parts.at(part).symbols.size()
				|| (!own && !placeableWays(device, part).contains(way)))
			return ElementsLocation();
		QETProject *project = projectOf(device);
		const DeviceType::Part &p = device.type.parts.at(part);
		const DeviceType::Symbol &symbol = p.symbols.at(way);
		ElementsLocation base = own ? own_drawing : DeviceTypeCheck::symbolLocation(symbol.location, project);

		QDomDocument document;
		document.appendChild(document.importNode(base.xml(), true));
		QDomElement definition = document.documentElement();

		const bool link = linksToMaster(device, p);
		const QString state = stateOf(p, symbol);
		const QString type = p.kind.isEmpty() ? QStringLiteral("simple") : p.kind;
		bool changed = false;
		if (link && (definition.attribute(QStringLiteral("link_type")) != QLatin1String("slave")
					 || kindOf(definition, QStringLiteral("state")) != state
					 || kindOf(definition, QStringLiteral("type")) != type)) {
			definition.setAttribute(QStringLiteral("link_type"), QStringLiteral("slave"));
			setKinds(document, definition, type, state);
			changed = true;
		}
			//A drawing from the user's collection says what it is in its
			//library, not in this device: a contact with no coil here is a
			//plain symbol
		if (!base.isProject() && !link
				&& definition.attribute(QStringLiteral("link_type")) == QLatin1String("slave")) {
			definition.setAttribute(QStringLiteral("link_type"), QStringLiteral("simple"));
			QDomElement kinds = definition.firstChildElement(QStringLiteral("kindInformations"));
			if (!kinds.isNull()) definition.removeChild(kinds);
			changed = true;
		}
			//A plain drawing taken as the device's head: contacts link to it
		if ((own || !base.isProject()) && p.role == QLatin1String("master")
				&& definition.attribute(QStringLiteral("link_type")) == QLatin1String("simple")) {
			definition.setAttribute(QStringLiteral("link_type"), QStringLiteral("master"));
			QDomElement kinds = definition.firstChildElement(QStringLiteral("kindInformations"));
			if (!kinds.isNull()) definition.removeChild(kinds);
			kinds = document.createElement(QStringLiteral("kindInformations"));
			QDomElement kind = document.createElement(QStringLiteral("kindInformation"));
			kind.setAttribute(QStringLiteral("name"), QStringLiteral("type"));
			kind.appendChild(document.createTextNode(
								 p.kind.isEmpty() ? QStringLiteral("coil") : p.kind));
			kinds.appendChild(kind);
			definition.insertBefore(kinds, definition.firstChildElement(QStringLiteral("description")));
			changed = true;
		}
		QList<QDomElement> terminals = terminalsOf(definition);
		const QStringList names = pinNames(terminals, symbol.pins);
		for (int i = 0 ; i < names.size() ; ++i) {
			if (terminals[i].attribute(QStringLiteral("name")) != names.at(i)) {
				terminals[i].setAttribute(QStringLiteral("name"), names.at(i));
				changed = true;
			}
		}
		if (!changed) return base;

			//A copy of the symbol beside it, one per part it can be
			//A drawing of the user's collection: the copy goes in the
			//project's folder of device type drawings
		const QString path = base.collectionPath(false);
		const QString folder = base.isProject() ? path.section(QLatin1Char('/'), 0, -2)
												: QStringLiteral("import/devicetypes");
		QString suffix = (link ? state : p.role == QLatin1String("master") && (own || !base.isProject())
							  ? QStringLiteral("master") : QStringLiteral("pins")) + QLatin1Char('_')
				+ symbol.pins.join(QLatin1Char('-'));
		suffix.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_-]")), QStringLiteral("_"));
		const QString name = base.baseName() + QLatin1Char('_') + suffix;
		const QString copy = folder + QLatin1Char('/') + name + QStringLiteral(".elmt");
		auto *collection = project->embeddedElementCollection();
		if (!collection->exist(folder)) {
			collection->createDir(QStringLiteral("import"), QStringLiteral("devicetypes"), NamesList());
		}
		if (!collection->exist(copy)) {
			QDomElement uuid = definition.firstChildElement(QStringLiteral("uuid"));
			if (!uuid.isNull())
				uuid.setAttribute(QStringLiteral("uuid"), QUuid::createUuid().toString());
			if (!collection->addElementDefinition(folder, name, definition))
				return ElementsLocation();
		}
		return ElementsLocation(QStringLiteral("embed://") + copy, project);
	}

	/**
		@brief finish
		@p placed has just been placed, as part @p part of @p device: it
		takes what the type gives its symbols, and, as children of @p undo,
		its link to the master. Without a master to link to, it takes the
		device's label and place.
	*/
	void finish(Element *placed, const DeviceTypeCheck::Device &device, int part,
				int way, QUndoCommand *undo)
	{
		Element *head = headOf(device);
		if (!placed || !head) return;

		DiagramContext info = placed->elementInformations();
		for (const auto &given : device.type.informations)
			info.addValue(given.first, given.second);

		const bool link = linksToMaster(device, device.type.parts.at(part))
				&& LinkElementCommand::isLinkable(placed, head);
		if (!link) {
			QString label = device.label;
			const QStringList pins = device.type.parts.at(part).symbols.at(way).pins;
			if ((placed->linkType() & Element::Terminale) && !pins.isEmpty())
				label += QLatin1Char(':') + pins.first();
			info.addValue(QETInformation::ELMT_LABEL, label);
			info.remove(QETInformation::ELMT_FORMULA);
			if (DuplicateLabels::place(placed) != DuplicateLabels::place(head)) {
				const StructureBoxItem::Place place = StructureBoxItem::placeOf(head);
				info.addValue(QStringLiteral("plant"), place.plant.trimmed());
				info.addValue(QStringLiteral("location"), place.location.trimmed());
			}
		}
		placed->setElementInformations(info);

		if (link) {
			auto *command = new LinkElementCommand(placed, undo);
			command->setLink(head);
		}
	}

	/**
		@return the placing of this way of drawing the part, for
		Diagram::setEventInterface(): the symbol follows the mouse and is
		dropped once. nullptr when the part cannot be placed.
	*/
	DiagramEventAddElement *begin(const DeviceTypeCheck::Device &device, int part,
								  int way, Diagram *diagram, QPointF pos)
	{
		if (!diagram || diagram->isReadOnly()) return nullptr;
		ElementsLocation location = symbolFor(device, part, way);
		if (!location.exist()) return nullptr;
		auto *event = new DiagramEventAddElement(location, diagram, pos);
		event->placeOnce([device, part, way](Element *placed, QUndoCommand *undo) {
			finish(placed, device, part, way, undo);
		});
		return event;
	}

	/**
		@return the parts of @p device not drawn yet which can be placed
		here: a symbol of theirs is in the project. Not the PLC points.
	*/
	QList<int> freeParts(const DeviceTypeCheck::Device &device)
	{
		QList<int> parts;
		if (!device.known()) return parts;
		for (int part : device.missing)
			if (!placeableWays(device, part).isEmpty()) parts << part;
		return parts;
	}

	/**
		@brief placeSideBySide
		Places the free parts of @p device in a row to the right of its
		head, on its folio, each in its first way, as one undoable step:
		to be moved where they belong.
		@return the symbols placed
	*/
	QList<Element *> placeSideBySide(const DeviceTypeCheck::Device &device)
	{
		QList<Element *> placed;
		Element *head = headOf(device);
		Diagram *diagram = head ? head->diagram() : nullptr;
		if (!diagram || diagram->isReadOnly()) return placed;
		const QList<int> parts = freeParts(device);
		if (parts.isEmpty()) return placed;

		//: %1 is the device's label, such as "-K1"
		auto *undo = new QUndoCommand(QObject::tr("Place the parts of %1").arg(device.label));
		qreal left = head->sceneBoundingRect().right() + 2 * Diagram::xGrid;
		for (int part : parts) {
			const int way = placeableWays(device, part).first();
			const ElementsLocation location = symbolFor(device, part, way);
			if (!location.exist()) continue;
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(location, nullptr, &state);
			if (state || !element) {
				delete element;
				continue;
			}
			const QPointF pos = Diagram::snapToGrid(QPointF(left - element->boundingRect().left(),
															head->pos().y()));
			diagram->addItem(element);
			element->setPos(pos);
			new AddGraphicsObjectCommand(element, diagram, pos, undo);
			finish(element, device, part, way, undo);
			left = element->sceneBoundingRect().right() + 2 * Diagram::xGrid;
			placed << element;
		}
		if (placed.isEmpty()) {
			delete undo;
			return placed;
		}
		diagram->undoStack().push(undo);
		for (Element *element : std::as_const(placed))
			element->freezeNewAddedElement();
		return placed;
	}

	/**
		@brief placeOneByOne
		The first free part of @p device follows the mouse on @p diagram;
		once dropped, the next one, until none is left or the placing is
		given up (Escape, a right click), which leaves the rest free.
		@return false when there is nothing to place
	*/
	bool placeOneByOne(const DeviceTypeCheck::Device &device, Diagram *diagram, QPointF pos)
	{
		const QList<int> parts = freeParts(device);
		QPointer<Element> head = headOf(device);
		if (parts.isEmpty() || !head || !diagram || diagram->project() != projectOf(device))
			return false;
		const int part = parts.first();
		const int way = placeableWays(device, part).first();
		DiagramEventAddElement *event = begin(device, part, way, diagram, pos);
		if (!event) return false;

		auto dropped = std::make_shared<bool>(false);
		event->placeOnce([device, part, way, dropped](Element *placed, QUndoCommand *undo) {
			finish(placed, device, part, way, undo);
			*dropped = true;
		});
		QPointer<Diagram> folio = diagram;
		QObject::connect(event, &DiagramEventInterface::finish, diagram, [head, folio, dropped]() {
			if (!*dropped) return;
				//Once the placing just ended is gone
			QTimer::singleShot(0, folio, [head, folio]() {
				if (head && folio && !folio->eventInterface())
					placeOneByOne(DeviceTypeCheck::checkDevice(head), folio, mouseOn(folio));
			});
		});
		diagram->setEventInterface(event);
		return true;
	}

	/**
		@brief offerParts
		@p head has just been placed: with askAfterHead() on and parts of
		its device type not drawn yet, ask how to go on with them.
	*/
	void offerParts(Element *head, QWidget *parent)
	{
		if (!askAfterHead() || !head || !head->diagram()) return;
		const DeviceTypeCheck::Device device = DeviceTypeCheck::checkDevice(head);
		if (device.symbols.value(0) != head) return;
		const QList<int> parts = freeParts(device);
		if (parts.isEmpty()) return;

		QStringList texts;
		for (int part : parts)
			texts << DeviceTypeCheck::partText(device.type.parts.at(part));
		QMessageBox box(QMessageBox::Question,
						//: %1 is the device's label, such as "-K1"
						QObject::tr("Parts of %1").arg(device.label),
						//: %1 is the device's label, %2 its parts, as "power contacts 1 2 3 4 5 6, NO 13 14"
						QObject::tr("%1 has parts not drawn yet: %2.\nHow do you want to place them?")
						.arg(device.label, texts.join(QStringLiteral(", "))),
						QMessageBox::NoButton, parent);
		QPushButton *one = box.addButton(QObject::tr("One by one"), QMessageBox::AcceptRole);
		one->setToolTip(QObject::tr("Each part follows the mouse until a click; Escape stops, the rest stay free"));
		QPushButton *all = box.addButton(QObject::tr("All side by side"), QMessageBox::AcceptRole);
		all->setToolTip(QObject::tr("All parts to the right of the head, to be moved where they belong"));
		QPushButton *later = box.addButton(QObject::tr("Not now"), QMessageBox::RejectRole);
		later->setToolTip(QObject::tr("The parts stay free in the Device Navigator"));
		box.setDefaultButton(one);
		box.exec();
		if (!head || !head->diagram()) return;
		if (box.clickedButton() == one)
			placeOneByOne(DeviceTypeCheck::checkDevice(head), head->diagram(), mouseOn(head->diagram()));
		else if (box.clickedButton() == all)
			placeSideBySide(DeviceTypeCheck::checkDevice(head));
	}

		/// Ask how to place a device's parts after its head; off by default
	bool askAfterHead()
	{
		return QSettings().value(QStringLiteral("diagrameditor/ask_parts_after_head"), false).toBool();
	}

	void setAskAfterHead(bool ask)
	{
		QSettings().setValue(QStringLiteral("diagrameditor/ask_parts_after_head"), ask);
	}
}
