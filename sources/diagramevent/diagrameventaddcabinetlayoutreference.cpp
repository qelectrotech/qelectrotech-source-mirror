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

#include "diagrameventaddcabinetlayoutreference.h"

#include "../diagram.h"
#include "../qetgraphicsitem/cabinetlayoutreferenceitem.h"
#include "../undocommand/addgraphicsobjectcommand.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QRectF>

/**
	@brief DiagramEventAddCabinetLayoutReference::DiagramEventAddCabinetLayoutReference
	@param diagram the diagram where this event must operate
	@param source_element_uuid the device this box represents
	@param is_side_view front (width x height) or side (depth x height) view
	@param initial_pos where the box starts, following the mouse from there
*/
DiagramEventAddCabinetLayoutReference::DiagramEventAddCabinetLayoutReference(
		Diagram *diagram,
		const QUuid &source_element_uuid,
		bool is_side_view,
		const QPointF &initial_pos) :
	DiagramEventInterface(diagram),
	m_reference_item(nullptr)
{
	auto *item = new CabinetLayoutReferenceItem(source_element_uuid, is_side_view);
	m_diagram->addItem(item);
	item->linkToSource(m_diagram->project());
	item->fitFontSizeToBox();

	if (item->isOrphaned()) {
		m_diagram->removeItem(item);
		delete item;
		return;
	}

	item->setPos(initial_pos);
	m_reference_item = item;
	m_running = true;
}

/**
	@brief DiagramEventAddCabinetLayoutReference::~DiagramEventAddCabinetLayoutReference
*/
DiagramEventAddCabinetLayoutReference::~DiagramEventAddCabinetLayoutReference()
{
	if ((m_running || m_abort) && m_reference_item)
	{
		m_diagram->removeItem(m_reference_item);
		delete m_reference_item;
	}

	foreach (QGraphicsView *v, m_diagram->views())
		v->setContextMenuPolicy(Qt::DefaultContextMenu);
}

/**
	@brief DiagramEventAddCabinetLayoutReference::mousePressEvent
	Left click confirms placement at the current position; right
	click cancels (matching DiagramEventAddShape's convention).
	@param event
*/
void DiagramEventAddCabinetLayoutReference::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (Q_UNLIKELY(m_diagram->isReadOnly()) || !m_reference_item) {
		return;
	}

	if (event->button() == Qt::LeftButton)
	{
		QPointF pos = event->scenePos();
		if (event->modifiers() != Qt::ControlModifier) {
			pos = snapToNearbyReferenceEdges(pos);
		}
		m_reference_item->setPos(pos);

		m_diagram->undoStack().push(
			new AddGraphicsObjectCommand(m_reference_item, m_diagram, pos));

		m_reference_item = nullptr; //ownership now with the undo command
		m_running = false;
		emit finish();
		event->setAccepted(true);
		return;
	}

	if (event->button() == Qt::RightButton) {
		m_running = false;
		m_abort = true;
		emit finish();
		event->setAccepted(true);
	}
}

/**
	@brief DiagramEventAddCabinetLayoutReference::mouseMoveEvent
	@param event
*/
void DiagramEventAddCabinetLayoutReference::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_reference_item)
		return;

	QPointF pos = event->scenePos();
	if (event->modifiers() != Qt::ControlModifier) {
		pos = snapToNearbyReferenceEdges(pos);
	}
	m_reference_item->setPos(pos);
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddCabinetLayoutReference::keyPressEvent
	Press space key rotate the box to 90° (return true)
	else call DiagramEventInterface::keyPressEvent(event), and return the value.
	@param event
*/
void DiagramEventAddCabinetLayoutReference::keyPressEvent(QKeyEvent *event)
{
	if (m_reference_item && event->key() == Qt::Key_Space)
	{
		m_reference_item->setRotation(m_reference_item->rotation() + 90);
		event->setAccepted(true);
	}
	else {
		DiagramEventInterface::keyPressEvent(event);
	}
}

/**
	@brief DiagramEventAddCabinetLayoutReference::snapToNearbyReferenceEdges
	@param pos the box's candidate center position (its own
	pos()/boundingRect() origin, not the cursor position -- caller is
	responsible for any cursor-to-item-origin offset). The box's
	boundingRect() is centered on the local origin, so pos() directly
	represents the box's center in scene coordinates.
	@return @a pos, adjusted so the moving box's left or right edge
	lands exactly on another CabinetLayoutReferenceItem's left/right
	edge if one is within a generous horizontal snap tolerance, and its
	top edge lands exactly on another box's top edge if one is
	already within a much tighter vertical tolerance. Left/right
	snapping is deliberately eager (it's the point of this feature --
	letting boxes butt up against each other despite their own
	fractional widths); top snapping is deliberately reluctant, since
	devices in the same row commonly have different heights and
	forcing their tops together would misrepresent that.
*/
QPointF DiagramEventAddCabinetLayoutReference::snapToNearbyReferenceEdges(QPointF pos) const
{
	if (!m_reference_item)
		return pos;

	constexpr qreal horizontal_snap_tolerance = 15.0;
	constexpr qreal vertical_snap_tolerance = 4.0;

	const qreal box_width  = m_reference_item->boundingRect().width();
	const qreal box_height = m_reference_item->boundingRect().height();

	qreal best_dx = horizontal_snap_tolerance;
	qreal best_dy = vertical_snap_tolerance;
	bool found_x = false;
	bool found_y = false;
	qreal snapped_x = pos.x();
	qreal snapped_y = pos.y();

	const qreal moving_left  = pos.x() - box_width / 2;
	const qreal moving_right = pos.x() + box_width / 2;
	const qreal moving_top   = pos.y() - box_height / 2;

	for (QGraphicsItem *item : m_diagram->items())
	{
		if (item->type() != CabinetLayoutReferenceItem::Type || item == m_reference_item)
			continue;

		const QRectF other = item->sceneBoundingRect();

		if (qAbs(moving_left - other.right()) < best_dx) {
			best_dx = qAbs(moving_left - other.right());
			snapped_x = other.right() + box_width / 2;
			found_x = true;
		}
		if (qAbs(moving_right - other.left()) < best_dx) {
			best_dx = qAbs(moving_right - other.left());
			snapped_x = other.left() - box_width / 2;
			found_x = true;
		}
		if (qAbs(moving_left - other.left()) < best_dx) {
			best_dx = qAbs(moving_left - other.left());
			snapped_x = other.left() + box_width / 2;
			found_x = true;
		}
		if (qAbs(moving_right - other.right()) < best_dx) {
			best_dx = qAbs(moving_right - other.right());
			snapped_x = other.right() - box_width / 2;
			found_x = true;
		}

		if (qAbs(moving_top - other.top()) < best_dy) {
			best_dy = qAbs(moving_top - other.top());
			snapped_y = other.top() + box_height / 2;
			found_y = true;
		}
	}

	return QPointF(found_x ? snapped_x : pos.x(),
					found_y ? snapped_y : pos.y());
}

void DiagramEventAddCabinetLayoutReference::init()
{
	foreach (QGraphicsView *v, m_diagram->views())
		v->setContextMenuPolicy(Qt::NoContextMenu);
}