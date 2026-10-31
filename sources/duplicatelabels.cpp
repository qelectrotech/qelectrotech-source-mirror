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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "duplicatelabels.h"

#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstripbridge.h"
#include "autoNum/assignvariables.h"
#include "diagram.h"
#include "qet.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetgraphicsitem/terminalelement.h"
#include "qetproject.h"

#include <QMap>
#include <QSet>
#include <algorithm>

namespace DuplicateLabels
{

	QString identifier(const QString& label)
	{
		const QString text = label.trimmed();
		return text == QLatin1String("_") ? QString() : text;
	}

	QString key(const QString& label, Qt::CaseSensitivity sensitivity)
	{
		const QString text = identifier(label);
		return sensitivity == Qt::CaseSensitive ? text : text.toCaseFolded();
	}

	QString displayedLabel(const Conductor* wire)
	{
		if (! wire->diagram()) return wire->properties().text;
		return displayedLabel(
			wire,
			wire->properties(),
			folioVariables(wire->diagram()));
	}

	autonum::FormulaContext folioVariables(Diagram* folio)
	{
		// As AssignVariables::formulaToLabel() reads them from a diagram.
		autonum::FormulaContext variables;
		const auto&				border = folio->border_and_titleblock;
		variables.folio				   = border.folio();
		variables.folio_title		   = border.title();
		variables.folio_index		   = folio->folioIndex();
		variables.folio_total		   = border.folioTotal();
		variables.plant				   = border.plant();
		variables.locmach			   = border.locmach();
		variables.title_block_fields   = border.additionalFields();
		variables.project_properties   = folio->project()->projectProperties();
		return variables;
	}

	QString displayedLabel(
		const Conductor*			   wire,
		const ConductorProperties&	   properties,
		const autonum::FormulaContext& folio)
	{
		// Mirrors Conductor::refreshText(), without storing the result: the
		// stored text is only refreshed by some folio changes.
		if (wire->isFreezeLabel() || properties.m_formula.isEmpty())
			return properties.text;
		autonum::FormulaContext variables	= folio;
		variables.has_conductor				= true;
		variables.wire_function				= properties.m_function;
		variables.wire_tension_protocol		= properties.m_tension_protocol;
		variables.wire_color				= properties.m_wire_color;
		variables.wire_section				= properties.m_wire_section;
		autonum::sequentialNumbers sequence = wire->sequenceNum();
		return autonum::AssignVariables::formulaToLabel(
			properties.m_formula,
			sequence,
			variables);
	}

	QString displayedLabel(Element* element)
	{
		// The stored "label" keeps the folio it was computed on; the texts
		// showing it follow actualLabel() unless the label is frozen.
		if (element->isFreezeLabel())
			return element->elementInformations()
				.value(QStringLiteral("label"))
				.toString();
		return element->actualLabel();
	}

	QString place(Element* element)
	{
		const auto info	   = element->elementInformations();
		Diagram*   diagram = element->diagram();
		QString	   plant =
			info.value(QStringLiteral("plant")).toString().trimmed();
		QString location =
			info.value(QStringLiteral("location")).toString().trimmed();
		if (plant.isEmpty() && diagram)
			plant = diagram->border_and_titleblock.plant().trimmed();
		if (location.isEmpty() && diagram)
			location = diagram->border_and_titleblock.locmach().trimmed();
		// QET stores bare names; the prefixes keep a plant apart from a
		// location of the same name.
		if (! plant.isEmpty() && ! plant.startsWith(QLatin1Char('=')))
			plant.prepend(QLatin1Char('='));
		if (! location.isEmpty() && ! location.startsWith(QLatin1Char('+')))
			location.prepend(QLatin1Char('+'));
		return plant + location;
	}

	Diagram* diagramOf(const QGraphicsObject* item)
	{
		return item ? qobject_cast<Diagram*>(item->scene()) : nullptr;
	}

	namespace
	{

		using Labels = QMap<QString, QVector<Occurrence>>;

		// What two components must share to clash: their place and key().
		QString componentKey(
			const QString&		place,
			const QString&		label,
			Qt::CaseSensitivity sensitivity)
		{
			return (sensitivity == Qt::CaseSensitive ? place :
													   place.toCaseFolded())
				   + QChar(0x1F) + key(label, sensitivity);
		}

