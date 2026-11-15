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
#include <autoNum/ui/pastenumberingimport.h>

#include <cable/addcablecommand.h>
#include <cable/cablecopy.h>
#include <cable/cablepart.h>
#include <cable/editcablecommand.h>
#include <diagram.h>
#include <foliogrid.h>
#include <diagramcommands.h>
#include <qetapp.h>
#include <qetdiagrameditor.h>
#include <qetgraphicsitem/conductor.h>
#include <bridge/bridge.h>
#include <bridge/bridgecopy.h>
#include <bridge/bridgepart.h>
#include <qetproject.h>
#include <shortcutmanager.h>

#include <QSettings>

#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QCursor>
#include <QGraphicsLineItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QStatusBar>

#include <utility>

/**
	@brief DiagramEventAddPaste::DiagramEventAddPaste
	@param diagram : diagram to paste into
	@param start_pos : where the pasted items first appear, in scene
	coordinates -- normally the cursor. Only used for UnderCursor; an
	AtOrigin paste stays at the coordinates it was copied from.
	@param placement : UnderCursor moves the group under the cursor
	(default), AtOrigin leaves it at its original XML position and warps
	the OS cursor to the group's origin so baseline and screen match.
	@param fragment : the diagram fragment to place, as Diagram::toXml()
	writes it; the clipboard's when empty
*/
	DiagramEventAddPaste::DiagramEventAddPaste(Diagram *diagram, const QPointF &start_pos,
						   PastePlacement placement, const QString &fragment) :
	DiagramEventInterface(diagram)
{
	m_placement = placement;

		//DiagramEventInterface::init() is called by Diagram::setEventInterface
		//only when it is replacing an earlier interface, so call it here as
		//DiagramEventAddMacro does.
	init();

	const QString clipboard_text = fragment.isEmpty() ? QApplication::clipboard()->text()
													  : fragment;
	if (clipboard_text.isEmpty()) return;

	QDomDocument document_xml;
	if (!document_xml.setContent(clipboard_text)) return;

		//Batch the database work the same way project loading does
		//(QETProject::readProjectXml): without this, every addItem()
		//below emits dataBaseUpdated(), which makes each connected
		//table model re-run its full SQL query -- ~77 queries for a
		//typical paste, i.e. the multi-second stall on Ctrl+V.
	auto *db = m_diagram->project() ? m_diagram->project()->dataBase() : nullptr;
	if (db) {
		db->blockSignals(true);
		db->setUpdateBlocked(true);
	}

		//Load items at their original XML coordinates. A cable line
		//comes back through its own list rather than through the
		//content: the cable owns it, so it is not one of the folio's
		//items -- and it travels as a whole, colour labels and all,
		//whether or not the place it is going to holds a wire.
	m_diagram->fromXml(document_xml, QPointF(), false, &m_content, &m_cables,
						   &m_bridges);
	m_copied_schemes = PasteNumberingImport::copiedBy(document_xml);

	if (db) {
		db->blockSignals(false);
		db->setUpdateBlocked(false);
		db->updateDB();
	}
	if (!m_content.count() && m_cables.isEmpty() && m_bridges.isEmpty()) return;

	QList<QGraphicsItem *> movable = m_content.items(MovableItems);
	for (CablePart *part : std::as_const(m_cables)) {
		if (part) {
			movable << part;
			m_step_items.insert(part);
		}
	}
			//A bridge line folds a setPos() into its own geometry and
		//keeps itself at the origin the very way a cable line does
		//(BridgePart::itemChange): it is given a step to take with
		//them rather than a place to stand.
	for (Bridge *bridge : std::as_const(m_bridges)) {
		for (BridgePart *part : BridgePart::allParts()) {
			if (part && part->bridge() == bridge) {
				movable << part;
				m_step_items.insert(part);
			}
		}
	}
	if (movable.isEmpty()) return;

		//Compute the top-left of all items' actual on-screen bounding
		//boxes (not their raw pos()) and snap to grid: this is the point
		//that gets placed under the cursor, and the baseline moveTo()
		//measures from. mapToScene(boundingRect()) matters here, not
		//pos() alone: pos() is the scene location of an item's local
		//origin, but for anything with a pivot-centered transform (a
		//scaled or rotated image, in particular) that origin can sit far
		//from where the item is actually drawn -- pivot + scale*(0 -
		//pivot) is nowhere near (0, 0) once scale is well under 1. Using
		//pos() here silently pasted content at the right *delta* from a
		//point that wasn't actually where the content visually was,
		//producing a constant, scale-dependent offset between the cursor
		//and the pasted picture. Diagram::fromXml()'s own position
		//parameter already gets this right the same way, for the same
		//reason.
	QRectF items_rect;
	for (auto *item : movable) {
		items_rect = items_rect.united(item->mapToScene(item->boundingRect()).boundingRect());
	}
	const QPointF top_left = items_rect.topLeft();
	QSettings settings;
	const int xGrid = FolioGrid::step(settings.value(FolioGrid::x_key), Diagram::xGrid);
	const int yGrid = FolioGrid::step(settings.value(FolioGrid::y_key), Diagram::yGrid);
	const auto snapGrid = [xGrid, yGrid](const QPointF &p) -> QPointF {
		return QPointF(
			qRound(p.x() / xGrid) * xGrid,
			qRound(p.y() / yGrid) * yGrid);
	};
	m_original_pos = snapGrid(top_left);
	const QPointF grid_origin = (m_placement != UnderCursor)
			? m_original_pos
			: snapGrid(start_pos);
	m_current_pos = grid_origin;

	if (m_placement == UnderCursor) {
			//Land the pasted content under the cursor immediately, rather than
			//leaving it at the copied source's own coordinates: fromXml() above
			//loads items at their original position purely because it doesn't
			//know the target yet, not because that is where a paste should end
			//up. The previous approach instead left items there and warped the
			//OS cursor to match -- QCursor::setPos() is silently ignored by
			//many window managers and compositors (Wayland in particular), so
			//on any of those the warp simply never happened and the paste was
			//left wherever it had originally been copied from, which could be
			//anywhere on the folio -- exactly the "far from the cursor" bug.
		const QPointF initial_delta = grid_origin - snapGrid(top_left);
		for (auto *item : movable) {
			item->setPos(item->pos() + initial_delta);
		}
	} else {
			//AtOrigin: the items keep the coordinates they were copied from,
			//so the copy stands exactly where the original stands. No delta is
			//applied; what has to move is the cursor -- it is warped to the
			//group's grid-snapped origin below, once m_group_origin is set,
			//so the baseline and the physical cursor position agree.
	}

		//Store each item's now-placed position.  moveTo() applies a
		//grid-snapped delta from the baseline to these, so items
		//preserve their layout and move in whole grid steps.
	for (auto *item : movable) {
		m_relative_pos.insert(item, item->pos());
	}
	m_group_origin = grid_origin;

		//The conductors were laid out against the original terminal
		//positions, so re-route them before anything is drawn.
	const QList<Conductor *> conductors = m_content.conductors(DiagramContent::AnyConductor);
	for (auto *conductor : conductors) {
		conductor->updatePath();
	}

		//The baseline is the group's grid-snapped origin. For UnderCursor
		//the group was just put there, so the baseline is known outright.
		//For AtOrigin the group stayed where it was copied from and the
		//OS cursor is warped to match instead -- but only a warp that
		//verifiably landed may be trusted as the baseline: QCursor::setPos()
		//needs pointer focus inside the target window (on Wayland it goes
		//through wp_pointer_warp_v1, and is a silent no-op without it), and
		//a baseline that assumes a warp which did not happen would move the
		//whole group by the warp-sized delta on the first mouse move. So
		//when the warp cannot be confirmed, leave m_baseline_captured
		//false: moveTo() re-baselines from the first real mouse position
		//instead, which costs at most the first movement but never flings
		//the group across the folio.
	if (m_placement == AtOrigin && !m_diagram->views().isEmpty()) {
		if (auto *view = m_diagram->views().at(0)) {
				//An origin outside the visible area cannot be warped to
				//(the compositor rejects targets outside the window) and
				//would leave the paste invisible, so scroll it into view
				//first.
			if (!view->viewport()->rect().contains(view->mapFromScene(m_group_origin))) {
				view->ensureVisible(items_rect, 50, 50);
			}
			const QPoint view_pos = view->mapFromScene(m_group_origin);
			const QPoint global_pos = view->viewport()->mapToGlobal(view_pos);
			QCursor::setPos(global_pos);
			if ((QCursor::pos() - global_pos).manhattanLength() <= 4) {
				m_initial_cursor = m_group_origin;
				m_baseline_captured = true;
			}
		}
	}
	if (m_placement == UnderCursor) {
		m_initial_cursor = m_group_origin;
		m_baseline_captured = true;
	}
	if (m_placement == InPlace) {
		m_initial_cursor = snapGrid(start_pos);
		m_current_pos = m_initial_cursor;
		m_baseline_captured = true;
	}

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
	removeHelpLines();
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

bool DiagramEventAddPaste::ctrlDragCopies()
{
	return QSettings().value(QStringLiteral("diagrameditor/ctrl_drag_copies"), false).toBool();
}

void DiagramEventAddPaste::setCtrlDragCopies(bool copies)
{
	QSettings().setValue(QStringLiteral("diagrameditor/ctrl_drag_copies"), copies);
}

bool DiagramEventAddPaste::duplicateFollowsMouse()
{
	return QSettings().value(QStringLiteral("diagrameditor/duplicate_follows_mouse"), false).toBool();
}

void DiagramEventAddPaste::setDuplicateFollowsMouse(bool follows)
{
	QSettings().setValue(QStringLiteral("diagrameditor/duplicate_follows_mouse"), follows);
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
		view->setFocus(Qt::OtherFocusReason);
	}
}

