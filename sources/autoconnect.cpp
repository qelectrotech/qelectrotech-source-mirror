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
#include "autoconnect.h"

#include "conductorautonumerotation.h"
#include "diagram.h"
#include "diagramcontent.h"
#include "qetproject.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "undocommand/addgraphicsobjectcommand.h"
#include "undocommand/deleteqgraphicsitemcommand.h"

#include <algorithm>

namespace {

	/// How far off the line a terminal may be and still count as in line
const qreal kTolerance = 0.5;

struct State
{
	Diagram *diagram = nullptr;
		/// wires that go with this change (removed here or by the caller)
	QSet<Conductor *> removed;
		/// terminals that got a wire in this change
	QSet<Terminal *> used;
		/// symbols the caller removes
	QSet<Element *> leaving;
};

QPointF direction(Qet::Orientation o)
{
	switch (o) {
		case Qet::North: return QPointF(0, -1);
		case Qet::East:  return QPointF(1, 0);
		case Qet::South: return QPointF(0, 1);
		case Qet::West:  return QPointF(-1, 0);
	}
	return QPointF();
}

/**
	@return how far @a point lies ahead of @a from along its orientation,
	or -1 when it is not on that line or lies behind
*/
qreal ahead(const Terminal *from, const QPointF &point)
{
	const QPointF d = direction(from->orientation());
	const QPointF v = point - from->dockConductor();
	const qreal along = v.x() * d.x() + v.y() * d.y();
	const qreal off = qAbs(v.x() * d.y() - v.y() * d.x());
	if (off > kTolerance || along < -kTolerance)
		return -1;
	return qMax(along, 0.0);
}

bool isFree(const State &s, const Terminal *t)
{
	if (s.used.contains(const_cast<Terminal *>(t)))
		return false;
	for (Conductor *c : t->conductors())
		if (!s.removed.contains(c))
			return false;
	return true;
}

/**
	@return true when the body of a symbol not in @a ignore lies across the
	straight, level or upright line from @a p to @a q
*/
bool bodyAcross(const Diagram *diagram, const QPointF &p, const QPointF &q,
				const QSet<const Element *> &ignore)
{
	const QRectF span = QRectF(p, q).normalized();
	const QRectF search = span.adjusted(-1, -1, 1, 1);

	for (QGraphicsItem *qgi : diagram->items(search, Qt::IntersectsItemBoundingRect))
	{
		auto *e = qgraphicsitem_cast<Element *>(qgi);
		if (!e || ignore.contains(e))
			continue;

			//Only the inside of the body stops the line: a symbol whose edge
			//the line just touches, or runs along, leaves it free.
		const QRectF body = e->sceneBoundingRect().adjusted(
					kTolerance, kTolerance, -kTolerance, -kTolerance);
		if (body.isEmpty())
			continue;
		if (qFuzzyCompare(span.top() + 1, span.bottom() + 1)) {
			const qreal y = span.top();
			if (y > body.top() && y < body.bottom()
				&& span.right() > body.left() && span.left() < body.right())
				return true;
		} else {
			const qreal x = span.left();
			if (x > body.left() && x < body.right()
				&& span.bottom() > body.top() && span.top() < body.bottom())
				return true;
		}
	}
	return false;
}

/**
	@return true when the body of a symbol other than the two ends lies
	across the straight line from @a a to @a b
*/
bool blocked(const State &s, const Terminal *a, const Terminal *b)
{
	QSet<const Element *> ignore {a->parentElement(), b->parentElement()};
	for (Element *e : s.leaving)
		ignore << e;
	return bodyAcross(s.diagram, a->dockConductor(), b->dockConductor(), ignore);
}

Terminal *findPartner(const State &s, const Terminal *from)
{
	const Element *own = from->parentElement();
	QList<QPair<qreal, Terminal *>> found;
	for (Element *e : s.diagram->elements())
	{
		if (e == own || s.leaving.contains(e))
			continue;
		for (Terminal *t : e->terminals())
		{
			if (!Qet::isOpposed(from->orientation(), t->orientation()))
				continue;
			const qreal d = ahead(from, t->dockConductor());
			if (d >= 0)
				found << qMakePair(d, t);
		}
	}
	std::sort(found.begin(), found.end(),
			  [](const auto &a, const auto &b) { return a.first < b.first; });

	for (const auto &pair : std::as_const(found))
	{
		if (blocked(s, from, pair.second))
			return nullptr;
		if (isFree(s, pair.second))
			return pair.second;
	}
	return nullptr;
}

bool stillValid(const State &s, const Conductor *c)
{
	const Terminal *a = c->terminal1;
	const Terminal *b = c->terminal2;
	if (!a || !b || a->diagram() != s.diagram || b->diagram() != s.diagram)
		return false;
	if (!Qet::isOpposed(a->orientation(), b->orientation()))
		return false;
	if (ahead(a, b->dockConductor()) < 0)
		return false;
	return !blocked(s, a, b);
}

/// @return true when the line @a t looks along crosses @a rect
bool looksAcross(const Terminal *t, const QRectF &rect, const QRectF &border)
{
	const QPointF p = t->dockConductor();
	const QPointF d = direction(t->orientation());
	const QPointF far = p + d * (border.width() + border.height());
	return rect.intersects(QRectF(p, far).normalized().adjusted(-kTolerance, -kTolerance,
																	kTolerance, kTolerance));
}

void makeWire(State &s, Terminal *a, Terminal *b, QUndoCommand *parent)
{
	Diagram *diagram = s.diagram;
	auto *conductor = new Conductor(a, b);
	conductor->setAutomatic(true);
	new AddGraphicsObjectCommand(conductor, diagram, QPointF(), parent);

		//Same rule as a wire drawn by hand: the properties of the potential
		//it joins when they all agree, the defaults otherwise.
	const auto others = conductor->relatedPotentialConductors();
	bool same = !others.isEmpty();
	ConductorProperties props;
	if (same) {
		props = (*others.cbegin())->properties();
		for (const Conductor *other : others)
			if (other->properties() != props)
				same = false;
	}
	if (same) {
		conductor->setProperties(props);
	} else {
		conductor->setProperties(diagram->defaultConductorProperties);
		ConductorAutoNumerotation can(conductor, diagram, parent);
		can.numerate();
	}
	if (diagram->freezeNewConductors() || diagram->project()->isFreezeNewConductors())
		conductor->setFreezeLabel(true);

	s.used << a << b;
}

QList<Terminal *> inOrder(const QSet<Terminal *> &set)
{
	QList<Terminal *> list(set.begin(), set.end());
	std::sort(list.begin(), list.end(), [](const Terminal *a, const Terminal *b) {
		const QPointF pa = a->dockConductor(), pb = b->dockConductor();
		return pa.y() != pb.y() ? pa.y() < pb.y() : pa.x() < pb.x();
	});
	return list;
}

void connectAll(State &s, const QSet<Terminal *> &candidates, QUndoCommand *parent, int *made = nullptr)
{
	for (Terminal *t : inOrder(candidates))
	{
		if (s.leaving.contains(t->parentElement()) || !isFree(s, t))
			continue;
		if (Terminal *other = findPartner(s, t)) {
			makeWire(s, t, other, parent);
			if (made) ++*made;
		}
	}
}

} // namespace

