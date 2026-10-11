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
#include "cablemanager.h"

#include "cable.h"
#include "../autoNum/assignvariables.h"
#include "../autoNum/numerotationcontextcommands.h"
#include "../diagram.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetproject.h"

#include <QHash>
#include <QPainterPath>

#include <algorithm>
#include <limits>

namespace {

/**
	@brief Where two segments cross, if they do at all.

	Done by hand rather than with QLineF::intersects() because the
	collinear case has to be a miss: a cable drawn exactly on top of a
	conductor follows it, it does not cross it, and counting it would
	claim a core the cable never reaches.
	@param a1 start of the first segment
	@param a2 end of the first segment
	@param b1 start of the second segment
	@param b2 end of the second segment
	@param result receives the crossing point when there is one
	@return true when the two segments cross within their own length
*/
bool segmentIntersection(const QPointF &a1, const QPointF &a2,
						 const QPointF &b1, const QPointF &b2,
						 QPointF *result)
{
	const QPointF r = a2 - a1;
	const QPointF s = b2 - b1;
	const QPointF u = b1 - a1;

	const qreal denominator = r.x() * s.y() - r.y() * s.x();
	if (qFuzzyIsNull(denominator)) {
		return false;
	}

	const qreal t = (u.x() * s.y() - u.y() * s.x()) / denominator;
	const qreal v = (u.x() * r.y() - u.y() * r.x()) / denominator;
	if (t < 0.0 || t > 1.0 || v < 0.0 || v > 1.0) {
		return false;
	}

	if (result) {
		*result = a1 + t * r;
	}
	return true;
}

/**
	@brief How far a point stands from a segment, in scene units
*/
qreal distanceToSegment(const QPointF &point, const QPointF &a, const QPointF &b)
{
	const QPointF ab = b - a;
	const qreal length_squared = ab.x() * ab.x() + ab.y() * ab.y();
	if (qFuzzyIsNull(length_squared)) {
		return QLineF(point, a).length();
	}

	const QPointF ap = point - a;
	qreal t = (ap.x() * ab.x() + ap.y() * ab.y()) / length_squared;
	t = qBound(0.0, t, 1.0);
	return QLineF(point, a + t * ab).length();
}

/**
	@brief Whether some cable of the project already stands under this
	number.

	A number which is taken must not be handed out twice. That happens
	while a whole run of cables is numbered one after the other -- none
	of them has been added to the project by anything but its own
	creation yet -- and it happens when a rule has simply been given a
	value which some cable carries already.
	@param project
	@param designation the number to look for
	@return true when a cable of that project stands under it
*/
bool designationIsTaken(QETProject *project, const QString &designation)
{
	if (!project || designation.isEmpty()) return false;

	for (Cable *cable : project->cables())
	{
		if (cable && cable->designation() == designation) {
			return true;
		}
	}
	return false;
}

} // namespace

/**
	@brief CableManager::crossedConductors
	Tell which conductors a line about to become a cable runs across, in
	the order they are met along the line. That order is what gives each
	core its place, and therefore its colour, so it has to be the order
	of the drawing and not the order the conductors happen to be stored
	in.
	@param diagram the folio the line is drawn on
	@param line the line, in scene coordinates
	@return the crossings, ordered from the start of the line
*/
QList<CableCrossing> CableManager::crossedConductors(Diagram *diagram, const QLineF &line)
{
	QList<CableCrossing> crossings;
	if (!diagram) {
		return crossings;
	}

	const QPointF start = line.p1();
	if (qFuzzyCompare(start, line.p2())) {
		return crossings;
	}

	const QPointF direction = line.p2() - start;
	for (Conductor *conductor : diagram->conductors())
	{
		QPointF where;
		if (!crossingOf(diagram, line, conductor, &where)) continue;

		CableCrossing crossing;
		crossing.conductor = conductor;
		crossing.position = where;
		crossings.append(crossing);
	}

	std::sort(crossings.begin(), crossings.end(),
			  [&start, &direction](const CableCrossing &a_, const CableCrossing &b_) {
				  const QPointF da = a_.position - start;
				  const QPointF db = b_.position - start;
				  const qreal ta = da.x() * direction.x() + da.y() * direction.y();
				  const qreal tb = db.x() * direction.x() + db.y() * direction.y();
				  return ta < tb;
			  });

	return crossings;
}