void DiagramEventAddPaste::showHint()
{
	if (!m_status_bar) return;

	const QKeySequence x_seq = ShortcutManager::instance().currentSequence(
		QStringLiteral("diagrameditor.paste_lock_x"));
	const QKeySequence y_seq = ShortcutManager::instance().currentSequence(
		QStringLiteral("diagrameditor.paste_lock_y"));
	const QString x_str = x_seq.isEmpty() ? QStringLiteral("X") : x_seq.toString(QKeySequence::NativeText);
	const QString y_str = y_seq.isEmpty() ? QStringLiteral("Y") : y_seq.toString(QKeySequence::NativeText);

	QString status;
	if (m_lock_x && m_lock_y) {
		status = tr("Original position (X+Y locked)", "status bar lock state");
	} else if (m_lock_x) {
		status = tr("Original X locked (move along Y)", "status bar lock state");
	} else if (m_lock_y) {
		status = tr("Original Y locked (move along X)", "status bar lock state");
	}

	QString msg = tr("Click to place, Esc or right-click to cancel. [%1]: lock X, [%2]: lock Y, [%1]+[%2]: original position",
			 "status bar tip while positioning a paste").arg(x_str, y_str);
	if (!status.isEmpty()) {
		msg += QStringLiteral(" -- ") + status;
	}
	m_status_bar->showMessage(msg);
}

