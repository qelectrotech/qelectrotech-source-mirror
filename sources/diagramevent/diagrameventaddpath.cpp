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
#include "diagrameventaddpath.h"

#include "../diagram.h"
#include "../lastusedstyle.h"
#include "../qetapp.h"
#include "../qetdiagrameditor.h"
#include "../undocommand/addgraphicsobjectcommand.h"

#include <QGraphicsLineItem>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QLineF>
#include <QStatusBar>
#include <QTimer>

/**
	@brief DiagramEventAddPath::DiagramEventAddPath
	@param diagram : the diagram where this event must operate
*/
DiagramEventAddPath::DiagramEventAddPath(Diagram *diagram) :
	DiagramEventInterface(diagram),
	m_shape_item (nullptr),
	m_help_horiz (nullptr),
	m_help_verti (nullptr)
{
	m_running = true;
	init();
	// Deferred for the same reason as DiagramEventAddShape's own
	// constructor-time hint: Diagram::setEventInterface() destroys
	// whatever tool was previously active *after* this constructor
	// returns, and that tool's own destructor clears the status bar --
	// an immediate show here would just get wiped out moments later.
	QTimer::singleShot(0, this, [this]() { showHint(); });
}

DiagramEventAddPath::~DiagramEventAddPath()
{
	if ((m_running || m_abort) && m_shape_item)
	{
		m_diagram->removeItem(m_shape_item);
		delete m_shape_item;
	}
	delete m_help_horiz;
	delete m_help_verti;

	if (m_diagram && !m_diagram->views().isEmpty())
	{
		if (auto *editor = QETApp::diagramEditorAncestorOf(m_diagram->views().constFirst()))
			editor->statusBar()->clearMessage();
	}

	foreach (QGraphicsView *v, m_diagram->views())
		v->setContextMenuPolicy(Qt::DefaultContextMenu);
}

/**
	@brief DiagramEventAddPath::showHint
	Re-asserted on every move within the canvas (see mouseMoveEvent), not
	just once at activation: Qt's own built-in "show an action's
	statusTip on hover" has its own internal "restore whatever was there
	before" logic for when the hover ends. Since this message is first
	shown *during* that same hover session (the user is still over the
	toolbar icon when the deferred constructor-time call above fires),
	Qt's hover-tracking has no idea this code changed the status bar in
	the meantime -- the moment the mouse leaves the icon for the canvas,
	it silently restores whatever it remembers being there before its
	own tip started, overwriting this one. Re-showing it on every move
	within the canvas simply outlasts that one-time restore.
*/
void DiagramEventAddPath::showHint() const
{
	if (!m_diagram || m_diagram->views().isEmpty())
		return;
	if (auto *editor = QETApp::diagramEditorAncestorOf(m_diagram->views().constFirst()))
		editor->statusBar()->showMessage(tr("Clic: point anguleux. Cliquer-glisser: point courbe. "
		                                     "Clic sur le premier point: fermer. Échap/Entrée: terminer. "
		                                     "Clic droit: annuler le dernier point."));
}

void DiagramEventAddPath::init()
{
	foreach (QGraphicsView *v, m_diagram->views())
		v->setContextMenuPolicy(Qt::NoContextMenu);
}

QPointF DiagramEventAddPath::snapped(const QPointF &scenePos, Qt::KeyboardModifiers mods) const
{
	return mods == Qt::ControlModifier ? scenePos : Diagram::snapToGrid(scenePos);
}

int DiagramEventAddPath::confirmedNodeCount() const
{
	// The trailing element is always the live preview while m_shape_item
	// exists; with no shape yet there are no nodes of any kind.
	return m_shape_item ? qMax(0, m_nodes.size() - 1) : 0;
}