/**
	@brief CableManager::crossingOf
	Where one line runs across one conductor, if it does at all.

	Done by hand rather than with QLineF::intersects() because the
	collinear case has to be a miss: a cable drawn exactly on top of a
	conductor follows it, it does not cross it, and counting it would
	claim a core the cable never reaches.
	@param diagram the folio the line is drawn on
	@param line the line, in scene coordinates
	@param conductor the wire being asked about
	@param result receives the crossing point, in scene coordinates
	@return true when the line really runs across that wire
*/
bool CableManager::crossingOf(Diagram *diagram,
							  const QLineF &line,
							  Conductor *conductor,
							  QPointF *result)
{
	if (!conductor || !diagram) return false;

	const QPointF start = line.p1();
	const QPointF end = line.p2();
	if (qFuzzyCompare(start, end)) return false;

		//Work in the conductor's own coordinates: its path is stored
		//there, and a conductor is never where it was when it was
		//created once the whole drawing has been moved about.
	const QPointF a = conductor->mapFromScene(start);
	const QPointF b = conductor->mapFromScene(end);

	const QPainterPath path = conductor->path();
	for (int i = 0; i + 1 < path.elementCount(); ++i)
	{
		const QPainterPath::Element &e1 = path.elementAt(i);
		const QPainterPath::Element &e2 = path.elementAt(i + 1);
			//A MoveTo ends the segment before it: the jump to the new
			//subpath is not drawn, so nothing can cross it.
		if (e2.type == QPainterPath::MoveToElement) {
			continue;
		}
		QPointF where;
		if (segmentIntersection(a, b,
								QPointF(e1.x, e1.y),
								QPointF(e2.x, e2.y),
								&where))
		{
			if (result) {
				*result = conductor->mapToScene(where);
			}
			return true;
		}
	}
	return false;
}

/**
	@brief CableManager::distanceTo
	How far a point of the sheet stands from a wire, in scene units.
	@param conductor
	@param scene_pos
	@return that distance, which is where the wire passes when the point
	is beside it rather than in front of one of its ends
*/
qreal CableManager::distanceTo(Conductor *conductor, const QPointF &scene_pos)
{
	if (!conductor) {
		return std::numeric_limits<qreal>::max();
	}

	qreal best = std::numeric_limits<qreal>::max();
	const QPainterPath path = conductor->path();
	for (int i = 0; i + 1 < path.elementCount(); ++i)
	{
		const QPainterPath::Element &e1 = path.elementAt(i);
		const QPainterPath::Element &e2 = path.elementAt(i + 1);
		if (e2.type == QPainterPath::MoveToElement) continue;

		const qreal d = distanceToSegment(
			scene_pos,
			conductor->mapToScene(QPointF(e1.x, e1.y)),
			conductor->mapToScene(QPointF(e2.x, e2.y)));
		if (d < best) {
			best = d;
		}
	}
	return best;
}

