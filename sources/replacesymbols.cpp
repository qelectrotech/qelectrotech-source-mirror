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
#include "replacesymbols.h"

#include "diagram.h"
#include "conductorprofile.h"
#include "diagramcontent.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "undocommand/addgraphicsobjectcommand.h"
#include "qgimanager.h"
#include "undocommand/deleteqgraphicsitemcommand.h"

#include <QHash>
#include <QLineF>
#include <QUndoCommand>

namespace {
	/**
		Take the old symbols and their wires off the folio, and put them
		back on undo. Not DeleteQGraphicsItemCommand: that one joins the
		wires of a removed terminal to keep their potential, and here the
		wires come back on the new symbol instead.
	*/
	class RemoveItemsCommand : public QUndoCommand
	{
		public:
			RemoveItemsCommand(Diagram *diagram, const QList<QGraphicsObject *> &items,
							   QUndoCommand *parent) :
				QUndoCommand(parent), m_diagram(diagram), m_items(items)
			{
				for (QGraphicsObject *item : std::as_const(m_items))
					m_diagram->qgiManager().manage(item);
			}
			~RemoveItemsCommand() override
			{
				for (QGraphicsObject *item : std::as_const(m_items))
					m_diagram->qgiManager().release(item);
			}
				/// Wires first, so no wire is left without its symbol
			void redo() override
			{
				for (QGraphicsObject *item : std::as_const(m_items))
					m_diagram->removeItem(item);
			}
			void undo() override
			{
				for (auto it = m_items.crbegin(); it != m_items.crend(); ++it)
					m_diagram->addItem(*it);
			}

		private:
			Diagram *m_diagram;
			QList<QGraphicsObject *> m_items;
	};

		/// What says which symbol it is in the plan, kept from the old one
	const QStringList kept_informations {
		QStringLiteral("label"), QStringLiteral("formula"), QStringLiteral("formula_id"),
		QStringLiteral("plant"), QStringLiteral("location"),
		QStringLiteral("function"), QStringLiteral("comment")};

	/**
		@return for each terminal of @a old_element, the terminal of
		@a new_element that takes its wires (wired terminals only): first the one on the same
		point, so the wires stay where they are; then, for those left,
		the one with the same name. A terminal with neither is missing.
	*/
	QHash<Terminal *, Terminal *> terminalMap(Element *old_element, Element *new_element)
	{
		QHash<Terminal *, Terminal *> map;
		QList<Terminal *> free_terminals = new_element->terminals();
		QList<Terminal *> left;
		for (Terminal *old_terminal : old_element->terminals()) {
				//Only a wired terminal needs a place: a free one would
				//take a point another wire may want by name
			if (old_terminal->conductors().isEmpty())
				continue;
			Terminal *found = nullptr;
			for (Terminal *t : std::as_const(free_terminals))
				if (QLineF(t->dockConductor(), old_terminal->dockConductor()).length() < 0.5)
					found = t;
			if (found) {
				map.insert(old_terminal, found);
				free_terminals.removeOne(found);
			} else {
				left << old_terminal;
			}
		}
		for (Terminal *old_terminal : std::as_const(left)) {
			if (old_terminal->name().isEmpty())
				continue;
			for (Terminal *t : std::as_const(free_terminals))
				if (t->name() == old_terminal->name()) {
					map.insert(old_terminal, t);
					free_terminals.removeOne(t);
					break;
				}
		}
		return map;
	}
}

/**
	@brief ReplaceSymbols::build
	@param diagram : the folio of @a elements
	@param elements : the symbols to replace
	@param location : the symbol that replaces them, in a collection; it
	is imported into the project as placing it would
	@return the undo step and what it could not keep, see Result
*/
ReplaceSymbols::Result ReplaceSymbols::build(Diagram *diagram, const QList<Element *> &elements,
											 const ElementsLocation &location)
{
	Result result;
	if (!diagram || elements.isEmpty() || !location.isElement())
		return result;

	DiagramContent removed;
	removed.m_elements = elements;
	QList<Conductor *> wires;
	for (Element *element : elements)
		for (Conductor *conductor : element->conductors())
			if (!wires.contains(conductor))
				wires << conductor;
	removed.m_conductors_to_update = wires;
	if (DeleteQGraphicsItemCommand::hasNonDeletableTerminal(removed)) {
		result.problem = QObject::tr("A terminal to replace is bridged or belongs to a "
									 "multi-level terminal.");
		return result;
	}

	ElementsLocation source = location;
	const ElementsLocation imported = diagram->project()->importElement(source);
	if (!imported.exist()) {
		result.problem = QObject::tr("The symbol could not be added to the project.");
		return result;
	}

	auto *undo = new QUndoCommand(QObject::tr("Replace %n symbol(s)", "", elements.size()));
	QList<QGraphicsObject *> gone;
	for (Conductor *wire : std::as_const(wires))
		gone << wire;
	for (Element *element : elements)
		gone << element;
	new RemoveItemsCommand(diagram, gone, undo);

	QHash<Terminal *, Terminal *> terminals;
	for (Element *old_element : elements) {
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(imported.projectCollectionPath()), nullptr, &state);
		if (state || !element) {
			delete element;
			delete undo;
			result.problem = QObject::tr("The symbol could not be loaded.");
			return result;
		}
		element->setPos(old_element->pos());
		element->setRotation(old_element->rotation());
		element->setHorizontalMirror(old_element->hasHorizontalMirror());
		element->setVerticalMirror(old_element->hasVerticalMirror());
		DiagramContext informations = element->elementInformations();
		const DiagramContext old_informations = old_element->elementInformations();
		for (const QString &key : kept_informations)
			if (old_informations.contains(key))
				informations.addValue(key, old_informations.value(key));
		element->setElementInformations(informations);
		new AddGraphicsObjectCommand(element, diagram, old_element->pos(), undo);
		terminals.insert(terminalMap(old_element, element));
	}

		//The wires again, on the new terminals; a wire between two
		//replaced symbols is one wire and comes back once
	for (Conductor *conductor : std::as_const(wires)) {
		const bool from_old_1 = elements.contains(conductor->terminal1->parentElement());
		const bool from_old_2 = elements.contains(conductor->terminal2->parentElement());
		Terminal *a = from_old_1 ? terminals.value(conductor->terminal1) : conductor->terminal1;
		Terminal *b = from_old_2 ? terminals.value(conductor->terminal2) : conductor->terminal2;
		if (!a || !b || a == b) {
			++result.lost_wires;
			continue;
		}
		auto *wire = new Conductor(a, b);
		wire->setProperties(conductor->properties());
		wire->setProfiles(conductor->profiles());
		new AddGraphicsObjectCommand(wire, diagram, QPointF(), undo);
	}

	result.command = undo;
	return result;
}
