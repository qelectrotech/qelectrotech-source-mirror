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
#include "diagrameventaddshape.h"

#include "../diagram.h"
#include "../lastusedstyle.h"
#include "../qetapp.h"
#include "../qetdiagrameditor.h"
#include "../undocommand/addgraphicsobjectcommand.h"

#include <QGraphicsEllipseItem>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QStatusBar>
#include <QTimer>

/**
	@brief DiagramEventAddShape::DiagramEventAddShape
	Default constructor
	@param diagram : the diagram where this event must operate
	@param shape_type : the type of shape to draw
*/
DiagramEventAddShape::DiagramEventAddShape(Diagram *diagram, QetShapeItem::ShapeType shape_type) :
	DiagramEventInterface(diagram),
	m_shape_type (shape_type),
	m_shape_item (nullptr),
	m_help_horiz (nullptr),
	m_help_verti (nullptr)
{
	m_running = true;
	init();
	// Deferred to the next event-loop iteration, not shown immediately:
	// Diagram::setEventInterface() destroys whatever tool was
	// previously active *after* this constructor returns, and that
	// previous tool's own destructor clears the status bar (see
	// ~DiagramEventAddShape() below) -- an immediate show here would
	// just get wiped out moments later by that cleanup. Letting the old
	// tool's teardown finish first, then showing this one's message, is
	// the same fix already used for the tooltip-flicker issue in
	// QetShapeItem::refreshInteractionHints(), applied to the same
	// class of "something later in the same call chain undoes what I
	// just did" ordering problem.
	QTimer::singleShot(0, this, [this]() { updateCreationHint(); });
}

/**
	@brief DiagramEventAddShape::~DiagramEventAddShape
*/
DiagramEventAddShape::~DiagramEventAddShape()
{
	if ((m_running || m_abort) && m_shape_item)
	{
		m_diagram->removeItem(m_shape_item);
		delete m_shape_item;
	}
	delete m_help_horiz;
	delete m_help_verti;
	delete m_center_marker;

	if (m_diagram && !m_diagram->views().isEmpty())
	{
		if (auto *editor = QETApp::diagramEditorAncestorOf(m_diagram->views().constFirst()))
			editor->statusBar()->clearMessage();
	}

	foreach (QGraphicsView *v, m_diagram->views())
		v->setContextMenuPolicy(Qt::DefaultContextMenu);
}

/**
	@brief DiagramEventAddShape::applyPosition
	Applies a drag/click position to the in-progress shape, honouring two
	modifiers that mirror how the very same shape can already be edited
	afterward, once placed:
	  - Ctrl, for Rectangle/Ellipse only: the first click becomes the
	    shape's *center* rather than a corner, growing symmetrically as
	    the cursor moves away from it -- the same meaning Ctrl already
	    has on a Resize handle (anchor at center). Deliberately not
	    offered for Line: unlike the Rectangle/Ellipse case, there's no
	    established convention for "a line grows symmetrically from its
	    middle" to justify it by, so Ctrl for Line means only free
	    positioning (see the plain grid-snap check above), nothing more.
	  - Shift, for Rectangle/Ellipse only: forces the bounding box square
	    (so Ellipse becomes a true circle), using whichever of the two
	    dragged dimensions is currently larger and mirroring that onto
	    the other, preserving the direction the user is actually
	    dragging in.
	Both can combine (Ctrl+Shift: a centered square/circle). Whether or
	not Ctrl is currently held, the non-anchored branch always rebuilds
	from m_anchor_point rather than nudging the existing rect/line --
	otherwise, if Ctrl had been held earlier in the same drag (moving the
	shape's own first point to a mirrored position), releasing it would
	leave that point stuck there instead of actually restoring it.
*/
void DiagramEventAddShape::applyPosition(const QPointF &pos, Qt::KeyboardModifiers mods)
{
	if (!m_shape_item)
		return;

	if (m_shape_type == QetShapeItem::Polygon)
	{
		// setP2() has its own dedicated Polygon handling: it moves the
		// *last* vertex in place rather than setting a second point,
		// which is exactly the live "next segment follows the mouse"
		// preview -- the same idea DiagramEventAddPath's trailing
		// preview node gives the pen tool. No Ctrl/Shift modifiers apply
		// here (those are Rectangle/Ellipse/Line-specific below), so
		// this is a direct, unconditional call.
		m_shape_item->setP2(pos);
		return;
	}

	// m_center_anchored is decided once, in mousePressEvent, not
	// re-checked here on every call -- re-checking it live meant
	// releasing Ctrl mid-drag (something you'd naturally do the moment
	// your hand gets tired holding it, long before you're done resizing)
	// silently snapped the shape back to corner-anchored, discarding
	// what felt like an already-made decision. Deciding it once at the
	// first click matches "I held Ctrl when I clicked, so this shape is
	// centered" -- a single, predictable rule instead of a live toggle.
	QPointF target = pos;

	if ((mods & Qt::ShiftModifier)
			&& (m_shape_type == QetShapeItem::Rectangle || m_shape_type == QetShapeItem::Ellipse))
	{
		const QPointF ref = m_anchor_point;
		const qreal dx = target.x() - ref.x();
		const qreal dy = target.y() - ref.y();
		const qreal size = qMax(qAbs(dx), qAbs(dy));
		target.setX(ref.x() + (dx < 0 ? -size : size));
		target.setY(ref.y() + (dy < 0 ? -size : size));
	}

	if (m_center_anchored)
	{
		const QPointF mirrored = 2 * m_anchor_point - target;
		m_shape_item->setRect(QRectF(mirrored, target));
	}
	else
	{
		if (m_shape_type == QetShapeItem::Line)
			m_shape_item->setLine(QLineF(m_anchor_point, target));
		else
			m_shape_item->setRect(QRectF(m_anchor_point, target));
	}
}

