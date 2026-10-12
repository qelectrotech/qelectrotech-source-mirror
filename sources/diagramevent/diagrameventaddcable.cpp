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
#include "diagrameventaddcable.h"

#include "../cable/addcablecommand.h"
#include "../cable/cablecreatedialog.h"
#include "../cable/cablepart.h"
#include "../diagram.h"
#include "../qetapp.h"
#include "../qetdiagrameditor.h"
#include "../qetproject.h"

#include <QGraphicsLineItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QLineF>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QUndoStack>

namespace {

	///A click closer than this to the start point is not an end point
	const qreal min_end_length = 2.0;

/**
	@brief How far apart two grid points are, the same way the snapping
	itself works it out -- the direction the line may be locked to has to
	be judged against the same step the end point will land on.
*/
qreal gridStep()
{
	QSettings settings;
	const int x = settings.value(QStringLiteral("diagrameditor/Xgrid"),
								 Diagram::xGrid).toInt();
	return qMax(1, x);
}

} // namespace

/**
	@brief DiagramEventAddCable::DiagramEventAddCable
	@param diagram the folio the cable is drawn on
*/
DiagramEventAddCable::DiagramEventAddCable(Diagram *diagram) :
	DiagramEventInterface(diagram)
{
	m_running = true;
		//Done here as well as in init(): Diagram::setEventInterface()
		//only calls init() when it replaces another tool, so a fresh
		//tool would otherwise never be set up at all.
	init();
		//The message itself waits for the next turn of the event loop:
		//setEventInterface() destroys the tool which was running before
		//this constructor returns, and its destructor clears the status
		//bar, which would wipe out anything shown before it is done.
	QTimer::singleShot(0, this, [this]() { updateHint(); });
}

/**
	@brief DiagramEventAddCable::~DiagramEventAddCable
*/
DiagramEventAddCable::~DiagramEventAddCable()
{
	delete m_preview;
	delete m_help_horiz;
	delete m_help_verti;

	if (m_diagram)
	{
		for (QGraphicsView *view : m_diagram->views()) {
			view->setContextMenuPolicy(Qt::DefaultContextMenu);
		}
	}
	clearHint();
}

/**
	@brief DiagramEventAddCable::init
	While the tool runs, the right button belongs to it alone: it cancels
	the line being drawn or ends the tool. The quick command ring and the
	context menu of the folio would both pop up in the middle of the
	drawing and take the tool away from under the user, so they are kept
	out of the way until the tool is gone.
*/
void DiagramEventAddCable::init()
{
	m_running = true;
	if (m_diagram)
	{
		for (QGraphicsView *view : m_diagram->views()) {
			view->setContextMenuPolicy(Qt::NoContextMenu);
		}
	}
	updateHint();
}

/**
	@brief DiagramEventAddCable::reset
	Put the tool back to where it was before the first click: no line
	kept, no preview, nothing remembered. The cross stays, since it
	belongs to the tool rather than to one line.
*/
void DiagramEventAddCable::reset()
{
	if (m_preview && m_diagram) {
		m_diagram->removeItem(m_preview);
	}
	delete m_preview;
	m_preview = nullptr;
	m_has_anchor = false;
	m_anchor = QPointF();
	m_axis = -1;
	m_crossings.clear();
	updateHint();
}

/**
	@brief DiagramEventAddCable::clearHint
*/
void DiagramEventAddCable::clearHint() const
{
	if (!m_diagram || m_diagram->views().isEmpty()) return;
	if (auto *editor = QETApp::diagramEditorAncestorOf(m_diagram->views().constFirst())) {
		editor->statusBar()->clearMessage();
	}
}

/**
	@brief DiagramEventAddCable::updateHint
	Say what the next click does, and -- once the line has been pulled
	out -- how many conductors it runs across, so the user knows before
	the dialog opens what the cable is going to be given.
*/
void DiagramEventAddCable::updateHint() const
{
	if (!m_diagram || m_diagram->views().isEmpty()) return;
	auto *editor = QETApp::diagramEditorAncestorOf(m_diagram->views().constFirst());
	if (!editor) return;

	if (!m_has_anchor)
	{
		editor->statusBar()->showMessage(tr(
			"Left click: place the cable start; "
			"drag or second click: finish; "
			"right click: cancel; Esc: quit"));
		return;
	}

	editor->statusBar()->showMessage(tr(
		"Left click or release: finish the line "
		"(%n core(s) recognized; always horizontal or vertical); "
		"right click: cancel the line",
		"how many conductors the line being drawn already crosses",
		m_crossings.size()));
}

