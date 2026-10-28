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
#include "conductordefinitionmark.h"

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../conductorsegment.h"
#include "../diagram.h"
#include "conductor.h"
#include "conductortextitem.h"

#include <QFontMetricsF>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QtMath>

namespace {
		/// Half the stroke's extent along each axis
	const qreal half_stroke = 4.0;
		/// Room between the stroke's end and its text
	const qreal text_gap = 2.0;
}

ConductorDefinitionMark::ConductorDefinitionMark(Conductor *conductor) :
	QGraphicsObject(conductor),
	m_conductor(conductor)
{
	setAcceptHoverEvents(true);
	setCursor(Qt::SizeAllCursor);
	setToolTip(tr("Drag along the conductor to move the colour and section"));
	refresh();
}

/**
	@return the length of the polyline @p points
*/
qreal ConductorDefinitionMark::length(const QList<QPointF> &points)
{
	qreal total = 0;
	for (int i = 1 ; i < points.size() ; ++i)
		total += QLineF(points.at(i - 1), points.at(i)).length();
	return total;
}

/**
	@return the point at @p distance from the start of the polyline
	@p points, kept on it; @p vertical tells whether the stretch there runs
	up and down
*/
QPointF ConductorDefinitionMark::pointAt(const QList<QPointF> &points,
					 qreal distance, bool *vertical)
{
	if (points.isEmpty())
		return QPointF();
	qreal left = qMax<qreal>(0, distance);
	for (int i = 1 ; i < points.size() ; ++i)
	{
		const QLineF stretch(points.at(i - 1), points.at(i));
		if (left <= stretch.length() || i == points.size() - 1)
		{
			if (vertical)
				*vertical = qAbs(stretch.dx()) < qAbs(stretch.dy());
			if (stretch.length() <= 0)
				return stretch.p1();
			return stretch.pointAt(qMin<qreal>(1, left / stretch.length()));
		}
		left -= stretch.length();
	}
	return points.first();
}

/**
	@return how far from the start of the polyline @p points the point of
	it nearest to @p point lies
*/
qreal ConductorDefinitionMark::distanceOf(const QList<QPointF> &points,
					  const QPointF &point)
{
	qreal best = 0, best_gap = -1, walked = 0;
	for (int i = 1 ; i < points.size() ; ++i)
	{
		const QLineF stretch(points.at(i - 1), points.at(i));
		const qreal len = stretch.length();
		qreal t = 0;
		if (len > 0) {
			const QPointF d = stretch.p2() - stretch.p1();
			t = qBound<qreal>(0, QPointF::dotProduct(point - stretch.p1(), d) / (len * len), 1);
		}
		const qreal gap = QLineF(stretch.pointAt(t), point).length();
		if (best_gap < 0 || gap < best_gap) {
			best_gap = gap;
			best = walked + t * len;
		}
		walked += len;
	}
	return best;
}

/**
	@return where the mark stands until it is moved: the middle of the
	longest stretch, where the conductor's text goes too
*/
qreal ConductorDefinitionMark::defaultDistance(const QList<QPointF> &points)
{
	qreal walked = 0, best = 0, best_length = -1;
	for (int i = 1 ; i < points.size() ; ++i)
	{
		const qreal len = QLineF(points.at(i - 1), points.at(i)).length();
		if (len > best_length) {
			best_length = len;
			best = walked + len / 2;
		}
		walked += len;
	}
	return best;
}

QList<QPointF> ConductorDefinitionMark::points() const
{
	QList<QPointF> list;
	const QList<ConductorSegment *> segments = m_conductor->segmentsList();
	if (segments.isEmpty())
		return list;
	list << segments.first()->firstPoint();
	for (ConductorSegment *segment : segments)
		list << segment->secondPoint();
	return list;
}

/**
	Put the mark where the conductor's properties say, and take over their
	colour and section
*/
void ConductorDefinitionMark::refresh()
{
	const qreal wanted = m_conductor->ownProperties().m_definition_mark_pos;
	const QList<QPointF> list = points();
	place(wanted < 0 ? defaultDistance(list) : qMin(wanted, length(list)));
}

void ConductorDefinitionMark::place(qreal distance)
{
	prepareGeometryChange();
	m_distance = distance;
	setPos(pointAt(points(), distance, &m_vertical));
	update();
}

/**
	@return where the colour is written, on the sheet: on a vertical
	conductor upright, left of the conductor and just above the stroke;
	on a horizontal one above the conductor, just past the stroke
*/
QRectF ConductorDefinitionMark::colorRect() const
{
	return textRect(m_conductor->ownProperties().m_wire_color, true);
}