/**
	@brief DiagramEventAddShape::showCenterMarker
	Small, filled marker at the anchor point, shown only while Ctrl-
	anchoring is actually in effect right now (see applyPosition()) --
	doubling as live confirmation that it is, rather than leaving the
	user to infer it purely from how the shape happens to be growing.
*/
void DiagramEventAddShape::showCenterMarker(const QPointF &scenePos)
{
	if (!m_center_marker)
	{
		m_center_marker = new QGraphicsEllipseItem(-4, -4, 8, 8);
		QPen pen(Qt::red);
		pen.setCosmetic(true);
		m_center_marker->setPen(pen);
		m_center_marker->setBrush(Qt::red);
		m_diagram->addItem(m_center_marker);
	}
	m_center_marker->setPos(scenePos);
}

void DiagramEventAddShape::hideCenterMarker()
{
	delete m_center_marker;
	m_center_marker = nullptr;
}

/**
	@brief DiagramEventAddShape::mousePressEvent
	Action when mouse is pressed
	@param event : event of mouse press
*/
void DiagramEventAddShape::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (Q_UNLIKELY(m_diagram->isReadOnly())) {
		return;
	}

	QPointF pos = event->scenePos();
	// A bitwise flag check, not exact equality: modifiers() == Ctrl
	// alone fails the moment any other key (Shift for the square/circle
	// lock, or an incidental platform flag) is also held, silently
	// falling through to snapToGrid() even though Ctrl is held --
	// exactly what made Ctrl+Shift together feel "frozen" (both grid-
	// snapped *and* square-locked, quantizing to whichever is coarser)
	// and made "Ctrl = free positioning" not actually hold up.
	if (!(event->modifiers() & Qt::ControlModifier)) {
		pos = Diagram::snapToGrid(pos);
	}

		//Action for left mouse click
	if (event->button() == Qt::LeftButton)
	{
			//Create shape item
		if (!m_shape_item)
		{
			m_shape_item = new QetShapeItem(pos, pos, m_shape_type);
			m_anchor_point = pos;
			// Decided once, here, rather than re-checked on every mouse
			// move for the rest of the drag -- see applyPosition()'s doc
			// comment for why continuous re-checking made releasing Ctrl
			// mid-drag feel like a bug rather than a deliberate choice.
			m_center_anchored = (event->modifiers() & Qt::ControlModifier)
					&& (m_shape_type == QetShapeItem::Rectangle || m_shape_type == QetShapeItem::Ellipse);
			if (m_center_anchored)
				showCenterMarker(m_anchor_point);
				//Start from whatever pen/brush was last applied this
				//session, rather than always the hardcoded default.
			if (LastUsedStyle::hasShapePen()) {
				m_shape_item->setPen(LastUsedStyle::shapePen());
			}
			if (LastUsedStyle::hasShapeBrush()) {
				m_shape_item->setBrush(LastUsedStyle::shapeBrush());
			}
			m_diagram->addItem (m_shape_item);
			updateCreationHint();
			event->setAccepted(true);
			return;
		}

			//If current item isn't a polyline, add it with an undo command
		if (m_shape_type != QetShapeItem::Polygon)
		{
			applyPosition(pos, event->modifiers());
			if (m_shape_item->shapeType() == QetShapeItem::Rectangle || m_shape_item->shapeType() == QetShapeItem::Ellipse) {
				m_shape_item->setRect(m_shape_item->rect().normalized());
			}
			m_diagram->undoStack().push (new AddGraphicsObjectCommand(m_shape_item, m_diagram));
			m_shape_item = nullptr; //< set to nullptr for create new shape at next left clic
			hideCenterMarker();
			updateCreationHint();
		}
			//Else add a new point to polyline
		else
		{
			m_shape_item->setNextPoint (pos);
		}

		event->setAccepted(true);
		return;
	}

	if (event->button() == Qt::RightButton) {
		event->setAccepted(true);
	}
}

