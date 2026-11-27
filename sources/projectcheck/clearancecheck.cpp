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
#include "clearancecheck.h"

#include "../conductorsegment.h"
#include "../diagram.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/conductortextitem.h"
#include "../qetgraphicsitem/diagramtextitem.h"
#include "../qetgraphicsitem/independenttextitem.h"

#include <QHash>
#include <QSet>
#include <algorithm>

namespace {
		/// The wire has to run this far side by side to count: a corner
		/// or a touch is not
	constexpr qreal minimum_overlap = 0.5;

		/// Wires on the grid are 1 M apart to within what mapToScene
		/// rounds: 9.9999 px is not closer than 10 px
	constexpr qreal tolerance = 1e-3;
		/// 1 M is one step of the grid, 10 px, whatever the module M is
		/// in mm: the wires of QElectroTech sit on that grid
	constexpr qreal one_module_px = 10.0;

		/// A wire segment, along one axis: across is where it is on the
		/// other, along its extent
	struct Segment
	{
		Conductor *conductor = nullptr;
		qreal across = 0;
		qreal from = 0;
		qreal to = 0;
		QPointF first;
		QPointF second;
	};

		/// A text rect, in the same two directions
	struct TextBox
	{
		DiagramTextItem *item = nullptr;
		Conductor *conductor = nullptr;   ///< its wire, if a wire's text
		qreal across_from = 0;
		qreal across_to = 0;
		qreal from = 0;
		qreal to = 0;
		qreal middle() const {return (across_from + across_to) / 2;}
	};

	struct Geometry
	{
		QVector<Segment> horizontal;   ///< across is y
		QVector<Segment> vertical;     ///< across is x
		QVector<TextBox> horizontal_texts;
		QVector<TextBox> vertical_texts;
	};

	bool lessAcross(const Segment &a, const Segment &b)
	{
		return a.across < b.across;
	}

	bool shareTerminal(const Conductor *a, const Conductor *b)
	{
		for (const auto *x : {a->terminal1, a->terminal2})
			if (x && (x == b->terminal1 || x == b->terminal2)) return true;
		return false;
	}

		/// Whether two segments meet end to end or at a point
	bool touch(const Segment &a, const Segment &b)
	{
		for (const QPointF &p : {a.first, a.second})
			for (const QPointF &q : {b.first, b.second})
				if (QLineF(p, q).length() < minimum_overlap) return true;
		return false;
	}

	QRectF transposed(const QRectF &rect)
	{
		return QRectF(rect.y(), rect.x(), rect.height(), rect.width());
	}

	void addText(Geometry &geometry, DiagramTextItem *text,
				 Conductor *conductor)
	{
		if (!text || !text->isVisible()
			|| text->toPlainText().trimmed().isEmpty())
			return;
		const QRectF rect = text->mapRectToScene(text->writtenRect());
		if (rect.isEmpty()) return;
		TextBox box;
		box.item = text;
		box.conductor = conductor;
		box.across_from = rect.top();
		box.across_to = rect.bottom();
		box.from = rect.left();
		box.to = rect.right();
		geometry.horizontal_texts << box;
		const QRectF flipped = transposed(rect);
		box.across_from = flipped.top();
		box.across_to = flipped.bottom();
		box.from = flipped.left();
		box.to = flipped.right();
		geometry.vertical_texts << box;
	}