		// Terminal blocks a strip bridge joins: one potential, although no
		// wire is drawn between them.
		QList<Terminal*> bridgedTerminals(const Terminal* terminal)
		{
			QList<Terminal*> joined;
			auto*			 block =
				qobject_cast<TerminalElement*>(terminal->parentElement());
			const auto real =
				block ? block->realTerminal() : QSharedPointer<RealTerminal>();
			const auto bridge =
				real ? real->bridge() : QSharedPointer<TerminalStripBridge>();
			if (! bridge) return joined;
			for (const auto& other : bridge->realTerminals())
			{
				Element* element = other ? other->element() : nullptr;
				if (element && element != block)
					joined.append(element->terminals());
			}
			return joined;
		}

		// Floods one potential from wire, numbering its terminals id.
		QSet<Conductor*> connectedConductors(
			Conductor*			   wire,
			const QSet<Diagram*>&  diagrams,
			int					   id,
			QHash<Terminal*, int>& potential_of)
		{
			QSet<Conductor*> connected;
			QSet<Terminal*>	 visited;
			QList<Terminal*> pending {wire->terminal1, wire->terminal2};
			while (! pending.isEmpty())
			{
				Terminal* terminal = pending.takeLast();
				if (visited.contains(terminal)
					|| ! diagrams.contains(terminal->diagram()))
					continue;
				visited.insert(terminal);
				potential_of.insert(terminal, id);
				// Reuse QET's terminal/report and function-separation rules,
				// but never cross a folio parked in the undo history.
				pending.append(relatedPotentialTerminal(terminal, true));
				pending.append(bridgedTerminals(terminal));
				for (Conductor* part : terminal->conductors())
				{
					if (connected.contains(part)
						|| ! diagrams.contains(part->diagram()))
						continue;
					connected.insert(part);
					pending.append(part->terminal1);
					pending.append(part->terminal2);
				}
			}
			return connected;
		}

		void appendGroups(
			QVector<Group>& result,
			Kind			kind,
			const Labels&	labels,
			QETProject*		project)
		{
			for (auto it = labels.cbegin(); it != labels.cend(); ++it)
			{
				QSet<int>	entities;
				QStringList spellings, places;
				for (const Occurrence& occurrence : it.value())
				{
					entities.insert(occurrence.entity);
					if (! spellings.contains(occurrence.label))
						spellings.append(occurrence.label);
					if (! places.contains(occurrence.place))
						places.append(occurrence.place);
				}
				if (entities.size() < 2) continue;
				std::sort(spellings.begin(), spellings.end());
				std::sort(places.begin(), places.end());
				Group group {
					kind,
					spellings.join(QStringLiteral(", ")),
					places.join(QStringLiteral(", ")),
					it.value(),
					int(entities.size())};
				std::sort(
					group.occurrences.begin(),
					group.occurrences.end(),
					[project](const Occurrence& a, const Occurrence& b)
					{
						const int af = project->folioIndex(diagramOf(a.item));
						const int bf = project->folioIndex(diagramOf(b.item));
						if (af != bf) return af < bf;
						const QPointF ap = a.item->sceneBoundingRect().center();
						const QPointF bp = b.item->sceneBoundingRect().center();
						if (ap.y() != bp.y()) return ap.y() < bp.y();
						if (ap.x() != bp.x()) return ap.x() < bp.x();
						return a.entity < b.entity;
					});
				result.append(group);
			}
		}

	} // namespace