bool AutoConnect::isOn(const Diagram *diagram)
{
	return diagram && diagram->project() && diagram->project()->liveAutoConnect();
}

void AutoConnect::update(Diagram *diagram,
						 const QList<Element *> &changed,
						 const QList<QRectF> &old_rects,
						 QUndoCommand *parent,
						 const QSet<Element *> &leaving)
{
	if (!diagram || !parent)
		return;

	State s;
	s.diagram = diagram;
	s.leaving = leaving;
	QSet<Terminal *> candidates;

		//The wires of the symbols that go are removed by the caller: their
		//far ends are free from now on.
	for (Element *e : leaving)
		for (Terminal *t : e->terminals())
			for (Conductor *c : t->conductors()) {
				s.removed << c;
				Terminal *other = c->terminal1 == t ? c->terminal2 : c->terminal1;
				if (other && !leaving.contains(other->parentElement()))
					candidates << other;
			}

		//Automatic wires that no longer hold
	DiagramContent gone;
	for (Conductor *c : diagram->conductors())
	{
		if (!c->isAutomatic() || s.removed.contains(c) || stillValid(s, c))
			continue;
		s.removed << c;
		gone.m_other_conductors << c;
		candidates << c->terminal1 << c->terminal2;
	}
	if (!gone.m_other_conductors.isEmpty())
		new DeleteQGraphicsItemCommand(diagram, gone, parent);

	for (Element *e : changed)
		for (Terminal *t : e->terminals())
			candidates << t;

		//Terminals a moved or removed symbol stood between
	QList<QRectF> freed_rects = old_rects;
	for (Element *e : leaving)
		freed_rects << e->sceneBoundingRect();
	if (!freed_rects.isEmpty())
	{
		const QRectF border = diagram->border_and_titleblock.insideBorderRect();
		for (Element *e : diagram->elements())
		{
			if (leaving.contains(e))
				continue;
			for (Terminal *t : e->terminals())
			{
				if (candidates.contains(t) || !isFree(s, t))
					continue;
				for (const QRectF &r : std::as_const(freed_rects))
					if (looksAcross(t, r, border)) {
						candidates << t;
						break;
					}
			}
		}
	}

	connectAll(s, candidates, parent);
}