namespace {

/**
	@brief The end of @a conductor which does not touch @a element.

	Which end that is tells where the wire goes on its own sheet: a wire
	ending on a report arrow points away from it, and that is the end
	which says something about where it comes from or where it runs to.
	@param conductor the wire to look at
	@param element the report arrow it ends on (may be nullptr)
	@return the scene position of the other end, (0, 0) when there is none
*/
QPointF farEndPosition(const Conductor *conductor, Element *element)
{
	const Terminal *far = nullptr;
	if (conductor->terminal1
		&& conductor->terminal1->parentElement() != element) {
		far = conductor->terminal1;
	} else {
		far = conductor->terminal2 ? conductor->terminal2 : conductor->terminal1;
	}
	return far ? far->scenePos() : QPointF();
}

/**
	@brief Whether wire @a a runs before wire @a b on its own sheet.

	Both are looked at from @a element: what tells them apart is where
	their other end goes. Putting the wires which come in to a report
	and the wires which go out of its partner in this order is what
	pairs a continuation up with the core it belongs to when several
	cores run through one and the same report.
	@param a @param b the two wires to compare
	@param element the report arrow both end on (may be nullptr)
	@return true when @a a comes first
*/
bool farBefore(const Conductor *a, const Conductor *b, Element *element)
{
	const QPointF pa = farEndPosition(a, element);
	const QPointF pb = farEndPosition(b, element);
	if (pa.y() != pb.y()) return pa.y() < pb.y();
	if (pa.x() != pb.x()) return pa.x() < pb.x();
	return a->uuid() < b->uuid();
}

} // namespace

/**
	@brief CableManager::refreshLabels
	Rewrite the cable field of every conductor of the project from the
	cables themselves.

	The rule is one-directional and therefore cannot drift: a conductor
	which some cable claims gets the label that cable generates, a
	conductor which no cable claims any more -- because the cable was
	delete, the core was dragged away or the type lost that core -- has
	the generated label taken away. A conductor which never belonged to
	a cable keeps whatever it had, since only one with a reference is
	touched.

	A core which ends on a folio report goes on over there: the wire
	coming out of the linked arrow shows the same piece of copper, so
	it belongs to the same core and carries the same cable field. What
	the links say is worked out from scratch on every pass, and a wire
	which some cable already claims is never taken away from it, so the
	reference appears wherever a report carries a core and disappears
	again by itself as soon as the cable, the core or the link itself
	is gone. Chains of reports are followed as far as they go, so a
	cable may run over any number of sheets.
	@param project
*/
void CableManager::refreshLabels(QETProject *project)
{
	if (!project) return;

	struct Binding {
		QUuid cable;
		int slot = -1;
		QString label;
	};

	QHash<QUuid, Binding> bindings;
	for (Cable *cable : project->cables())
	{
		if (!cable) continue;
		for (const CableCore &core : cable->usedCores())
		{
			if (core.conductor.isNull()) continue;
			Binding binding;
			binding.cable = cable->uuid();
			binding.slot = core.core;
			binding.label = cable->labelOfCore(core.core);
			bindings.insert(core.conductor, binding);
		}
	}

		//A core which ends on a folio report continues on the wire which
		//leaves the linked arrow on the other sheet. That wire shows the
		//same piece of copper, so it gets the same reference as the core
		//it continues. The walk repeats while it finds new wires, so a
		//chain of reports is followed across any number of sheets, and
		//it only ever fills a wire no cable has claimed yet: two cores
		//can never fight over the same wire, and a reference worked out
		//here is taken away again by the same pass as soon as the cable,
		//the core or the link does.
	QHash<QUuid, Conductor *> conductors_by_uuid;
	QHash<Element *, QList<Conductor *>> endings_on_report;
	for (Diagram *diagram : project->diagrams())
	{
		if (!diagram) continue;
		for (Conductor *conductor : diagram->conductors())
		{
			if (conductor) conductors_by_uuid.insert(conductor->uuid(), conductor);
		}
	}

		//Where a wire goes on its own sheet (farBefore, above): putting the
		//wires which come in and the wires which go out in the same order
		//is what pairs a continuation up with its core when several cores
		//run through one and the same report.

	bool grew = true;
	while (grew)
	{
		grew = false;
		endings_on_report.clear();
		for (const QUuid &source_uuid : bindings.keys())
		{
			Conductor *source = conductors_by_uuid.value(source_uuid);
			if (!source) continue;

			Terminal *ends[2] = {source->terminal1, source->terminal2};
			for (Terminal *end : ends)
			{
				if (!end) continue;
				Element *element = end->parentElement();
					//An arrow taken off the sheet carries nothing over
					//any more, whatever still links it
				if (!element || !element->scene()) continue;
				if (!(element->linkType() & Element::AllReport)) continue;

				QList<Conductor *> &in = endings_on_report[element];
				if (!in.contains(source)) in.append(source);
			}
		}

		for (auto it = endings_on_report.constBegin();
			 it != endings_on_report.constEnd(); ++it)
		{
			Element *element = it.key();
			QList<Conductor *> in = it.value();
			std::sort(in.begin(), in.end(),
					  [&](const Conductor *a, const Conductor *b) {
				return farBefore(a, b, element);
			});

			for (Element *partner : element->linkedElements())
			{
				if (!partner || partner == element || !partner->scene()) continue;

				QList<Conductor *> out = partner->conductors();
				std::sort(out.begin(), out.end(),
						  [&](const Conductor *a, const Conductor *b) {
					return farBefore(a, b, partner);
				});

					//One core coming in may branch out into several wires
					//over there -- they all show the same piece of copper.
					//Several cores coming in take one wire each, in the
					//order they run on their own sheet.
				const int count = (in.size() == 1)
						? out.size()
						: qMin(in.size(), out.size());
				for (int i = 0; i < count; ++i)
				{
					Conductor *continued = out.at(i);
					if (!continued || bindings.contains(continued->uuid())) continue;
					const int from = qMin(i, in.size() - 1);
					bindings.insert(continued->uuid(),
									bindings.value(in.at(from)->uuid()));
					grew = true;
				}
			}
		}
	}

	for (Diagram *diagram : project->diagrams())
	{
		if (!diagram) continue;
		for (Conductor *conductor : diagram->conductors())
		{
			if (!conductor) continue;

			const auto it = bindings.constFind(conductor->uuid());
			if (it != bindings.constEnd())
			{
				const ConductorProperties &properties = conductor->properties();
				conductor->setCableReference(it->label, it->cable, it->slot);
			}
			else if (!conductor->properties().m_cable_uuid.isNull())
			{
				conductor->setCableReference(QString(), QUuid(), -1);
			}
		}
	}
}