/**
	@brief DiagramEventAddShape::mouseMoveEvent
	Action when mouse move
	@param event : event of mouse move
*/
void DiagramEventAddShape::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	updateHelpCross(event->scenePos());

	// Re-asserted on every move, not just once at activation: Qt's own
	// built-in "show an action's statusTip on hover" has its own
	// internal "restore whatever was there before" logic for when the
	// hover ends. Since our message is shown *during* that same hover
	// session (the user is still over the toolbar icon when the
	// deferred constructor-time call fires), Qt's hover-tracking has no
	// idea we changed the status bar in the meantime -- the moment the
	// mouse leaves the icon for the canvas, it "restores" to whatever it
	// remembers being there before its own tip started, which is stale
	// and empty, silently overwriting ours. Re-showing it here, on every
	// move within the canvas, simply outlasts that one-time restore.
	updateCreationHint();

	if (m_shape_item && event->buttons() == Qt::NoButton)
	{
		m_last_mouse_scene_pos = event->scenePos();   // raw, before snapping -- see reapplyLastPosition()

		QPointF pos = event->scenePos();
		if (!(event->modifiers() & Qt::ControlModifier)) {
			pos = Diagram::snapToGrid(pos);
		}

		applyPosition(pos, event->modifiers());
		event->setAccepted(true);
	}
}

/**
	@brief DiagramEventAddShape::keyPressEvent / keyReleaseEvent
	Pressing or releasing Shift (the square/circle lock) does nothing
	visible on its own -- applyPosition() only ever runs from
	mouseMoveEvent, so without this, a keyboard-only change just sits
	there until the next, often incidental, pixel of mouse movement
	brings the shape in line with it. That's exactly what looked like a
	freeze: holding Shift while the mouse is genuinely still produces no
	visible change (correctly -- nothing has moved), and it only
	"unsticks" once the mouse moves again, which released keys tend to
	coincide with purely by hand tremor, not because releasing itself
	did anything. Re-running the last known mouse position through
	applyPosition() here makes the key press or release itself the
	trigger, giving immediate feedback instead of waiting on chance.
*/
void DiagramEventAddShape::keyPressEvent(QKeyEvent *event)
{
	reapplyLastPosition(event);
}

void DiagramEventAddShape::keyReleaseEvent(QKeyEvent *event)
{
	reapplyLastPosition(event);
}

void DiagramEventAddShape::reapplyLastPosition(QKeyEvent *event)
{
	if (!m_shape_item || (event->key() != Qt::Key_Shift && event->key() != Qt::Key_Control))
		return;

	// A fresh, global query, not event->modifiers(): for a press/release
	// of a modifier key itself, whether that key is already reflected in
	// the key event's own modifiers() is ambiguous and platform-
	// dependent -- the same reason Diagram::snapToGrid() queries this
	// directly rather than trusting a passed-in modifiers() value.
	const Qt::KeyboardModifiers mods = QGuiApplication::keyboardModifiers();
	QPointF pos = m_last_mouse_scene_pos;
	if (!(mods & Qt::ControlModifier))
		pos = Diagram::snapToGrid(pos);

	applyPosition(pos, mods);
}

/**
	@brief DiagramEventAddShape::mouseReleaseEvent
	Action when mouse button is released
	@param event : event of mouse release
*/
void DiagramEventAddShape::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::RightButton)
	{
			//If shape is created, we manage right click
		if (m_shape_item)
		{
				//Shape is a polyline and have three points or more we just remove the last point
			if (m_shape_type == QetShapeItem::Polygon && (m_shape_item->pointsCount() >= 3) )
			{
				m_shape_item->removePoints();

				QPointF pos = event->scenePos();
				if (!(event->modifiers() & Qt::ControlModifier))
					pos = Diagram::snapToGrid(pos);

				m_shape_item->setP2(pos); //Set the new last point under the cursor
				event->setAccepted(true);
				return;
			}

				//For other case, we remove item from scene
			m_diagram->removeItem(m_shape_item);
			delete m_shape_item;
			m_shape_item = nullptr;
			hideCenterMarker();
			updateCreationHint();
			event->setAccepted(true);
			return;
		}

			//Else (no shape), we set to false the running status
			//for indicate to the owner of this event that everything is done
		m_running = false;
		emit finish();
		event->setAccepted(true);
	}
}