/**
	@return where the section is written: level with the colour, on the
	other side of the conductor
*/
QRectF ConductorDefinitionMark::sectionRect() const
{
	return textRect(m_conductor->ownProperties().m_wire_section, false);
}

QRectF ConductorDefinitionMark::textRect(const QString &text, bool first_side) const
{
	const QFontMetricsF metrics(m_conductor->textItem()->font());
	const qreal width = metrics.horizontalAdvance(text);
	const qreal height = metrics.height();
	if (m_vertical)
		return QRectF(first_side ? -height - text_gap / 2 : text_gap / 2,
			      -half_stroke - text_gap - width, height, width);
	return QRectF(half_stroke + text_gap,
		      first_side ? -height - text_gap / 2 : text_gap / 2, width, height);
}

/**
	Write @p text into @p rect, upright on a vertical conductor
*/
void ConductorDefinitionMark::drawText(QPainter *painter, const QRectF &rect,
				       const QString &text) const
{
	if (!m_vertical) {
		painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
		return;
	}
	painter->save();
	painter->translate(rect.bottomLeft());
	painter->rotate(-90);
	painter->drawText(QRectF(0, 0, rect.height(), rect.width()),
			  Qt::AlignLeft | Qt::AlignVCenter, text);
	painter->restore();
}

QRectF ConductorDefinitionMark::boundingRect() const
{
	return QRectF(-half_stroke - 1, -half_stroke - 1,
		      2 * half_stroke + 2, 2 * half_stroke + 2)
		.united(colorRect()).united(sectionRect());
}

QPainterPath ConductorDefinitionMark::shape() const
{
	QPainterPath path;
	path.addRect(QRectF(-half_stroke - 2, -half_stroke - 2,
			    2 * half_stroke + 4, 2 * half_stroke + 4));
	path.addRect(colorRect());
	path.addRect(sectionRect());
	return path;
}

void ConductorDefinitionMark::paint(QPainter *painter,
				    const QStyleOptionGraphicsItem *option,
				    QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)
	const ConductorProperties p = m_conductor->ownProperties();
	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);

	QColor stroke_color = p.color;
	if (Diagram *diagram = m_conductor->diagram())
		if (!diagram->drawColoredConductors())
			stroke_color = Qt::black;
	QPen pen(stroke_color, qMax<qreal>(1, p.cond_size));
	pen.setCapStyle(Qt::FlatCap);
	painter->setPen(pen);
	painter->drawLine(QPointF(-half_stroke, half_stroke),
			  QPointF(half_stroke, -half_stroke));

	painter->setFont(m_conductor->textItem()->font());
	painter->setPen(m_conductor->textItem()->defaultTextColor());
	drawText(painter, colorRect(), p.m_wire_color);
	drawText(painter, sectionRect(), p.m_wire_section);

	if (m_hovered) {
		painter->setPen(QPen(QColor(0x60, 0x90, 0xd0), 0, Qt::DashLine));
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(boundingRect().adjusted(0.5, 0.5, -0.5, -0.5));
	}
	painter->restore();
}

void ConductorDefinitionMark::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() != Qt::LeftButton) {
		event->ignore();
		return;
	}
	m_distance_before_drag = m_distance;
	event->accept();
}

void ConductorDefinitionMark::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_distance_before_drag < 0)
		return;
	const QPointF at = m_conductor->mapFromScene(Diagram::snapToGrid(event->scenePos()));
	place(distanceOf(points(), at));
}

/**
	A drag ends as one undo step, which writes the new distance into the
	conductor's properties
*/
void ConductorDefinitionMark::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(event)
	const qreal before = m_distance_before_drag;
	m_distance_before_drag = -1;
	if (before < 0 || qFuzzyCompare(1 + before, 1 + m_distance))
		return;

	const ConductorProperties old_properties = m_conductor->properties();
	ConductorProperties new_properties = old_properties;
	new_properties.m_definition_mark_pos = qRound(m_distance);
	auto *undo = new QPropertyUndoCommand(m_conductor, "properties",
					      QVariant::fromValue(old_properties),
					      QVariant::fromValue(new_properties));
	undo->setText(tr("Move the colour and section"));
	if (Diagram *diagram = m_conductor->diagram())
		diagram->undoStack().push(undo);
	else {
		undo->redo();
		delete undo;
	}
}

void ConductorDefinitionMark::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	Q_UNUSED(event)
	m_hovered = true;
	update();
}

void ConductorDefinitionMark::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	Q_UNUSED(event)
	m_hovered = false;
	update();
}