	Geometry geometryOf(Diagram *diagram)
	{
		Geometry geometry;
		for (QGraphicsItem *item : diagram->items()) {
			if (auto *conductor = qgraphicsitem_cast<Conductor *>(item)) {
				addText(geometry, conductor->textItem(), conductor);
				for (ConductorSegment *segment : conductor->segmentsList()) {
					if (!segment || segment->isPoint()) continue;
					const QPointF a = conductor->mapToScene(
						segment->firstPoint());
					const QPointF b = conductor->mapToScene(
						segment->secondPoint());
					Segment found;
					found.conductor = conductor;
					found.first = a;
					found.second = b;
					if (qAbs(a.y() - b.y()) < 0.01) {
						found.across = a.y();
						found.from = qMin(a.x(), b.x());
						found.to = qMax(a.x(), b.x());
						geometry.horizontal << found;
					} else if (qAbs(a.x() - b.x()) < 0.01) {
						found.across = a.x();
						found.from = qMin(a.y(), b.y());
						found.to = qMax(a.y(), b.y());
						geometry.vertical << found;
					}
				}
			} else if (item->type() == IndependentTextItem::Type) {
				addText(geometry, static_cast<IndependentTextItem *>(item),
						nullptr);
			}
		}
		std::sort(geometry.horizontal.begin(), geometry.horizontal.end(),
				  lessAcross);
		std::sort(geometry.vertical.begin(), geometry.vertical.end(),
				  lessAcross);
		const auto by_middle = [](const TextBox &a, const TextBox &b) {
			return a.middle() < b.middle();
		};
		std::sort(geometry.horizontal_texts.begin(),
				  geometry.horizontal_texts.end(), by_middle);
		std::sort(geometry.vertical_texts.begin(),
				  geometry.vertical_texts.end(), by_middle);
		return geometry;
	}

		/// Whether a text, its middle between the two lines, lies along
		/// [from, to]
	bool textBetween(const QVector<TextBox> &texts, qreal low, qreal high,
					 qreal from, qreal to)
	{
		auto it = std::lower_bound(texts.cbegin(), texts.cend(), low,
			[](const TextBox &box, qreal value) {
				return box.middle() <= value;
			});
		for (; it != texts.cend() && it->middle() < high ; ++it)
			if (it->from < to && it->to > from) return true;
		return false;
	}

	QPointF point(bool horizontal, qreal along, qreal across)
	{
		return horizontal ? QPointF(along, across) : QPointF(across, along);
	}

		/// Whether @p a is a worse finding than @p b: a plain one (under
		/// 1 M) is worse than one with a text between (under 2 M), then
		/// the smaller distance is
	bool worse(const ClearanceCheck::Clearance &a,
			   const ClearanceCheck::Clearance &b)
	{
		if (a.text != b.text) return !a.text;
		return a.distance < b.distance;
	}

		/// One finding per pair of wires, the worst of all the segments
		/// of both directions: @p seen gives its place in @p found
	void clearancesOf(const QVector<Segment> &segments,
					  const QVector<TextBox> &texts, bool horizontal,
					  QList<ClearanceCheck::Clearance> &found,
					  QHash<QPair<Conductor *, Conductor *>, int> &seen)
	{
		const qreal one = one_module_px;
		const qreal two = 2 * one;
		for (int i = 0 ; i < segments.size() ; ++i) {
			const Segment &a = segments.at(i);
			for (int j = i + 1 ; j < segments.size() ; ++j) {
				const Segment &b = segments.at(j);
				const qreal distance = b.across - a.across;
				if (distance >= two - tolerance) break;
				if (a.conductor == b.conductor) continue;
				const qreal from = qMax(a.from, b.from);
				const qreal to = qMin(a.to, b.to);
				if (to - from < minimum_overlap) continue;
				if (shareTerminal(a.conductor, b.conductor) || touch(a, b))
					continue;
				const bool text = distance >= one - tolerance;
				if (text
					&& !textBetween(texts, a.across, b.across, from, to))
					continue;
				const auto pair = a.conductor < b.conductor
					? qMakePair(a.conductor, b.conductor)
					: qMakePair(b.conductor, a.conductor);
				ClearanceCheck::Clearance clearance;
				clearance.first = a.conductor;
				clearance.second = b.conductor;
				clearance.distance = distance;
				clearance.text = text;
				clearance.where = point(horizontal, (from + to) / 2,
										(a.across + b.across) / 2);
				const auto known = seen.constFind(pair);
				if (known == seen.cend()) {
					seen.insert(pair, found.size());
					found << clearance;
				} else if (worse(clearance, found.at(*known))) {
					found[*known] = clearance;
				}
			}
		}
	}

