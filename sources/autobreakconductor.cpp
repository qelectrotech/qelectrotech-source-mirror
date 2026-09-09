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
#include "autobreakconductor.h"

#include "diagram.h"
#include "qetproject.h"
#include "conductorautonumerotation.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetgraphicsitem/conductor.h"
#include "undocommand/addgraphicsobjectcommand.h"
#include "undocommand/deleteqgraphicsitemcommand.h"

#include <QPainterPath>
#include <QMap>
#include <limits>

namespace {

/**
	@brief distanceToSegment
	@param point : point to measure from
	@param segment : the finite segment (not the infinite line through it)
	@return the distance from @a point to the closest point actually on
	@a segment.  Unlike QET::orthogonalProjection(), which reports a hit
	for any point on the segment's infinite extension, this clamps the
	projection to the segment itself: a point collinear with a wire but
	past its actual drawn end is correctly reported as far away, not "on"
	the wire.
*/
qreal distanceToSegment(const QPointF &point, const QLineF &segment)
{
	const QPointF a = segment.p1();
	const QPointF b = segment.p2();
	const QPointF ab = b - a;
	const qreal len2 = QPointF::dotProduct(ab, ab);

	if (len2 <= 0.0)
		return QLineF(point, a).length();

	qreal t = QPointF::dotProduct(point - a, ab) / len2;
	t = qBound(0.0, t, 1.0);
	return QLineF(point, a + t * ab).length();
}

} // anonymous namespace

/**
	@brief autoBreakConductors
	For each terminal of @a element, check if its dock point lies on an
	existing conductor.  If so, break the conductor and reconnect through
	the element's terminal.  Broken conductors from the same circuit
	(sharing the same far-end endpoint) are broken together; independent
	crossing conductors are left untouched.
*/
QSet<Terminal *> autoBreakConductors(
	Diagram *diagram,
	Element *element,
	QUndoCommand *parent,
	QList<Conductor *> &conductors_handled,
	QSet<Terminal *> &used_terminals)
{
	QSet<Terminal *> broken_endpoints;

	if (!diagram->project()->autoBreakConductor())
		return broken_endpoints;

	foreach (Terminal *t, element->terminals())
	{
		if (used_terminals.contains(t))
			continue;

		QPointF t_dock = t->dockConductor();

		struct ConductorMatch {
			Conductor *conductor;
			Terminal *connect_to;
			Terminal *other;
		};
		QList<ConductorMatch> all_matches;

		foreach (Conductor *c, diagram->conductors())
		{
			if (conductors_handled.contains(c) ||
			    c->terminal1->parentElement() == element ||
			    c->terminal2->parentElement() == element)
				continue;

			QPointF local_dock = c->mapFromScene(t_dock);
			bool point_on_conductor = false;
			QPainterPath path = c->path();
			for (int i = 0; i < path.elementCount() - 1; ++i)
			{
				const QPainterPath::Element &e1 = path.elementAt(i);
				const QPainterPath::Element &e2 = path.elementAt(i + 1);
				QLineF segment(QPointF(e1.x, e1.y), QPointF(e2.x, e2.y));
				if (distanceToSegment(local_dock, segment) < 5.0)
				{
					point_on_conductor = true;
					break;
				}
			}

			if (!point_on_conductor)
				continue;

			Terminal *c1 = c->terminal1;
			Terminal *c2 = c->terminal2;
			QPointF c1_dock = c1->dockConductor();
			QPointF c2_dock = c2->dockConductor();

			Terminal *connect_to = nullptr;
			Terminal *other = nullptr;

			switch (t->orientation()) {
				case Qet::North:
					if (c1_dock.y() < t_dock.y()) { connect_to = c1; other = c2; }
					else if (c2_dock.y() < t_dock.y()) { connect_to = c2; other = c1; }
					break;
				case Qet::South:
					if (c1_dock.y() > t_dock.y()) { connect_to = c1; other = c2; }
					else if (c2_dock.y() > t_dock.y()) { connect_to = c2; other = c1; }
					break;
				case Qet::East:
					if (c1_dock.x() > t_dock.x()) { connect_to = c1; other = c2; }
					else if (c2_dock.x() > t_dock.x()) { connect_to = c2; other = c1; }
					break;
				case Qet::West:
					if (c1_dock.x() < t_dock.x()) { connect_to = c1; other = c2; }
					else if (c2_dock.x() < t_dock.x()) { connect_to = c2; other = c1; }
					break;
			}

			if (!connect_to) {
				qreal d1 = QLineF(t_dock, c1_dock).length();
				qreal d2 = QLineF(t_dock, c2_dock).length();
				if (d1 <= d2) { connect_to = c1; other = c2; }
				else { connect_to = c2; other = c1; }
			}

			all_matches.append({c, connect_to, other});
		}

		if (all_matches.isEmpty())
			continue;

		QMap<Terminal *, QList<ConductorMatch>> groups;
		for (const auto &m : all_matches) {
			groups[m.other].append(m);
		}
		Terminal *best_other = nullptr;
		int best_count = 0;
		for (auto it = groups.constBegin(); it != groups.constEnd(); ++it) {
			if (it.value().size() > best_count) {
				best_count = it.value().size();
				best_other = it.key();
			}
		}

		const QList<ConductorMatch> &matches = groups[best_other];

		used_terminals.insert(t);

		DiagramContent content;
		for (const auto &m : matches) {
			content.m_other_conductors.append(m.conductor);
			conductors_handled.append(m.conductor);
		}
		new DeleteQGraphicsItemCommand(diagram, content, parent);

		for (const auto &m : matches) {
			Conductor *new_c = new Conductor(m.connect_to, t);
			new AddGraphicsObjectCommand(new_c, diagram, QPointF(), parent);
			ConductorAutoNumerotation can(new_c, diagram, parent);
			can.numerate();
			if (diagram->freezeNewConductors() || diagram->project()->isFreezeNewConductors())
				new_c->setFreezeLabel(true);

			broken_endpoints.insert(m.connect_to);
		}

		QPointF other_dock = best_other->dockConductor();
		Terminal *other_terminal = nullptr;
		qreal best_dist = std::numeric_limits<qreal>::max();
		foreach (Terminal *ot, element->terminals())
		{
			if (used_terminals.contains(ot))
				continue;

			QPointF ot_dock = ot->dockConductor();
			bool orientation_ok = false;
			switch (ot->orientation()) {
				case Qet::North: orientation_ok = other_dock.y() < ot_dock.y(); break;
				case Qet::South: orientation_ok = other_dock.y() > ot_dock.y(); break;
				case Qet::East:  orientation_ok = other_dock.x() > ot_dock.x(); break;
				case Qet::West:  orientation_ok = other_dock.x() < ot_dock.x(); break;
			}
			if (!orientation_ok)
				continue;

			qreal dist = QLineF(ot_dock, other_dock).length();
			if (dist < best_dist) {
				best_dist = dist;
				other_terminal = ot;
			}
		}

		if (other_terminal) {
			Conductor *new_c2 = new Conductor(best_other, other_terminal);
			new AddGraphicsObjectCommand(new_c2, diagram, QPointF(), parent);
			ConductorAutoNumerotation can2(new_c2, diagram, parent);
			can2.numerate();
			if (diagram->freezeNewConductors() || diagram->project()->isFreezeNewConductors())
				new_c2->setFreezeLabel(true);

			broken_endpoints.insert(best_other);
			used_terminals.insert(other_terminal);
		}
	}

	return broken_endpoints;
}
