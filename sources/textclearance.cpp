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
#include "textclearance.h"

#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "diagram.h"

#include <QCoreApplication>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QUndoCommand>

namespace {

struct Obstacle {
	QPainterPath shape;
	QRectF box;
};

	/// The symbol a text belongs to, if any
Element *ownerOf(QGraphicsItem *text)
{
	for (QGraphicsItem *p = text->parentItem(); p; p = p->parentItem())
		if (Element *element = qgraphicsitem_cast<Element *>(p))
			return element;
	return nullptr;
}

bool isSelected(QGraphicsItem *text)
{
	if (text->isSelected())
		return true;
	Element *owner = ownerOf(text);
	return owner && owner->isSelected();
}

	/// Whether this text is one the planner moves
bool eligible(DiagramTextItem *text)
{
	if (!text->isVisible() || text->effectiveOpacity() <= 0
	    || text->toPlainText().trimmed().isEmpty())
		return false;
	if (auto *dynamic = qgraphicsitem_cast<DynamicElementTextItem *>(text))
		return !dynamic->parentGroup();
	return qgraphicsitem_cast<IndependentTextItem *>(text) != nullptr;
}

bool hits(const QRectF &box, const QList<Obstacle> &obstacles)
{
	for (const Obstacle &o : obstacles)
		if (o.box.intersects(box) && o.shape.intersects(box))
			return true;
	return false;
}

} // namespace

/**
	@brief TextClearance::plan
	@param diagram folio to look at
	@param settings gap and limits
	@param selectedOnly only the selected texts and the texts of selected symbols
	@return what would move; nothing is changed
*/
TextClearance::Plan TextClearance::plan(Diagram *diagram, const Settings &settings, bool selectedOnly)
{
	Plan result;
	if (!diagram)
		return result;

	QList<Obstacle> obstacles;
	QList<DiagramTextItem *> texts;
	for (QGraphicsItem *item : diagram->items())
	{
		if (auto *wire = qgraphicsitem_cast<Conductor *>(item))
		{
			QPainterPathStroker stroker;
			stroker.setWidth(qMax<qreal>(0.01, wire->properties().cond_size) + settings.distance * 2);
			stroker.setCapStyle(Qt::SquareCap);
			stroker.setJoinStyle(Qt::RoundJoin);
			const QPainterPath shape = wire->mapToScene(stroker.createStroke(wire->path()));
			obstacles.append({shape, shape.boundingRect()});
		}
		else if (auto *text = dynamic_cast<DiagramTextItem *>(item))
		{
			if (eligible(text) && (!selectedOnly || isSelected(text)))
				texts.append(text);
		}
	}

	struct Step { qreal x, y; };
	QList<Step> steps;
	if (settings.direction != Vertical)
		steps << Step{1, 0} << Step{-1, 0};
	if (settings.direction != Horizontal)
		steps << Step{0, 1} << Step{0, -1};

	const int limit = qMax(1, qRound(settings.maxMove));
	for (DiagramTextItem *text : std::as_const(texts))
	{
		++result.considered;
		const QRectF box = text->sceneBoundingRect();
		if (!hits(box, obstacles))
		{
			++result.clear;
			continue;
		}

			//The nearest free place, a unit at a time
		QPointF found;
		bool ok = false;
		for (int d = 1; d <= limit && !ok; ++d)
			for (const Step &s : std::as_const(steps))
			{
				const QPointF delta(s.x * d, s.y * d);
				if (hits(box.translated(delta), obstacles))
					continue;
				found = delta;
				ok = true;
				break;
			}
		if (!ok)
		{
			++result.unresolved;
			continue;
		}

			//The move is in the scene; the text's position is in its parent
		QGraphicsItem *parent = text->parentItem();
		const QPointF inParent = parent
				? parent->mapFromScene(found) - parent->mapFromScene(QPointF(0, 0))
				: found;
		result.moves.append({text, text->pos(), text->pos() + inParent});
	}
	return result;
}

/**
	@brief TextClearance::command
	@return one undo step moving every text of the plan, or nullptr when
	nothing moves. Pushing it onto the undo stack moves the texts.
*/
QUndoCommand *TextClearance::command(const Plan &plan)
{
	if (plan.moves.isEmpty())
		return nullptr;
	auto *undo = new QUndoCommand(
				QCoreApplication::translate("TextClearance", "Keep texts clear of wires"));
	for (const Move &move : plan.moves)
		if (move.text)
			new QPropertyUndoCommand(move.text, "pos", move.before, move.after, undo);
	if (!undo->childCount())
	{
		delete undo;
		return nullptr;
	}
	return undo;
}