void DiagramEventAddPaste::updateEffectivePosition()
{
	const QPointF target = m_initial_cursor + (m_original_pos - m_group_origin);
	QPointF effective_pos = m_current_pos;
	QPointF visible_pos = m_group_origin + (m_current_pos - m_initial_cursor);
	if (m_lock_x) {
		effective_pos.setX(target.x());
		visible_pos.setX(m_original_pos.x());
	}
	if (m_lock_y) {
		effective_pos.setY(target.y());
		visible_pos.setY(m_original_pos.y());
	}
	if ((m_lock_x || m_lock_y) && m_diagram && !m_diagram->views().isEmpty()) {
		if (auto *view = m_diagram->views().at(0)) {
			if (!view->viewport()->rect().contains(view->mapFromScene(visible_pos.toPoint()))) {
				view->ensureVisible(QRectF(visible_pos, QSizeF(100, 100)), 50, 50);
			}
		}
	}
	moveTo(effective_pos);
	updateHelpLines();
}

void DiagramEventAddPaste::updateHelpLines()
{
	if (!m_diagram) return;

	if (!m_lock_x && !m_lock_y) {
		if (m_help_line_x) m_help_line_x->setVisible(false);
		if (m_help_line_y) m_help_line_y->setVisible(false);
		return;
	}

	const bool dark = Diagram::background_color.lightness() < 128;
	const QColor color = dark ? QColor(85, 170, 255) : QColor(0, 102, 204);
	QPen pen(color, 1.0, Qt::DashLine);
	pen.setCosmetic(true);

	QRectF rect = m_diagram->border_and_titleblock.insideBorderRect();
	if (rect.isEmpty()) {
		rect = m_diagram->sceneRect();
	}
	const qreal left = rect.isValid() ? rect.left() - 5000 : -10000;
	const qreal right = rect.isValid() ? rect.right() + 5000 : 10000;
	const qreal top = rect.isValid() ? rect.top() - 5000 : -10000;
	const qreal bottom = rect.isValid() ? rect.bottom() + 5000 : 10000;

	// When X is locked (or both): movement is along Y, show dashed line on Y axis at original X
	if (m_lock_x) {
		if (!m_help_line_y) {
			m_help_line_y = new QGraphicsLineItem();
			m_help_line_y->setPen(pen);
			m_help_line_y->setZValue(1e6);
			m_diagram->addItem(m_help_line_y);
		} else {
			m_help_line_y->setPen(pen);
		}
		m_help_line_y->setLine(m_original_pos.x(), top, m_original_pos.x(), bottom);
		m_help_line_y->setVisible(true);
	} else if (m_help_line_y) {
		m_help_line_y->setVisible(false);
	}

	// When Y is locked (or both): movement is along X, show dashed line on X axis at original Y
	if (m_lock_y) {
		if (!m_help_line_x) {
			m_help_line_x = new QGraphicsLineItem();
			m_help_line_x->setPen(pen);
			m_help_line_x->setZValue(1e6);
			m_diagram->addItem(m_help_line_x);
		} else {
			m_help_line_x->setPen(pen);
		}
		m_help_line_x->setLine(left, m_original_pos.y(), right, m_original_pos.y());
		m_help_line_x->setVisible(true);
	} else if (m_help_line_x) {
		m_help_line_x->setVisible(false);
	}
}