/**
	@brief DiagramEventAddPath::mousePressEvent
	Left click: on the very first click, creates the shape with a real
	node *and* an immediate preview node at the same spot, so a segment
	exists (even if zero-length) from the start rather than requiring a
	second click before anything is visible. On later clicks: either
	confirms the live preview into a real point and appends a fresh one
	for the next segment, or -- if close enough to the first node --
	closes the path.
*/
void DiagramEventAddPath::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (Q_UNLIKELY(m_diagram->isReadOnly()))
		return;

	if (event->button() != Qt::LeftButton)
	{
		// Accept every button while this tool is running, not just the
		// one it actually acts on -- Diagram::mousePressEvent falls
		// through to Qt's own default scene handling for anything left
		// unaccepted, which is exactly the kind of competing control
		// this tool can't afford while it's supposed to have exclusive
		// ownership of input.
		event->setAccepted(true);
		return;
	}

	const QPointF pos = snapped(event->scenePos(), event->modifiers());

	if (!m_shape_item)
	{
		m_shape_item = new QetShapeItem(pos, pos, QetShapeItem::Path);
		if (LastUsedStyle::hasShapePen())
			m_shape_item->setPen(LastUsedStyle::shapePen());
		if (LastUsedStyle::hasShapeBrush())
			m_shape_item->setBrush(LastUsedStyle::shapeBrush());
		m_diagram->addItem(m_shape_item);
		// Handles only ever get built for a selected item.
		m_shape_item->setSelected(true);

		QetShapeItem::PathNode node;
		node.anchor = pos;
		m_nodes << node;
		m_nodes << node;   // live preview, tracks the mouse from here on
		m_shape_item->setPathNodes(m_nodes);
		m_shape_item->enableNodeEditMode();

		m_dragging_node = 0;
		event->setAccepted(true);
		return;
	}

	if (confirmedNodeCount() >= 2 && nearFirstNode(pos))
	{
		finishPath(true);
		event->setAccepted(true);
		return;
	}

	// Confirm the preview node as a real point, then append a fresh
	// preview (a plain Corner, not a copy of the just-confirmed node's
	// kind/handles) for the segment after it.
	m_dragging_node = m_nodes.size() - 1;
	m_nodes[m_dragging_node].anchor = pos;

	QetShapeItem::PathNode preview;
	preview.anchor = pos;
	m_nodes << preview;

	m_shape_item->setPathNodes(m_nodes);
	m_shape_item->enableNodeEditMode();
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddPath::mouseMoveEvent
	Two mutually exclusive behaviours, matching whether a button is held:
	with the left button down on a just-placed node, dragging shapes that
	node's handles (same convention as editing an existing node -- see
	QetShapeItem::dragPathControlHandle()). With no button held, the
	trailing preview node instead tracks the mouse, giving the live
	rubber-band segment.
*/
void DiagramEventAddPath::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	updateHelpCross(event->scenePos());
	showHint();

	if (m_shape_item)
	{
		const QPointF pos = snapped(event->scenePos(), event->modifiers());

		if (m_dragging_node >= 0 && (event->buttons() & Qt::LeftButton))
		{
			QetShapeItem::PathNode &node = m_nodes[m_dragging_node];
			const QPointF delta = pos - node.anchor;

			// A small threshold so an accidental few-pixel wobble on
			// what was meant to be a plain click doesn't silently add
			// curve handles the user never intended.
			if (QLineF(QPointF(), delta).length() > 3.0)
			{
				node.kind = QetShapeItem::NodeKind::Smooth;
				node.outHandle = delta;
				node.inHandle  = -delta;
			}
			else
			{
				node.kind = QetShapeItem::NodeKind::Corner;
				node.outHandle.reset();
				node.inHandle.reset();
			}
			m_shape_item->setPathNodes(m_nodes);
		}
		else if (!(event->buttons() & Qt::LeftButton) && !m_nodes.isEmpty())
		{
			m_nodes.last().anchor = pos;
			m_shape_item->setPathNodes(m_nodes);
		}
	}

	// Ours unconditionally while running: a stray, unaccepted move event
	// falling through to Qt's default handling risks it dragging our
	// selected, movable in-progress shape out from under the tool.
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddPath::mouseReleaseEvent
	Left release just ends the current node's drag phase (the trailing
	preview resumes tracking the mouse on the next move). Right release
	steps back one *confirmed* point (the preview is left alone), or
	cancels outright once only one remains, or exits the tool entirely if
	nothing is in progress at all.
*/
void DiagramEventAddPath::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_dragging_node = -1;
	}
	else if (event->button() == Qt::RightButton)
	{
		if (m_shape_item)
		{
			if (confirmedNodeCount() > 1)
			{
				m_nodes.remove(m_nodes.size() - 2);   // the last *confirmed* node; keep the trailing preview
				m_shape_item->setPathNodes(m_nodes);
			}
			else
			{
				cancelPath();
			}
		}
		else
		{
			m_running = false;
			emit finish();
		}
	}
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddPath::mouseDoubleClickEvent
	A double-click is a press, release, press, release, doubleclick
	sequence -- the second press already confirmed the preview into a
	duplicate point (mousePressEvent can't distinguish a double-click
	from two single clicks in the same place) and appended a fresh
	preview after it. Dropping the last node here removes that fresh
	preview; finishPath()'s own trailing-preview removal then removes the
	duplicate underneath it, leaving only the genuinely-placed points.
