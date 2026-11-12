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
#ifndef DUPLICATELABELS_H
#define DUPLICATELABELS_H

#include "autoNum/assignvariables.h"

#include <QGraphicsObject>
#include <QPointer>
#include <QString>
#include <QVector>

class Conductor;
class ConductorProperties;
class Diagram;
class Element;
class QETProject;

namespace DuplicateLabels
{

	enum class Kind
	{
		Signal,
		Component
	};

	// When symbols with one label in one place are one component.
	enum class ComponentRule
	{
		// Only when QET links them (coil and contacts) or, for terminal
		// blocks, when they are on one potential: every other drawing is a
		// device of its own, and a second one with the label clashes.
		LinkedOnly,
		// Also when nothing links them: one designation (=plant+location
		// -label, IEC 81346) names one device however often it is drawn.
		// They clash only when they disagree -- two different part
		// numbers.
		SameDesignation
	};

	// Where a wire number must be unique.
	enum class SignalScope
	{
		// In the whole project: two potentials with one number clash.
		Project,
		// In each installation (=plant of the wire's folio), as
		// plant-structured projects number: one number may name a
		// potential in =FACTORY and another in =SILO.
		Plant
	};

	struct Occurrence
	{
		QPointer<QGraphicsObject> item;
		// The label as the sheet shows it; with case-insensitive matching the
		// occurrences of one group may differ in letter case.
		QString label;
		// Wires on one potential, or symbols representing one linked device.
		int entity = 0;
		// A component's place(); for a signal, its folio's plant ("=SILO")
		// with SignalScope::Plant, else empty.
		QString place;
		// A component's part number (manufacturer_reference); empty when it
		// has none, which agrees with any.
		QString part;
	};

	struct Group
	{
		Kind				kind;
		QString				label;
		QString				place;
		QVector<Occurrence> occurrences;
		int					entity_count = 0;
		// ComponentRule::SameDesignation: the part numbers which disagree,
		// sorted; empty for every other group.
		QStringList			parts;
	};

	// Labels ignoring surrounding whitespace. Empty labels and QET's
	// unassigned "_" placeholder are not duplicate identifiers.
	QString identifier(const QString& label);
	// What two labels must share to be the same identifier: identifier(),
	// case-folded unless matching is case-sensitive.
	QString key(const QString& label, Qt::CaseSensitivity sensitivity);

	// Labels as the sheet shows them: formulas (%F, %f, %id, ...) are
	// resolved against the folio the item is on now, so "%F21" on folio 1
	// is "121". Frozen labels keep the text they were frozen with.
	QString displayedLabel(const Conductor* wire);
	QString displayedLabel(Element* element);
	// The variables a folio gives its formulas, to label many of its wires
	// without collecting them again for each one, and the wire's properties
	// as already read (Conductor::properties() goes through the project
	// database each time).
	autonum::FormulaContext folioVariables(Diagram* folio);
	QString					displayedLabel(
						const Conductor*			   wire,
						const ConductorProperties&	   properties,
						const autonum::FormulaContext& folio);

	// Where a symbol is, as an IEC 81346 prefix such as "=P1+UV1": the
	// element's own plant and location, else its folio's -- what the label
	// formulas %M and %LM show. One label names one device per place.
	QString place(Element* element);

	Diagram* diagramOf(const QGraphicsObject* item);

	// Reads the whole project, including lazy folios. Does not number or edit
	// it. Each group has at least two unrelated entities; every occurrence is
	// retained so the user can navigate to each wire segment or symbol
	// involved. Signals follow wires, terminal blocks, strip bridges and
	// linked folio reports; terminal-type symbols (terminal blocks, splices,
	// earth symbols) drawn on one potential are one component. Components
	// only clash within one place; a linked device is at its master's.
	// With ComponentRule::SameDesignation, symbols of one label in one place
	// which nothing links are one component unless their part numbers
	// differ; terminal blocks still follow their potentials. With
	// SignalScope::Plant, signals only clash within one plant.
	QVector<Group> find(
		QETProject*			project,
		Qt::CaseSensitivity sensitivity = Qt::CaseSensitive,
		ComponentRule		rule		= ComponentRule::LinkedOnly,
		SignalScope			scope		= SignalScope::Project);

} // namespace DuplicateLabels

#endif