/**
	@brief CableManager::claimConductor
	Make a conductor a core of @a new_owner, taking it away from whichever
	cable held it before. A conductor is a single piece of wire: two
	cables cannot both claim it, and the one that loses it must not go
	on drawing a label for it.
	@param project
	@param conductor
	@param new_owner the cable which is about to use the conductor
*/
void CableManager::claimConductor(QETProject *project, Conductor *conductor, Cable *new_owner)
{
	if (!project || !conductor) return;

	const QUuid owner_uuid = conductor->properties().m_cable_uuid;
	if (owner_uuid.isNull() || (new_owner && owner_uuid == new_owner->uuid())) {
		return;
	}

	if (Cable *previous = cableByUuid(project, owner_uuid)) {
		previous->removeCore(conductor->properties().m_cable_slot);
	}
}

/**
	@brief CableManager::cableByUuid
	@param project
	@param uuid
	@return the cable with that identity, or nullptr
*/
Cable *CableManager::cableByUuid(QETProject *project, const QUuid &uuid)
{
	if (!project || uuid.isNull()) return nullptr;
	for (Cable *cable : project->cables()) {
		if (cable && cable->uuid() == uuid) return cable;
	}
	return nullptr;
}

/**
	@brief CableManager::conductorByUuid
	@param project
	@param uuid
	@return the conductor with that identity, or nullptr. Needed because
	a cable remembers its cores by identity: only the drawing knows where
	they ended up.
*/
Conductor *CableManager::conductorByUuid(QETProject *project, const QUuid &uuid)
{
	if (!project || uuid.isNull()) return nullptr;
	for (Diagram *diagram : project->diagrams()) {
		if (!diagram) continue;
		for (Conductor *conductor : diagram->conductors()) {
			if (conductor && conductor->uuid() == uuid) return conductor;
		}
	}
	return nullptr;
}