		/// The wire standing for the potential of @p conductor: the one
		/// with the lowest uuid, kept in @p representatives
	Conductor *representative(Conductor *conductor,
							  QHash<Conductor *, Conductor *> &representatives)
	{
		const auto known = representatives.constFind(conductor);
		if (known != representatives.cend()) return *known;
		Conductor *lowest = conductor;
		const auto related = conductor->relatedPotentialConductors();
		for (Conductor *other : related)
			if (other->uuid() < lowest->uuid()) lowest = other;
		for (Conductor *other : related)
			representatives.insert(other, lowest);
		representatives.insert(conductor, lowest);
		return lowest;
	}

	void textsOf(const QVector<Segment> &segments,
				 const QVector<TextBox> &texts, bool horizontal,
				 QList<ClearanceCheck::TextOnLine> &found,
				 QSet<QPair<const void *, Conductor *>> &seen,
				 QHash<Conductor *, Conductor *> &representatives)
	{
		for (const TextBox &box : texts) {
				//A wire's number crosses the other wires of its own
				//potential (the same wire number) without harm: two
				//wires are of one potential when they have the same
				//representative
			Conductor *own = box.conductor
				? representative(box.conductor, representatives) : nullptr;
			auto it = std::lower_bound(segments.cbegin(), segments.cend(),
				box.across_from,
				[](const Segment &segment, qreal value) {
					return segment.across <= value;
				});
			for (; it != segments.cend() && it->across < box.across_to ; ++it) {
				if (it->conductor == box.conductor) continue;
				if (it->from >= box.to || it->to <= box.from) continue;
				Conductor *crossed = representative(it->conductor,
													representatives);
				if (own && crossed == own) continue;
					//One finding for each potential the text crosses,
					//however many of its wires lie under the text
				const auto key = qMakePair(
					static_cast<const void *>(box.item), crossed);
				if (seen.contains(key)) continue;
				seen.insert(key);
				ClearanceCheck::TextOnLine line;
				line.text = box.item;
					//The wire under the text, on its folio: the
					//representative of the potential may be on another
				line.conductor = it->conductor;
				line.potential = crossed;
				line.where = point(horizontal,
								   (qMax(it->from, box.from)
									+ qMin(it->to, box.to)) / 2,
								   it->across);
				found << line;
			}
		}
	}
}

namespace {
		/// The first @p cap of @p found by place in the folio, top to
		/// bottom then left to right, so that no direction is favoured;
		/// @p omitted gets the number left out
	template<typename T>
	void capFindings(QList<T> &found, int cap, int *omitted)
	{
		std::stable_sort(found.begin(), found.end(),
						 [](const T &a, const T &b) {
			return a.where.y() != b.where.y() ? a.where.y() < b.where.y()
											  : a.where.x() < b.where.x();
		});
		const int over = qMax(0, found.size() - cap);
		if (over > 0) found.erase(found.begin() + cap, found.end());
		if (omitted) *omitted = over;
	}
}

QList<ClearanceCheck::Clearance> ClearanceCheck::clearances(
	Diagram *diagram, int *omitted, int cap)
{
	QList<Clearance> found;
	if (omitted) *omitted = 0;
	if (!diagram) return found;
	const Geometry geometry = geometryOf(diagram);
	QHash<QPair<Conductor *, Conductor *>, int> seen;
	clearancesOf(geometry.horizontal, geometry.horizontal_texts, true, found,
				 seen);
	clearancesOf(geometry.vertical, geometry.vertical_texts, false, found,
				 seen);
	capFindings(found, cap, omitted);
	return found;
}

QList<ClearanceCheck::TextOnLine> ClearanceCheck::textsOnLines(
	Diagram *diagram, int *omitted, int cap)
{
	QList<TextOnLine> found;
	if (omitted) *omitted = 0;
	if (!diagram) return found;
	const Geometry geometry = geometryOf(diagram);
	QSet<QPair<const void *, Conductor *>> seen;
	QHash<Conductor *, Conductor *> representatives;
	textsOf(geometry.horizontal, geometry.horizontal_texts, true, found, seen,
			representatives);
	textsOf(geometry.vertical, geometry.vertical_texts, false, found, seen,
			representatives);
	capFindings(found, cap, omitted);
	return found;
}
