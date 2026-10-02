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
#include "conductorrouter.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <vector>

namespace {

using ConductorRouter::Direction;

constexpr qreal eps = 1e-6;
	///Larger than any folio needs: a 2000 x 2000 grid
constexpr int max_nodes = 4000000;

	///What each thing costs, in scene units of length. A bend is worth
	///three grid steps of extra wire, so the search prefers a slightly
	///longer route to a more crooked one, as a person drawing would.
constexpr qreal bend_steps   = 3.0;
	///Running along another wire costs this much more per unit of length
constexpr qreal along_factor = 3.0;
	///Each crossing of another wire costs two grid steps
constexpr qreal cross_steps  = 2.0;

QPointF step(Direction d)
{
	switch (d) {
		case Direction::North: return {0, -1};
		case Direction::East:  return {1, 0};
		case Direction::South: return {0, 1};
		case Direction::West:  return {-1, 0};
	}
	return {0, 0};
}

Direction opposite(Direction d)
{
	return static_cast<Direction>((static_cast<int>(d) + 2) % 4);
}

bool strictlyInside(const QPointF &p, const QRectF &r)
{
	return p.x() > r.left() + eps && p.x() < r.right() - eps
		&& p.y() > r.top() + eps && p.y() < r.bottom() - eps;
}

bool insideAny(const QPointF &p, const QList<QRectF> &rects)
{
	for (const QRectF &r : rects)
		if (strictlyInside(p, r)) return true;
	return false;
}

	///A wire segment, kept as the fixed coordinate and the range along the
	///other axis
struct Span { qreal at, from, to; };

	///One grid step out of a terminal in its direction, snapped to the
	///grid the way Conductor::extendTerminal() snaps it
QPointF firstStep(const QPointF &dock, Direction d, qreal grid)
{
	const QPointF s = step(d);
	QPointF p = dock;
	if (s.x() != 0)
		p.setX(std::round((dock.x() + s.x() * grid) / grid) * grid);
	else
		p.setY(std::round((dock.y() + s.y() * grid) / grid) * grid);
	return p;
}

	///Whether the horizontal or vertical segment from @p a to @p b runs
	///through the inside of @p rect; along its edge does not count.
bool crossesInside(const QPointF &a, const QPointF &b, const QRectF &rect)
{
	const QRectF seg = QRectF(a, b).normalized();
	if (seg.width() < eps)
		return seg.left() > rect.left() + eps && seg.left() < rect.right() - eps
			&& std::min(seg.bottom(), rect.bottom()) - std::max(seg.top(), rect.top()) > eps;
	return seg.top() > rect.top() + eps && seg.top() < rect.bottom() - eps
		&& std::min(seg.right(), rect.right()) - std::max(seg.left(), rect.left()) > eps;
}

	///The first point of a route after a terminal: firstStep(), then on
	///until it is clear of every obstacle -- of the terminal's own symbol
	///above all.
	///When the terminal's own symbol is known, never through another
	///symbol, only through the margin around it: a terminal pointing
	///straight into one has no exit, where walking on through it would
	///give a route that crosses that symbol and loops back. @p others are
	///the other symbols, without their margin.
bool exitPoint(const QPointF &dock, Direction d, const ConductorRouter::Request &r,
			   const QList<QRectF> &obstacles, const QList<QRectF> &others,
			   bool own_known, QPointF &out)
{
	const QPointF s = step(d);
	QPointF from = dock, p = firstStep(dock, d, r.grid);
	for (int i = 0; i < 200; ++i) {
		if (own_known)
			for (const QRectF &o : others)
				if (crossesInside(from, p, o)) return false;
		if (!insideAny(p, obstacles)) {
			out = p;
			return true;
		}
		from = p;
		p += s * r.grid;
	}
	return false;
}

	///Sorted, without duplicates: the grid lines inside [low, high], plus
	///the extra coordinates (the two exit points, which need not be on the
	///grid when a terminal is not)
QVector<qreal> axis(qreal low, qreal high, qreal grid, std::initializer_list<qreal> extra)
{
	QVector<qreal> v;
	for (qreal c = std::ceil(low / grid) * grid; c <= high + eps; c += grid)
		v << c;
	for (qreal c : extra) v << c;
	std::sort(v.begin(), v.end());
	v.erase(std::unique(v.begin(), v.end(),
						[](qreal a, qreal b) { return std::abs(a - b) < eps; }),
			v.end());
	return v;
}

int indexOf(const QVector<qreal> &v, qreal c)
{
	auto it = std::lower_bound(v.begin(), v.end(), c - eps);
	return (it != v.end() && std::abs(*it - c) < eps) ? int(it - v.begin()) : -1;
}