void DiagramEventAddPaste::removeHelpLines()
{
	if (m_help_line_x) {
		if (m_diagram) m_diagram->removeItem(m_help_line_x);
		delete m_help_line_x;
		m_help_line_x = nullptr;
	}
	if (m_help_line_y) {
		if (m_diagram) m_diagram->removeItem(m_help_line_y);
		delete m_help_line_y;
		m_help_line_y = nullptr;
	}
}

bool DiagramEventAddPaste::matchesShortcut(const QKeyEvent *event, const QString &action_id) const
{
	const QKeySequence sequence = ShortcutManager::instance().currentSequence(action_id);
	if (sequence.isEmpty()) {
		return false;
	}
	const int key = event->key();
	if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Meta) {
		return false;
	}
	const Qt::KeyboardModifiers mods = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
	const int key_with_mod = (mods != Qt::NoModifier) ? (int(mods) | key) : key;
	return sequence.matches(QKeySequence(key_with_mod)) == QKeySequence::ExactMatch;
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
	const int xGrid = FolioGrid::step(settings.value(FolioGrid::x_key), Diagram::xGrid);
	const int yGrid = FolioGrid::step(settings.value(FolioGrid::y_key), Diagram::yGrid);

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

		//A cable line is handed a step rather than a place to stand: it
		//folds a translation into its own geometry and into the colour
		//labels it carries and keeps its own pos() at (0, 0)
		//(CablePart::itemChange, BridgePart::itemChange), so the position stored above -- always
		//(0, 0) for it -- would be an addition again on every mouse move
		//and the line would run away from the group it was pasted with.
		//The difference to the last step keeps the translation the same
		//one the rest of the paste gets, expressed their way: a place.
	const QPointF step = delta - m_step_delta;
	for (auto it = m_relative_pos.constBegin() ; it != m_relative_pos.constEnd() ; ++it) {
		if (!it.key()) continue;
		if (m_step_items.contains(it.key())) {
			it.key()->setPos(step);
		} else {
			it.key()->setPos(it.value() + delta);
		}
	}
	m_step_delta = delta;

		//Update conductor paths so they follow the moved terminals.
	const QList<Conductor *> conductors = m_content.conductors(DiagramContent::AnyConductor);
	for (auto *conductor : conductors) {
		conductor->updatePath();
	}
}