	QVector<Group> find(QETProject* project, Qt::CaseSensitivity sensitivity)
	{
		QVector<Group> result;
		if (! project) return result;

		// Build every folio first: both report links and device relationships
		// may point at symbols on a folio which has never been displayed.
		const auto			  diagrams = project->diagrams();
		const QSet<Diagram*>  active_diagrams(diagrams.begin(), diagrams.end());
		Labels				  signal_labels, components;
		QSet<Conductor*>	  visited_wires;
		QSet<Element*>		  visited_elements;
		QHash<Terminal*, int> potential_of;
		int					  potential = 0, device = 0;
		struct TerminalSymbol
		{
			Occurrence occurrence;
			QList<int> potentials;
		};
		QHash<QString, QVector<TerminalSymbol>>	 terminal_symbols;
		QHash<Diagram*, autonum::FormulaContext> folio_variables;
		for (Diagram* diagram : diagrams)
		{
			for (Conductor* wire : diagram->conductors())
			{
				if (visited_wires.contains(wire)) continue;
				++potential;
				// QET follows actual terminals, function separation and linked
				// folio-report arrows; the name alone never connects two
				// signals.
				const auto connected = connectedConductors(
					wire,
					active_diagrams,
					potential,
					potential_of);
				visited_wires.unite(connected);
				for (Conductor* part : connected)
				{
					const ConductorProperties properties = part->properties();
					if (properties.type != ConductorProperties::Multi) continue;
					Diagram* folio = part->diagram();
					if (! folio_variables.contains(folio))
						folio_variables.insert(folio, folioVariables(folio));
					const QString label = identifier(displayedLabel(
						part,
						properties,
						folio_variables.value(folio)));
					if (! label.isEmpty())
						signal_labels[key(label, sensitivity)].append(
							{part, label, potential});
				}
			}
			for (Element* element : diagram->elements())
			{
				if (visited_elements.contains(element)) continue;
				++device;
				// A linked device is where its master is, whichever folio
				// shows its contacts.
				QString				device_place = place(element);
				QVector<Occurrence> parts;
				QList<Element*>		pending {element};
				while (! pending.isEmpty())
				{
					Element* part = pending.takeLast();
					if (visited_elements.contains(part)
						|| ! active_diagrams.contains(part->diagram()))
						continue;
					visited_elements.insert(part);
					pending.append(part->linkedElements());
					if (part->linkType() & Element::Master)
						device_place = place(part);
					// Traverse even excluded masters: their contacts still
					// represent one device. Only the excluded symbol's own
					// occurrence is omitted.
					if (part->linkType()
						& (Element::AllReport | Element::ConductorDefinition))
						continue;
					const auto info = part->elementInformations();
					if (QET::infoFlagIsTrue(
							info.value(QStringLiteral("exclude_from_bom"))
								.toString()))
						continue;
					const QString label = identifier(displayedLabel(part));
					if (label.isEmpty()) continue;
					if (part->linkType() & Element::Terminale)
					{
						QList<int> potentials;
						for (Terminal* terminal : part->terminals())
							if (potential_of.contains(terminal))
								potentials.append(potential_of.value(terminal));
						if (! potentials.isEmpty())
						{
							const QString where = place(part);
							terminal_symbols
								[componentKey(where, label, sensitivity)]
									.append(
										{{part, label, 0, where}, potentials});
							continue;
						}
					}
					parts.append({part, label, device});
				}
				for (Occurrence& occurrence : parts)
				{
					occurrence.place = device_place;
					components[componentKey(
								   device_place,
								   occurrence.label,
								   sensitivity)]
						.append(occurrence);
				}
			}
		}
		// A terminal block, splice or earth symbol drawn several times names
		// a connection point: its drawings sharing a potential are one
		// component. A terminal with function separation spans the potentials
		// of its sides; only drawings with the same label are joined.
		for (auto it = terminal_symbols.cbegin(); it != terminal_symbols.cend();
			 ++it)
		{
			QHash<int, int> parent;
			const auto		root = [&parent](int id)
			{
				while (parent.value(id, id) != id) id = parent.value(id);
				return id;
			};
			for (const TerminalSymbol& symbol : it.value())
				for (int id : symbol.potentials)
					parent.insert(root(id), root(symbol.potentials.first()));
			for (TerminalSymbol symbol : it.value())
			{
				// Negative: never equal to a device number.
				symbol.occurrence.entity = -1 - root(symbol.potentials.first());
				components[it.key()].append(symbol.occurrence);
			}
		}
		appendGroups(result, Kind::Signal, signal_labels, project);
		appendGroups(result, Kind::Component, components, project);
		return result;
	}

} // namespace DuplicateLabels