	///[first, last) of the sorted v: the values strictly between low and high
std::pair<int, int> openRange(const QVector<qreal> &v, qreal low, qreal high)
{
	const int first = int(std::upper_bound(v.begin(), v.end(), low) - v.begin());
	const int last  = int(std::lower_bound(v.begin(), v.end(), high) - v.begin());
	return {first, std::max(first, last)};
}

	///The cost of every edge along one axis, or infinity where an obstacle
	///blocks it. Edge (k, i) runs from pos[i] to pos[i + 1] at the fixed
	///coordinate lines[k], and is stored at [k * pos.size() + i];
	///horizontal says which axis pos is. Each obstacle and wire visits only
	///the edges it can touch, so the cost grows with the folio, not with
	///the folio times everything drawn on it.
std::vector<qreal> edgeCosts(bool horizontal,
							 const QVector<qreal> &pos, const QVector<qreal> &lines,
							 const QList<QRectF> &obstacles,
							 const QVector<Span> &along, const QVector<Span> &across,
							 qreal grid)
{
	const int np = pos.size(), nl = lines.size();
	std::vector<qreal> cost(size_t(np) * nl, std::numeric_limits<qreal>::infinity());
	for (int k = 0; k < nl; ++k)
		for (int i = 0; i + 1 < np; ++i)
			cost[size_t(k) * np + i] = pos[i + 1] - pos[i];

		// The edges i that can overlap (low, high): pos[i + 1] > low and
		// pos[i] < high. The exact test is applied to each one found.
	const auto edges = [&](qreal low, qreal high) {
		const int first = int(std::upper_bound(pos.begin(), pos.end(), low) - pos.begin()) - 1;
		const int last  = int(std::lower_bound(pos.begin(), pos.end(), high) - pos.begin());
		return std::pair<int, int>{std::max(0, first), std::min(np - 1, last)};
	};

	for (const QRectF &r : obstacles) {
		const qreal lo  = horizontal ? r.left()  : r.top();
		const qreal hi  = horizontal ? r.right() : r.bottom();
		const qreal flo = horizontal ? r.top()   : r.left();
		const qreal fhi = horizontal ? r.bottom(): r.right();
		const auto [k0, k1] = openRange(lines, flo, fhi);
		const auto [i0, i1] = edges(lo, hi);
		for (int k = k0; k < k1; ++k) {
			if (!(lines[k] > flo + eps && lines[k] < fhi - eps)) continue;
			for (int i = i0; i < i1; ++i)
				if (std::min(pos[i + 1], hi) - std::max(pos[i], lo) > eps)
					cost[size_t(k) * np + i] = std::numeric_limits<qreal>::infinity();
		}
	}
	for (const Span &s : along) {
		const int k0 = int(std::lower_bound(lines.begin(), lines.end(), s.at - 0.5) - lines.begin());
		const auto [i0, i1] = edges(s.from, s.to);
		for (int k = k0; k < nl && lines[k] <= s.at + 0.5; ++k) {
			if (std::abs(s.at - lines[k]) > 0.5) continue;
			for (int i = i0; i < i1; ++i) {
				const qreal overlap = std::min(pos[i + 1], s.to) - std::max(pos[i], s.from);
				if (overlap > eps) cost[size_t(k) * np + i] += overlap * along_factor;
			}
		}
	}
	for (const Span &s : across) {
			// Half-open (a, b], so a crossing on a grid node is counted
			// once, by the edge that ends on it; and only through the
			// other wire's interior, not at its end, which is a junction.
		const auto [k0, k1] = openRange(lines, s.from, s.to);
		const auto [i0, i1] = edges(s.at - 1.0, s.at + 1.0);
		for (int k = k0; k < k1; ++k) {
			if (!(lines[k] > s.from + 0.5 && lines[k] < s.to - 0.5)) continue;
			for (int i = i0; i < i1; ++i)
				if (s.at > pos[i] + eps && s.at <= pos[i + 1] + eps)
					cost[size_t(k) * np + i] += cross_steps * grid;
		}
	}
	return cost;
}

} // namespace