/**
	@brief DiagramEventAddShape::mouseDoubleClickEvent
	Action when mouse button is double clicked
	@param event : event of mouse double click
*/
void DiagramEventAddShape::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
		//If current item is a polyline, add it with an undo command
	if (m_shape_item && m_shape_type == QetShapeItem::Polygon && event->button() == Qt::LeftButton)
	{
			//<double clic is used to finish polyline, but they also add two points at the same pos
			//<(double clic is a double press event), so we remove the last point of polyline
		m_shape_item->removePoints();

			//If the last is at the same pos of the first point
			//that mean user want a closed polygon, so we remove the last point and close polygon
		QPolygonF polygon = m_shape_item->polygon();
		if (polygon.first() == polygon.last())
		{
			m_shape_item->removePoints();
			m_shape_item->setClosed(true);
		}
		m_diagram->undoStack().push (new AddGraphicsObjectCommand(m_shape_item, m_diagram));
		m_shape_item = nullptr; //< set to nullptr for create new shape at next left clic
		hideCenterMarker();
		updateCreationHint();
		event->setAccepted(true);
	}
}

void DiagramEventAddShape::init()
{
	foreach (QGraphicsView *v, m_diagram->views())
		v->setContextMenuPolicy(Qt::NoContextMenu);
}

/**
	@brief DiagramEventAddShape::updateCreationHint
	Shows whichever of beforeClickHint()/afterClickHint() matches the
	current phase -- there was previously either no message at all
	(Line/Rectangle/Ellipse) or a single static one that never changed
	regardless of progress (Polygon, set externally in
	QETDiagramEditor::addItemGroupTriggered()); this replaces both with
	one phase-aware message per shape type, managed by the tool itself.
*/
void DiagramEventAddShape::updateCreationHint() const
{
	if (!m_diagram || m_diagram->views().isEmpty())
		return;
	if (auto *editor = QETApp::diagramEditorAncestorOf(m_diagram->views().constFirst()))
		editor->statusBar()->showMessage(m_shape_item ? afterClickHint() : beforeClickHint());
}

QString DiagramEventAddShape::beforeClickHint() const
{
	switch (m_shape_type)
	{
		case QetShapeItem::Line:
			return tr("Clic gauche : positionner le point de départ (Ctrl = position libre)");
		case QetShapeItem::Rectangle:
		case QetShapeItem::Ellipse:
			return tr("Clic gauche : positionner le premier coin (Ctrl = point central, position libre)");
		case QetShapeItem::Polygon:
			return tr("Clic gauche : positionner le premier point (Ctrl = position libre)");
		default:
			return QString();
	}
}

QString DiagramEventAddShape::afterClickHint() const
{
	switch (m_shape_type)
	{
		case QetShapeItem::Line:
			return tr("Clic gauche : positionner le point final (Ctrl = position libre) ; clic droit : annuler");
		case QetShapeItem::Rectangle:
			return tr("Clic gauche : positionner le coin opposé (Maj = carré, "
					"Ctrl = depuis le centre + position libre, Ctrl+Maj = carré centré) ; clic droit : annuler");
		case QetShapeItem::Ellipse:
			return tr("Clic gauche : positionner le coin opposé (Maj = cercle, "
					"Ctrl = depuis le centre + position libre, Ctrl+Maj = cercle centré) ; clic droit : annuler");
		case QetShapeItem::Polygon:
			return tr("Clic gauche : point suivant ; double-clic ou Entrée : terminer ; "
					"clic droit : annuler le dernier point");
		default:
			return QString();
	}
}

/**
	@brief DiagramEventAddShape::updateHelpCross
	Create and update the position of the cross to help user for draw new shape
	@param p : the center of the cross
*/
void DiagramEventAddShape::updateHelpCross(const QPointF &p)
{
		//If line isn't created yet, we create it.
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

		//Update the position of the cross
	QPointF point = Diagram::snapToGrid(p);

	m_help_horiz->setY(point.y());
	m_help_verti->setX(point.x());
}
