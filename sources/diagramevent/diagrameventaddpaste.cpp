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
#include "diagrameventaddpaste.h"

#include "../diagram.h"
#include "../diagramcommands.h"
#include "../qetapp.h"
#include "../qetdiagrameditor.h"
#include "../qetgraphicsitem/conductor.h"

#include <QSettings>

#include <QApplication>
#include <QClipboard>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QStatusBar>

/**
	@brief DiagramEventAddPaste::DiagramEventAddPaste
	@param diagram : diagram to paste into
	@param start_pos : where the pasted items first appear, in scene
	coordinates -- normally the cursor
*/
	DiagramEventAddPaste::DiagramEventAddPaste(Diagram *diagram, const QPointF &start_pos) :
	DiagramEventInterface(diagram)
{
		//DiagramEventInterface::init() is called by Diagram::setEventInterface
		//only when it is replacing an earlier interface, so call it here as
		//DiagramEventAddMacro does.
	init();

	const QString clipboard_text = QApplication::clipboard()->text();
	if (clipboard_text.isEmpty()) return;

	QDomDocument document_xml;
	if (!document_xml.setContent(clipboard_text)) return;

		//Load items at their original XML coordinates.
	m_diagram->fromXml(document_xml, QPointF(), false, &m_content);
	if (!m_content.count()) return;

	const QList<QGraphicsItem *> movable = m_content.items(MovableItems);
	if (movable.isEmpty()) return;

		//Compute the top-left of all items' positions (not bounding
		//rects) and snap to grid: this is the point that gets placed
		//under the cursor, and the baseline moveTo() measures from.
	QPointF top_left;
	bool first = true;
	for (auto *item : movable) {
		const QPointF p = item->pos();
		if (first) {
			top_left = p;
			first = false;
		} else {
			if (p.x() < top_left.x()) top_left.setX(p.x());
			if (p.y() < top_left.y()) top_left.setY(p.y());
		}
	}
	QSettings settings;
	const int xGrid = settings.value(QStringLiteral("diagrameditor/Xgrid"),
					  Diagram::xGrid).toInt();
	const int yGrid = settings.value(QStringLiteral("diagrameditor/Ygrid"),
					  Diagram::yGrid).toInt();
	const auto snapGrid = [xGrid, yGrid](const QPointF &p) -> QPointF {
		return QPointF(
			qRound(p.x() / xGrid) * xGrid,
			qRound(p.y() / yGrid) * yGrid);
	};
	const QPointF grid_origin = snapGrid(top_left);

		//Move the group to the cursor, rather than the cursor to the
		//group. Both put the copy under the pointer, but warping the
		//pointer also drags it back to the original's position, so the
		//copy appears exactly on top of what was copied until the mouse
		//is moved -- which is the thing pasting under the cursor was
		//meant to avoid (issue #913). Taking the pointer away from
		//where the user put it is also its own surprise.
	m_group_origin = snapGrid(start_pos);
	const QPointF offset = m_group_origin - grid_origin;

		//Store each item's position after the move. moveTo() applies a
		//grid-snapped delta from the baseline to these, so items
		//preserve their layout and move in whole grid steps.
	for (auto *item : movable) {
		item->setPos(item->pos() + offset);
		m_relative_pos.insert(item, item->pos());
	}

		//The conductors were laid out against the old terminal
		//positions, so re-route them before anything is drawn.
	const QList<Conductor *> conductors = m_content.conductors(DiagramContent::AnyConductor);
	for (auto *conductor : conductors) {
		conductor->updatePath();
	}

		//The baseline is known now, so moveTo() does not have to
		//capture one from the first mouse movement.
	m_initial_cursor = m_group_origin;
	m_baseline_captured = true;

	m_diagram->clearSelection();
	for (auto *item : movable) {
		item->setSelected(true);
	}

	if (!m_diagram->views().isEmpty()) {
		if (auto *view = m_diagram->views().at(0)) {
			if (const auto qde = QETApp::diagramEditorAncestorOf(view)) {
				m_status_bar = qde->statusBar();
			}
		}
	}
	showHint();

	m_running = true;
}

/**
	@brief DiagramEventAddPaste::~DiagramEventAddPaste
	If the placement never finished -- the editor closed, or another tool took
	over -- the items are still on the folio with nothing on the undo stack to
	account for them, so take them away.
*/
DiagramEventAddPaste::~DiagramEventAddPaste()
{
	if (!m_finished && m_diagram) {
		removeItems();
		m_finished = true;
		m_running = false;
	}
	if (m_status_bar) {
		m_status_bar->clearMessage();
	}

		//Give the context menu back. init() turned it off so a right
		//click would cancel the placement instead of opening a menu over
		//it, and nothing turned it on again: one Ctrl+V left the folio's
		//right-click menu dead for the rest of the session, taking
		//"Coller ici", "Collage multiple" and the folio properties with
		//it. Every other DiagramEvent* class restores it here; this one
		//did not.
	if (m_diagram) {
		const auto views = m_diagram->views();
		for (auto *view : views) {
			view->setContextMenuPolicy(Qt::DefaultContextMenu);
		}
	}
}