ConductorRouter::Result ConductorRouter::route(const Request &r)
{
	Result result;
	if (r.grid <= 0) {
		result.error = QStringLiteral("the grid step must be positive");
		return result;
	}

	const QRectF own1 = r.start_symbol.normalized(), own2 = r.end_symbol.normalized();
	QList<QRectF> obstacles, others, other_symbols;
	for (const QRectF &o : r.obstacles) {
		const QRectF n = o.normalized();
		const bool own = (own1.isValid() && n == own1) || (own2.isValid() && n == own2);
			// A symbol drawn around a terminal's own one is a frame the
			// wire starts or ends inside: crossing its edge is the way in
			// or out, and its inside is the place to route.
		if (!own && ((own1.isValid() && n.contains(own1))
					 || (own2.isValid() && n.contains(own2))))
			continue;
		const QRectF padded = n.adjusted(-r.margin, -r.margin, r.margin, r.margin);
		obstacles << padded;
		if (!own) {
			others << padded;
			other_symbols << n;
		}
	}

		// Two terminals facing each other on one line, with nothing
		// between them: the straight line, even when they are too close
		// for each to step out a grid square first. With room, it keeps
		// the step out of each terminal, as the search does (see the
		// corners below); without, the point between them gives the path
		// the three points a conductor's path needs.
	const QPointF s = step(r.start_direction);
	const QPointF ahead = r.end - r.start;
	if (r.end_direction == opposite(r.start_direction)
		&& std::abs(s.x() != 0 ? ahead.y() : ahead.x()) < eps
		&& ahead.x() * s.x() + ahead.y() * s.y() > eps) {
		bool clear = true;
		for (const QRectF &o : others)
			if (crossesInside(r.start, r.end, o)) { clear = false; break; }
			// nor along another wire, which the search would avoid
		const bool vertical = s.x() == 0;
		const qreal line = vertical ? r.start.x() : r.start.y();
		const qreal lo = vertical ? std::min(r.start.y(), r.end.y()) : std::min(r.start.x(), r.end.x());
		const qreal hi = vertical ? std::max(r.start.y(), r.end.y()) : std::max(r.start.x(), r.end.x());
		for (const QVector<QPointF> &w : r.wires) {
			for (int i = 0; clear && i + 1 < w.size(); ++i) {
				const QPointF a = w.at(i), b = w.at(i + 1);
				const qreal a_at = vertical ? a.x() : a.y(), b_at = vertical ? b.x() : b.y();
				if (std::abs(a_at - line) > 0.5 || std::abs(b_at - line) > 0.5) continue;
				const qreal a_on = vertical ? a.y() : a.x(), b_on = vertical ? b.y() : b.x();
				if (std::min(hi, std::max(a_on, b_on)) - std::max(lo, std::min(a_on, b_on)) > eps)
					clear = false;
			}
		}
		if (clear) {
			const QPointF e1 = firstStep(r.start, r.start_direction, r.grid);
			const QPointF e2 = firstStep(r.end, r.end_direction, r.grid);
			const QPointF gap = e2 - e1;
			result.points << r.start;
			if (gap.x() * s.x() + gap.y() * s.y() > eps)
				result.points << e1 << e2;
			else if (std::abs(gap.x()) < eps && std::abs(gap.y()) < eps)
				result.points << e1;
			else
				result.points << (r.start + r.end) / 2;
			result.points << r.end;
			return result;
		}
	}

	QPointF s1, s2;
	if (!exitPoint(r.start, r.start_direction, r, obstacles, other_symbols, own1.isValid(), s1)
		|| !exitPoint(r.end, r.end_direction, r, obstacles, other_symbols, own2.isValid(), s2)) {
		result.error = QStringLiteral("a terminal points straight into another symbol, "
									  "or has no way out of the symbols around it");
		return result;
	}

		// The area searched: everything involved, with room to go round
		// it, kept on the folio when there is one.
	QRectF box = QRectF(s1, s2).normalized();
	for (const QRectF &o : obstacles) box = box.united(o);
	box.adjust(-3 * r.grid, -3 * r.grid, 3 * r.grid, 3 * r.grid);
	if (r.bounds.isValid()) box = box.intersected(r.bounds);
	box = box.united(QRectF(s1, s2).normalized());

	const QVector<qreal> xs = axis(box.left(), box.right(), r.grid, {s1.x(), s2.x()});
	const QVector<qreal> ys = axis(box.top(), box.bottom(), r.grid, {s1.y(), s2.y()});
	const int nx = xs.size(), ny = ys.size();
	if (qint64(nx) * ny > max_nodes) {
		result.error = QStringLiteral("the area to search is too large");
		return result;
	}

	QVector<Span> horizontal_wires, vertical_wires;
	for (const QVector<QPointF> &w : r.wires) {
		for (int i = 0; i + 1 < w.size(); ++i) {
			const QPointF a = w.at(i), b = w.at(i + 1);
			if (std::abs(a.y() - b.y()) < 0.5 && std::abs(a.x() - b.x()) > eps)
				horizontal_wires << Span{a.y(), std::min(a.x(), b.x()), std::max(a.x(), b.x())};
			else if (std::abs(a.x() - b.x()) < 0.5 && std::abs(a.y() - b.y()) > eps)
				vertical_wires << Span{a.x(), std::min(a.y(), b.y()), std::max(a.y(), b.y())};
		}
	}

		// The cost of the edge from each node to the next one east,
		// h_cost[j * nx + i], and south, v_cost[i * ny + j]; infinity
		// where blocked or none.
	const qreal inf = std::numeric_limits<qreal>::infinity();
	const std::vector<qreal> h_cost = edgeCosts(true, xs, ys, obstacles,
											   horizontal_wires, vertical_wires, r.grid);
	const std::vector<qreal> v_cost = edgeCosts(false, ys, xs, obstacles,
											   vertical_wires, horizontal_wires, r.grid);

	const int start_node = indexOf(ys, s1.y()) * nx + indexOf(xs, s1.x());
	const int goal_node  = indexOf(ys, s2.y()) * nx + indexOf(xs, s2.x());
	const qreal bend = bend_steps * r.grid;
		// The wire arrives at the second terminal moving opposite to the
		// way it points.
	const Direction arrival = opposite(r.end_direction);

		// Dijkstra over (node, direction of travel): the direction is what
		// lets a bend be charged for.
	const size_t states = size_t(nx) * ny * 4;
	std::vector<qreal> dist(states, inf);
	std::vector<int> previous(states, -1);
	using Entry = std::pair<qreal, int>;
	std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
	const int first = start_node * 4 + int(r.start_direction);
	dist[size_t(first)] = 0;
	queue.push({0, first});

	qreal best = inf;
	int best_state = -1;
	while (!queue.empty()) {
		const auto [cost, state] = queue.top();
		queue.pop();
		if (cost > dist[size_t(state)] || cost >= best) {
			if (cost >= best) break;
			continue;
		}
		const int node = state / 4;
		const Direction d = static_cast<Direction>(state % 4);
		if (node == goal_node && d != r.end_direction) {
			const qreal total = cost + (d == arrival ? 0 : bend);
			if (total < best) { best = total; best_state = state; }
		}
		const int i = node % nx, j = node / nx;
		for (int k = 0; k < 4; ++k) {
			const Direction nd = static_cast<Direction>(k);
			if (nd == opposite(d)) continue;
			int ni = i, nj = j;
			qreal edge = inf;
			switch (nd) {
				case Direction::East:  if (i + 1 < nx) { ni = i + 1; edge = h_cost[size_t(j) * nx + i]; } break;
				case Direction::West:  if (i > 0)      { ni = i - 1; edge = h_cost[size_t(j) * nx + ni]; } break;
				case Direction::South: if (j + 1 < ny) { nj = j + 1; edge = v_cost[size_t(i) * ny + j]; } break;
				case Direction::North: if (j > 0)      { nj = j - 1; edge = v_cost[size_t(i) * ny + nj]; } break;
			}
			if (edge == inf) continue;
			const qreal next_cost = cost + edge + (nd == d ? 0 : bend);
			const int next = (nj * nx + ni) * 4 + k;
			if (next_cost < dist[size_t(next)]) {
				dist[size_t(next)] = next_cost;
				previous[size_t(next)] = state;
				queue.push({next_cost, next});
			}
		}
	}

	if (best_state < 0) {
		result.error = QStringLiteral("no path around the symbols was found");
		return result;
	}

	QList<QPointF> chain;
	for (int s = best_state; s >= 0; s = previous[size_t(s)]) {
		const int node = s / 4;
		chain.prepend(QPointF(xs[node % nx], ys[node / nx]));
	}

		// Keep only the corners. The two exit points stay even when
		// straight on, as Conductor::generateConductorPath() keeps them:
		// the segment from a terminal to its exit point is the one the
		// application holds fixed when a wire is edited by hand.
	QList<QPointF> corners;
	for (int k = 0; k < chain.size(); ++k) {
		const QPointF p = chain.at(k);
		if (k > 0 && k + 1 < chain.size()) {
			const QPointF a = chain.at(k - 1), b = chain.at(k + 1);
			const bool straight = (std::abs(a.x() - p.x()) < eps && std::abs(b.x() - p.x()) < eps)
							   || (std::abs(a.y() - p.y()) < eps && std::abs(b.y() - p.y()) < eps);
			if (straight) continue;
		}
		corners << p;
	}

	result.points << r.start;
	for (const QPointF &p : corners)
		if (result.points.last() != p) result.points << p;
	if (result.points.last() != r.end) result.points << r.end;
	return result;
}