void DiagramEventAddPaste::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_running) return;
	m_current_pos = event->scenePos();
	updateEffectivePosition();
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

bool DiagramEventAddPaste::handlesShortcutOverride(QKeyEvent *event) const
{
	return m_running && (
		matchesShortcut(event, QStringLiteral("diagrameditor.paste_lock_x"))
		|| matchesShortcut(event, QStringLiteral("diagrameditor.paste_lock_y")));
}

void DiagramEventAddPaste::keyPressEvent(QKeyEvent *event)
{
	if (!m_running) return;

	if (matchesShortcut(event, QStringLiteral("diagrameditor.paste_lock_x"))) {
		m_lock_x = !m_lock_x;
		updateEffectivePosition();
		showHint();
		event->setAccepted(true);
		return;
	}

	if (matchesShortcut(event, QStringLiteral("diagrameditor.paste_lock_y"))) {
		m_lock_y = !m_lock_y;
		updateEffectivePosition();
		showHint();
		event->setAccepted(true);
		return;
	}

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

	The cable lines are wired just before that, which is the first moment
	the place they will occupy is final: each colour label takes the wire
	it happens to stand on, and a line the user refuses to let take wires
	away from another cable goes away again on its own.
*/
void DiagramEventAddPaste::commit()
{
	if (m_finished || !m_diagram) return;
	removeHelpLines();
	m_finished = true;
	m_running = false;

	const QList<CableCopy::Wired> wired = CableCopy::wire(
			m_diagram, m_cables,
			m_content.conductors(DiagramContent::AnyConductor));
	m_cables.clear();

		//The cable lines the copy brings are not part of the paste
		//itself: they are handed to the paste so that one Ctrl+Z takes
		//the whole of it back in one go.
	QList<QUndoCommand *> cable_commands;
	for (const CableCopy::Wired &line : std::as_const(wired))
	{
		if (!line.part) continue;

		auto *added = new AddCableCommand(line.part->cable(), line.part.data(),
										  m_diagram, true, line.taken);
		added->setText(QCoreApplication::translate("CableCopy", "Paste a cable"));
		cable_commands << added;
	}

		//Asks whether to import the numberings the copy brings, if the
		//project has not got them; the paste itself and the cable lines
		//travel inside the same step of the undo stack.
			//The bridge lines the same: what they land on is worked
		//out now, at the place the hand left them -- one Ctrl+Z takes
		//the whole of it back in one go.
	QList<QUndoCommand *> bridge_commands;
	for (Bridge *bridge : std::as_const(m_bridges)) {
		for (QUndoCommand *added : BridgeCopy::land(m_diagram, bridge)) {
			added->setText(QCoreApplication::translate("BridgeCopy", "Paste a bridge"));
			bridge_commands << added;
		}
	}
	m_bridges.clear();

PasteNumberingImport::push(m_diagram->views().isEmpty() ? nullptr : m_diagram->views().first(),
							   m_diagram, m_content, m_copied_schemes, cable_commands + bridge_commands);

		//The undo stack has just run its first redo, which may have
		//renewed the identity of a pasted conductor: the cores are
		//pointed at the wires again so their cable field survives it.
	CableCopy::repoint(wired);
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
	removeHelpLines();
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

		//The cable lines never reached the undo stack either, so they
		//are taken back the way they were built: off the folio, out of
		//the project and deleted, leaving no trace of having been
		//there -- not even a project asking to be saved for nothing.
	for (CablePart *part : std::as_const(m_cables)) {
		CableCopy::discard(m_diagram, part);
	}
	m_cables.clear();

		//The bridge lines likewise: the bridges they were made
		//for go away whole, leaving no trace of having been there.
	for (Bridge *bridge : std::as_const(m_bridges)) {
		BridgeCopy::discard(m_diagram, bridge);
	}
	m_bridges.clear();

	m_content.clear();
	m_relative_pos.clear();
}