int AutoConnect::connectWholeDiagram(Diagram *diagram, QUndoCommand *parent)
{
	if (!diagram || !parent)
		return 0;
	State s;
	s.diagram = diagram;
	QSet<Terminal *> candidates;
	for (Element *e : diagram->elements())
		for (Terminal *t : e->terminals())
			candidates << t;
	int made = 0;
	connectAll(s, candidates, parent, &made);
	return made;
}

Terminal *AutoConnect::partner(const Terminal *from)
{
	if (!from || !from->diagram())
		return nullptr;
	State s;
	s.diagram = from->diagram();
	return findPartner(s, from);
}

QList<QPointF> AutoConnect::dragPlacements(const Diagram *diagram,
										   const Element *sample,
										   const QLineF &drag,
										   const QSet<const Element *> &ignore)
{
	QList<QPointF> placements;
	if (!diagram || !sample || drag.length() < kTolerance)
		return placements;

	QSet<const Element *> skip = ignore;
	skip << sample;

	for (Element *e : diagram->elements())
	{
		if (skip.contains(e))
			continue;
		for (Terminal *t : e->terminals())
		{
				//Only lines automatic connections own: a terminal with a
				//wire drawn by hand is left alone
			bool by_hand = false;
			for (Conductor *c : t->conductors())
				by_hand |= !c->isAutomatic();
			if (by_hand)
				continue;

			const QPointF p = t->dockConductor();
			const QPointF d = direction(t->orientation());
			if (d.isNull())
				continue;
			const QLineF ray(p, p + d * 1e6);
			QPointF crossing;
			if (ray.intersects(drag, &crossing) != QLineF::BoundedIntersection)
				continue;

			for (Terminal *f : sample->terminals())
			{
				if (!Qet::isOpposed(t->orientation(), f->orientation()))
					continue;
					//The symbol's origin on the crossing, on the grid along
					//the line, and across it where the terminal facing the
					//line lies on it
				const QPointF offset = f->dockConductor() - sample->pos();
				const QPointF grid = Diagram::snapToGrid(crossing);
				QPointF origin;
				if (qFuzzyIsNull(d.x()))
					origin = QPointF(p.x() - offset.x(), grid.y());
				else
					origin = QPointF(grid.x(), p.y() - offset.y());
				const QPointF facing = origin + offset;
				if (ahead(t, facing) <= kTolerance
					|| bodyAcross(diagram, p, facing, skip | QSet<const Element *>{e}))
					continue;

				bool twice = false;
				for (const QPointF &other : std::as_const(placements))
					twice |= QLineF(other, origin).length() < kTolerance;
				if (!twice)
					placements << origin;
				break;
			}
		}
	}

	std::sort(placements.begin(), placements.end(), [](const QPointF &a, const QPointF &b) {
		return a.y() != b.y() ? a.y() < b.y() : a.x() < b.x();
	});
	return placements;
}

QList<QLineF> AutoConnect::previewWires(const Diagram *diagram,
										const QList<Element *> &placed,
										const QSet<const Element *> &ignore)
{
	QList<QLineF> wires;
	if (!diagram)
		return wires;

	State s;
	s.diagram = const_cast<Diagram *>(diagram);
	for (const Element *e : ignore)
		s.leaving << const_cast<Element *>(e);
		//An automatic wire the copies stand across goes when they are
		//placed: its ends count as free
	for (Conductor *c : diagram->conductors())
		if (c->isAutomatic())
			s.removed << c;

	for (Element *e : placed)
		for (Terminal *t : e->terminals())
		{
			if (s.used.contains(t))
				continue;
			if (Terminal *other = findPartner(s, t)) {
				wires << QLineF(t->dockConductor(), other->dockConductor());
				s.used << t << other;
			}
		}
	return wires;
}