/**
	@brief DiagramEventAddCable::mousePressEvent
	@param event
*/
void DiagramEventAddCable::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (Q_UNLIKELY(!m_diagram || m_diagram->isReadOnly())) return;

	if (event->button() == Qt::RightButton)
	{
			//Accepted first: emitting finish() makes the diagram destroy
			//this very tool, so nothing may touch its members afterwards.
		event->setAccepted(true);

			//First right click takes the line being drawn back, second one
			//leaves the tool: the user always knows how far he has to press
			//to get out, and never lands in a menu he did not ask for.
		if (m_has_anchor) {
			reset();
		} else {
			m_running = false;
			emit finish();
		}
		return;
	}
	if (event->button() != Qt::LeftButton) return;

	const QPointF pos = Diagram::snapToGrid(event->scenePos());

	if (!m_has_anchor)
	{
		m_has_anchor = true;
		m_anchor = pos;
		m_axis = -1;
		updatePreview(pos);
		event->setAccepted(true);
		return;
	}

		//A second click ends the line wherever it lands: it is snapped
		//back onto whichever direction the line already runs along, so
		//the user does not have to aim at the line itself.
	const QPointF end = finishPoint(pos);
	if (QLineF(m_anchor, end).length() >= min_end_length) {
		finishLine(end);
	}
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddCable::mouseMoveEvent
	@param event
*/
void DiagramEventAddCable::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	showGuides(event->scenePos());
	if (!m_has_anchor) return;

	updatePreview(event->scenePos());
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddCable::mouseReleaseEvent
	Letting go of a drag which was pulled out far enough lays the line
	down straight away, which is how a line is drawn everywhere else in
	QElectroTech -- a click without a drag, on the other hand, only puts
	the start point down and waits for the next click.
	@param event
*/
void DiagramEventAddCable::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() != Qt::LeftButton || !m_has_anchor) return;

	const QPointF end = finishPoint(Diagram::snapToGrid(event->scenePos()));
	if (QLineF(m_anchor, end).length() >= min_end_length) {
		finishLine(end);
	}
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddCable::keyPressEvent
	@param event
*/
void DiagramEventAddCable::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape)
	{
		if (m_has_anchor)
		{
			reset();
			event->setAccepted(true);
			return;
		}
	}
	DiagramEventInterface::keyPressEvent(event);
}

/**
	@brief DiagramEventAddCable::finishPoint
	The point the line really ends at: snapped to the grid, and pulled
	onto the direction the line already runs along, so a second click
	which does not sit exactly on the axis still ends the line instead
	of starting a new one.
	@param scene_pos where the user clicked
	@return the end of the line, in scene coordinates
*/
QPointF DiagramEventAddCable::finishPoint(const QPointF &scene_pos) const
{
	if (m_axis == 0) {
		return Diagram::snapToGrid(QPointF(scene_pos.x(), m_anchor.y()));
	}
	if (m_axis == 1) {
		return Diagram::snapToGrid(QPointF(m_anchor.x(), scene_pos.y()));
	}

		//The line was never pulled far enough to have a direction: take
		//the one which is clearly longer, so it never ends up diagonal.
	const QPointF delta = scene_pos - m_anchor;
	if (qAbs(delta.x()) >= qAbs(delta.y())) {
		return Diagram::snapToGrid(QPointF(scene_pos.x(), m_anchor.y()));
	}
	return Diagram::snapToGrid(QPointF(m_anchor.x(), scene_pos.y()));
}