/**
	@brief CableManager::nextDesignation
	Work out the number of the next cable drawn on a folio.

	When the project has a numbering rule for cables, that rule decides:
	whatever it holds at this moment is the number, read by the very
	same machinery the element numbering reads its own. When it has
	none, the plain letter W is handed out: a project which numbers its
	cables by no rule gets no numbers invented for it, and W -- the one
	part of the old numbers which was never a number -- is all such a
	cable is called until a rule is defined.

	A rule whose counter belongs to one folio is read from the beginning
	of the folio the cable is drawn on, so that the number this cable
	gets is the first free one of its own folio rather than whatever the
	project's rule happens to hold after numbering other folios.

	Nothing here moves the project's own rule on: that belongs to the
	moment the cable really comes into being.
	@param project
	@param diagram the folio the cable is being drawn on
	@return the number
*/
QString CableManager::nextDesignation(QETProject *project, Diagram *diagram)
{
	NumerotationContext context;
	if (project) {
		context = project->cableAutoNum(project->cableCurrentAutoNum());
	}
		//The folio-bound parts are put back to the start of that folio;
		//the stepping of the overload below then walks past every
		//number of this folio which is already taken
	context = autonum::resetFolioCounters(context);
	return nextDesignation(project, diagram, context);
}

/**
	@brief CableManager::nextDesignation
	Work the number of a cable out of a rule which the caller keeps in
	@p context -- the project's own rule as it stands, or a copy of it
	which the caller is numbering a whole run of cables with.

	Whatever the rule holds is the number, unless some cable already
	stands under it: then @p context is stepped on and the rule is read
	once more, so that a paste which brings in several cables at once
	numbers each of them differently although the project's own rule
	does not move until those cables really exist. A rule which cannot
	step -- one with nothing in it which counts -- hands out what it
	says and says no more, for what it says is all it knows.

	A project which numbers its cables by no rule is not read at all
	and is handed the plain letter W instead: no numbers are invented
	where none have been defined.
	@param project
	@param diagram the folio the cable is being drawn on, which every
	folio-bound part of a rule is read from
	@param context the rule to read; stepped on when what it says is
	already taken
	@return the number
*/
QString CableManager::nextDesignation(QETProject *project,
									  Diagram *diagram,
									  NumerotationContext &context)
{
	if (!project || !diagram) return QString();

	const QString key = project->cableCurrentAutoNum();
		//Whether there was a rule to read at all, taken down before the
		//loop: stepping the rule on empties it, and that must not look
		//like a project which never had one.
	const bool ruled = !key.isEmpty() && !context.isEmpty();
	QString previous;

	for (int attempt = 0; ruled && attempt < 1000 && !context.isEmpty(); ++attempt)
	{
			//The rule is read on a copy: reading it fills in what its
			//folio-bound parts are worth here and now, and that must
			//not creep into the rule itself.
		NumerotationContext evaluated = context;
		const QString formula = autonum::numerotationContextToFormula(evaluated);
		autonum::sequentialNumbers seq;
		autonum::setSequential(formula, seq, evaluated, diagram, key);
		const QString candidate =
			autonum::AssignVariables::formulaToLabel(formula, seq, diagram);

		if (candidate.isEmpty()) break;
		if (!designationIsTaken(project, candidate)) return candidate;

			//Taken already: step the rule on and read it once more.
		if (candidate == previous) return candidate;
		previous = candidate;

		NumerotationContextCommands ncc(context, diagram);
		const NumerotationContext following = ncc.next();
		if (following.size() != context.size()) break;
		context = following;
	}

		//No rule at all: the plain letter W, and nothing more. The old
		//numbers read folio + W + index, and W is the only part of them
		//which was never a number -- so it is what a project which has
		//defined no numbering rule calls its cables, until it defines
		//one.
	if (!ruled) {
		return QStringLiteral("W");
	}

		//A rule which says nothing usable here: no number at all. The
		//number is what the rule says, and a rule which says nothing
		//leaves the field to the user rather than having it filled in
		//with something it never held.
	return QString();
}
