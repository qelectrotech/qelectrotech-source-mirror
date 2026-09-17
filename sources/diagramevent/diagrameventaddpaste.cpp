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

	m_diagram->fromXml(document_xml, Diagram::snapToGrid(start_pos), false, &m_content);
	if (!m_content.count()) return;

		//Remember where each item sits relative to the group's top left, so a
		//move is one assignment per item rather than an accumulated delta.
	QRectF group_rect;
	const QList<QGraphicsItem *> movable = m_content.items(MovableItems);
	for (auto *item : movable) {
		group_rect = group_rect.united(item->mapToScene(item->boundingRect()).boundingRect());
	}
	const QPointF top_left = group_rect.topLeft();
	for (auto *item : movable) {
		m_relative_pos.insert(item, item->pos() - top_left);
	}

	m_diagram->clearSelection();
	for (auto *item : movable) {
		item->setSelected(true);
	}

	if (!m_diagram->views().isEmpty()) {
		if (const auto qde = QETApp::diagramEditorAncestorOf(m_diagram->views().at(0))) {
			m_status_bar = qde->statusBar();
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
	if (!m_finished) {
		removeItems();
		m_finished = true;
		m_running = false;
	}
	if (m_status_bar) m_status_bar->clearMessage();
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
	Put the group's top left corner at @a scene_pos, snapped to the grid.
*/
void DiagramEventAddPaste::moveTo(const QPointF &scene_pos)
{
	const QPointF anchor = Diagram::snapToGrid(scene_pos);
	for (auto it = m_relative_pos.constBegin() ; it != m_relative_pos.constEnd() ; ++it) {
		if (it.key()) {
			it.key()->setPos(anchor + it.value());
		}
	}
	const auto conductors = m_content.conductors();  // AnyConductor by default
	for (auto *cond : conductors) {
		cond->updatePath();
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

	if (event->button() == Qt::LeftButton) {
		moveTo(event->scenePos());
		commit();
	} else if (event->button() == Qt::RightButton) {
		cancel();
	}
	event->setAccepted(true);
}

void DiagramEventAddPaste::keyPressEvent(QKeyEvent *event)
{
	if (!m_running) return;

	switch (event->key()) {
		case Qt::Key_Escape:
			cancel();
			event->setAccepted(true);
			break;
			//Return and Enter drop the paste where it stands, so the whole
			//operation can be completed without a mouse.
		case Qt::Key_Return:
		case Qt::Key_Enter:
			commit();
			event->setAccepted(true);
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