/**
	@brief DiagramEventAddCable::updatePreview
	Show the line as it will be laid down.

	The direction is decided again on every move rather than once and
	for all: the user pulls, and the line follows whichever way he pulls
	harder. A dead band of half a grid step keeps it from flickering
	when he moves along the diagonal.
	@param scene_pos where the mouse stands
*/
void DiagramEventAddCable::updatePreview(const QPointF &scene_pos)
{
	if (!m_has_anchor || !m_diagram) return;

	QPointF end = scene_pos;
	{
		const QPointF delta = end - m_anchor;
		const qreal dx = qAbs(delta.x());
		const qreal dy = qAbs(delta.y());
		const qreal dead = gridStep() * 0.5;

		int wanted = -1;
		if (dx > dy + dead) {
			wanted = 0;
		} else if (dy > dx + dead) {
			wanted = 1;
		} else if (m_axis < 0 && (dx > dead || dy > dead)) {
			wanted = dx >= dy ? 0 : 1;
		}
		if (wanted >= 0) {
			m_axis = wanted;
		}
	}

	if (m_axis == 0) {
		end.setY(m_anchor.y());
	} else if (m_axis == 1) {
		end.setX(m_anchor.x());
	}
	end = Diagram::snapToGrid(end);

	if (!m_preview)
	{
		m_preview = new QGraphicsLineItem;
		QPen pen(QColor(0, 120, 215));
		pen.setWidthF(2.0);
		pen.setStyle(Qt::DashLine);
		m_preview->setPen(pen);
		m_preview->setZValue(1000.0);
		m_diagram->addItem(m_preview);
	}
	m_preview->setLine(QLineF(m_anchor, end));

		//Keep the count of crossed conductors up to date, so the status
		//bar already says what the dialog is about to say.
	m_crossings = CableManager::crossedConductors(m_diagram, QLineF(m_anchor, end));
	updateHint();
}

/**
	@brief DiagramEventAddCable::showGuides
	The thin cross QElectroTech draws while any shape is drawn, which
	says where the mouse stands in the folio and, for a line, along
	which of the two directions it may run.
	@param scene_pos where the mouse stands
*/
void DiagramEventAddCable::showGuides(const QPointF &scene_pos)
{
	if (!m_diagram) return;

	if (!m_help_horiz || !m_help_verti)
	{
		QPen pen;
		pen.setWidthF(0.4);
		pen.setCosmetic(true);
		pen.setColor(Diagram::background_color == Qt::darkGray ? Qt::lightGray : Qt::darkGray);

		const QRectF rect = m_diagram->border_and_titleblock.insideBorderRect();

		if (!m_help_horiz)
		{
			m_help_horiz = new QGraphicsLineItem(rect.topLeft().x(), 0, rect.topRight().x(), 0);
			m_help_horiz->setPen(pen);
			m_help_horiz->setZValue(999.0);
			m_diagram->addItem(m_help_horiz);
		}
		if (!m_help_verti)
		{
			m_help_verti = new QGraphicsLineItem(0, rect.topLeft().y(), 0, rect.bottomLeft().y());
			m_help_verti->setPen(pen);
			m_help_verti->setZValue(999.0);
			m_diagram->addItem(m_help_verti);
		}
	}

	const QPointF point = Diagram::snapToGrid(scene_pos);
	m_help_horiz->setY(point.y());
	m_help_verti->setX(point.x());
}

/**
	@brief DiagramEventAddCable::finishLine
	Put the line down and hand it over to the dialog which turns it into
	a cable -- a new one from the cable type file, or one the project
	already holds.
	@param end where the line stops
*/
void DiagramEventAddCable::finishLine(const QPointF &end)
{
	if (!m_diagram || !m_diagram->project()) {
		reset();
		return;
	}

	QPointF to = end;
	if (m_axis == 0) {
		to.setY(m_anchor.y());
	} else if (m_axis == 1) {
		to.setX(m_anchor.x());
	}
	to = Diagram::snapToGrid(to);

	const QLineF line(m_anchor, to);
	if (line.length() < min_end_length) {
		reset();
		return;
	}

	m_crossings = CableManager::crossedConductors(m_diagram, line);

	CablePartData part;
	part.uuid = QUuid::createUuid();
	part.diagram = m_diagram->uuid();
	part.p1 = line.p1();
	part.p2 = line.p2();

		//Remember which cable the line was given to: a brand new one was
		//brought into being by this very dialog and undo has to take it
		//away again, an existing one only gains a section.
	const QVector<Cable *> before = m_diagram->project()->cables();

	{
		CableCreateDialog dialog(m_diagram, m_crossings, part, m_diagram->views().isEmpty()
								 ? nullptr : m_diagram->views().constFirst());
		if (dialog.exec() != QDialog::Accepted || !dialog.cable())
		{
			reset();
			return;
		}

		const bool own_cable = !before.contains(dialog.cable());

		auto *item = new CablePart(part);
		item->setCable(dialog.cable());
		m_diagram->addItem(item);

		m_diagram->project()->undoStack()->push(
			new AddCableCommand(dialog.cable(), item, m_diagram, own_cable,
								dialog.takenCores()));
	}

	reset();
}