/**
	@brief DiagramEventAddPaste::clipboardHasDiagram
	@return true if the clipboard holds a diagram fragment
*/
bool DiagramEventAddPaste::clipboardHasDiagram()
{
	return Diagram::clipboardMayContainDiagram();
}

/**
	@brief DiagramEventAddPaste::init
	Suppress the context menu while placing, so a right click can cancel
	instead of opening a menu over the items being positioned.
*/
void DiagramEventAddPaste::init()
{
	if (!m_diagram) return;
	const auto views = m_diagram->views();
	for (auto *view : views) {
		view->setContextMenuPolicy(Qt::NoContextMenu);
	}
}

void DiagramEventAddPaste::showHint()
{
	if (m_status_bar) {
		m_status_bar->showMessage(
			tr("Cliquez pour poser le collage, Échap ou clic droit pour annuler",
			   "status bar tip while positioning a paste"));
	}
}

/**
	@brief DiagramEventAddPaste::moveTo
	Compute a grid-snapped delta from the initial cursor position and
	apply it to every item's stored position.  Working from a delta
	against a fixed baseline, rather than from the previous position,
	keeps all items exactly on grid points regardless of modifier keys
	and stops rounding accumulating over a long drag.
*/
void DiagramEventAddPaste::moveTo(const QPointF &scene_pos)
{
	QSettings settings;
	const int xGrid = settings.value(QStringLiteral("diagrameditor/Xgrid"),
					  Diagram::xGrid).toInt();
	const int yGrid = settings.value(QStringLiteral("diagrameditor/Ygrid"),
					  Diagram::yGrid).toInt();

	const auto snapGrid = [xGrid, yGrid](const QPointF &p) -> QPointF {
		return QPointF(
			qRound(p.x() / xGrid) * xGrid,
			qRound(p.y() / yGrid) * yGrid);
	};

		//The constructor normally sets the baseline, having just put the
		//group there. This covers the case where it could not -- no view
		//to map through -- by taking the first cursor position instead.
		//Tested with m_baseline_captured rather than
		//m_initial_cursor.isNull(), which silently re-baselines when the
		//baseline is legitimately scene (0,0).
	if (!m_baseline_captured) {
		m_initial_cursor = snapGrid(scene_pos);
		m_baseline_captured = true;
		return;
	}

	const QPointF delta = snapGrid(scene_pos) - m_initial_cursor;

	for (auto it = m_relative_pos.constBegin() ; it != m_relative_pos.constEnd() ; ++it) {
		if (it.key()) {
			it.key()->setPos(it.value() + delta);
		}
	}

		//Update conductor paths so they follow the moved terminals.
	const QList<Conductor *> conductors = m_content.conductors(DiagramContent::AnyConductor);
	for (auto *conductor : conductors) {
		conductor->updatePath();
	}
}

void DiagramEventAddPaste::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_running) return;
	moveTo(event->scenePos());
	event->setAccepted(true);
}

void DiagramEventAddPaste::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_running) return;
		//Swallowed so the press cannot start a rubber band or drag an item
		//out of the group; the release is what decides.
	event->setAccepted(true);
}

void DiagramEventAddPaste::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_running) return;

	event->setAccepted(true);
	if (event->button() == Qt::LeftButton) {
		commit();
	} else if (event->button() == Qt::RightButton) {
		cancel();
	}
}

void DiagramEventAddPaste::keyPressEvent(QKeyEvent *event)
{
	if (!m_running) return;

	switch (event->key()) {
		case Qt::Key_Escape:
			event->setAccepted(true);
			cancel();
			break;
			//Return and Enter drop the paste where it stands, so the whole
			//operation can be completed without a mouse.
		case Qt::Key_Return:
		case Qt::Key_Enter:
			event->setAccepted(true);
			commit();
			break;
		default:
			break;
	}
}

/**
	@brief DiagramEventAddPaste::commit
	Hand the items to the undo stack where they stand.

	PasteDiagramCommand's first redo() does not add the items to the scene --
	it assumes they are already there, which is what Diagram::fromXml did when
	this started. So pushing it here adopts them rather than duplicating them.
*/
void DiagramEventAddPaste::commit()
{
	if (m_finished || !m_diagram) return;
	m_finished = true;
	m_running = false;

	m_diagram->undoStack().push(new PasteDiagramCommand(m_diagram, m_content));
	emit finish();
}

/**
	@brief DiagramEventAddPaste::cancel
	Take the items back off the folio. Nothing was pushed to the undo stack,
	so there is nothing to undo afterwards.
*/
void DiagramEventAddPaste::cancel()
{
	if (m_finished || !m_diagram) return;
	removeItems();
	m_finished = true;
	m_running = false;
	emit finish();  // only the user-driven path signals
}

void DiagramEventAddPaste::removeItems()
{
	const QList<Conductor *> conductors = m_content.conductors(DiagramContent::AnyConductor);
	for (auto *conductor : conductors) {
		m_diagram->removeItem(conductor);
		delete conductor;
	}

	const QList<QGraphicsItem *> rest = m_content.items(MovableItems);
	for (auto *item : rest) {
		m_diagram->removeItem(item);
		delete item;
	}

	m_content.clear();
	m_relative_pos.clear();
}