*/
void DiagramEventAddPath::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_shape_item && event->button() == Qt::LeftButton && !m_nodes.isEmpty())
	{
		m_nodes.removeLast();
		finishPath(false);
	}
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddPath::keyPressEvent
	Escape or Enter finish the path open once at least two real points
	exist; Escape with fewer (or none placed at all) cancels/exits
	instead, since there's nothing meaningful to keep.
*/
void DiagramEventAddPath::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape)
	{
		if (m_shape_item && confirmedNodeCount() >= 2)
			finishPath(false);
		else if (m_shape_item)
			cancelPath();
		else
		{
			m_running = false;
			emit finish();
		}
		event->accept();
	}
	else if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
			&& m_shape_item && confirmedNodeCount() >= 2)
	{
		finishPath(false);
		event->accept();
	}
}

/**
	@brief DiagramEventAddPath::finishPath
	Strips the trailing live-preview node, commits the in-progress path
	onto the undo stack, and resets so the tool is ready to draw another
	one immediately -- matching every other shape tool's own behaviour
	after finishing a shape.
*/
void DiagramEventAddPath::finishPath(bool closed)
{
	if (!m_shape_item)
		return;

	if (!m_nodes.isEmpty())
		m_nodes.removeLast();

	if (m_nodes.size() < 2)
	{
		cancelPath();
		return;
	}

	if (closed)
		m_shape_item->setClosed(true);

	m_shape_item->setPathNodes(m_nodes);
	m_diagram->undoStack().push(new AddGraphicsObjectCommand(m_shape_item, m_diagram));
	m_shape_item = nullptr;
	m_nodes.clear();
	m_dragging_node = -1;
}

/**
	@brief DiagramEventAddPath::cancelPath
	Discards the in-progress path entirely -- nothing worth keeping (an
	empty or single-point path isn't a usable shape).
*/
void DiagramEventAddPath::cancelPath()
{
	if (m_shape_item)
	{
		m_diagram->removeItem(m_shape_item);
		delete m_shape_item;
		m_shape_item = nullptr;
	}
	m_nodes.clear();
	m_dragging_node = -1;
}

/**
	@brief DiagramEventAddPath::nearFirstNode
	m_shape_item's pos()/transform() stay at their identity defaults for
	its entire construction here -- nothing during drawing ever touches
	them -- so the first node's anchor, stored in local coordinates, is
	directly comparable to a scene position without any mapping.
*/
bool DiagramEventAddPath::nearFirstNode(const QPointF &scenePos) const
{
	if (m_nodes.isEmpty())
		return false;
	return QLineF(m_nodes.first().anchor, scenePos).length() <= CLOSE_THRESHOLD;
}

/**
	@brief DiagramEventAddPath::updateHelpCross
	Same crosshair guide as every other shape tool (see
	DiagramEventAddShape::updateHelpCross) -- duplicated rather than
	shared, since the two classes don't otherwise share a common base
	beyond DiagramEventInterface.
*/
void DiagramEventAddPath::updateHelpCross(const QPointF &p)
{
	if (!m_help_horiz || !m_help_verti)
	{
		QPen pen;
		pen.setWidthF(0.4);
		pen.setCosmetic(true);
		pen.setColor(Diagram::background_color == Qt::darkGray ? Qt::lightGray : Qt::darkGray);

		QRectF rect = m_diagram->border_and_titleblock.insideBorderRect();

		if (!m_help_horiz)
		{
			m_help_horiz = new QGraphicsLineItem(rect.topLeft().x(), 0, rect.topRight().x(), 0);
			m_help_horiz->setPen(pen);
			m_diagram->addItem(m_help_horiz);
		}

		if (!m_help_verti)
		{
			m_help_verti = new QGraphicsLineItem(0, rect.topLeft().y(), 0, rect.bottomLeft().y());
			m_help_verti->setPen(pen);
			m_diagram->addItem(m_help_verti);
		}
	}

	QPointF point = Diagram::snapToGrid(p);

	m_help_horiz->setY(point.y());
	m_help_verti->setX(point.x());
}
