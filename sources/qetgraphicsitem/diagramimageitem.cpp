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
#include "diagramimageitem.h"

#include "../PropertiesEditor/propertieseditordialog.h"
#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../diagram.h"
#include "../diagramview.h"
#include "../qet.h"
#include "../qetapp.h"
#include "../qetdiagrameditor.h"
#include "../ui/imagepropertieswidget.h"
#include "../ui/imagecropdialog.h"
#include "../ui/imagetransparentcolordialog.h"
#include "../utils/qetutils.h"
#include "../QetGraphicsItemModeler/qetgraphicshandleritem.h"

#include <QAction>
#include <QFileDialog>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QMessageBox>

/**
	@brief DiagramImageItem::DiagramImageItem
	Constructor without pixmap
	@param parent_item the parent graphics item
*/
DiagramImageItem::DiagramImageItem(QetGraphicsItem *parent_item):
	QetGraphicsItem(parent_item)
{
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemSendsGeometryChanges);
	setAcceptHoverEvents(true);
}

/**
	@brief DiagramImageItem::DiagramImageItem
	Constructor with pixmap
	@param pixmap the pixmap to be draw
	@param parent_item the parent graphic item
*/
DiagramImageItem::DiagramImageItem(const QPixmap &pixmap, QetGraphicsItem *parent_item):
	QetGraphicsItem(parent_item),
	pixmap_(pixmap),
	m_base_pixmap(pixmap),
	m_crop_rect(pixmap.rect())
{
	// m_transform.toMatrix(), not QGraphicsItem::setRotation()/setScale():
	// those are a single uniform scale() float, which is exactly why an
	// image could never break its own aspect ratio before this class
	// gained proper independent scaleX/scaleY.
	m_transform.pivot = boundingRect().center();
	setTransform(m_transform.toMatrix());
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable|QGraphicsItem::ItemSendsGeometryChanges);
	setAcceptHoverEvents(true);
}

/**
	@brief DiagramImageItem::~DiagramImageItem
	Destructor
*/
DiagramImageItem::~DiagramImageItem()
{
	if (!m_handler_vector.isEmpty())
		qDeleteAll(m_handler_vector);
}

/**
	@brief DiagramImageItem::paint
	Draw the pixmap.
	@param painter the Qpainter to use for draw the pixmap
	@param option the style option
	@param widget the QWidget where we draw the pixmap
*/
void DiagramImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	painter -> drawPixmap(pixmap_.rect(),pixmap_);

	Q_UNUSED(option); Q_UNUSED(widget);

	if (isSelected()) {
		painter -> save();
		// Annulation des renderhints
		painter -> setRenderHint(QPainter::Antialiasing,          false);
		painter -> setRenderHint(QPainter::TextAntialiasing,      false);
		painter -> setRenderHint(QPainter::SmoothPixmapTransform, false);
		// Dessin du cadre de selection en noir à partir du boundingrect
		QPen t(Qt::black);
		t.setStyle(Qt::DashLine);
		painter -> setPen(t);
		painter -> drawRect(boundingRect());
		painter -> restore();
	}
}

/**
	@brief DiagramImageItem::editProperty
	Open the appropriate dialog to edit this image
*/
void DiagramImageItem::editProperty()
{
	if (diagram() -> isReadOnly()) return;
	PropertiesEditorDialog dialog(new ImagePropertiesWidget(this), QApplication::activeWindow());
	dialog.exec();
}

/**
	@brief DiagramImageItem::setPixmap
	Set the new pixmap to be draw
	@param pixmap the new pixmap
*/
void DiagramImageItem::setPixmap(const QPixmap &pixmap) {
	// Only ever mattered once setPixmap() could be called on an
	// already-placed item with a differently-sized replacement (see
	// replaceImage()) -- previously this only ran from the constructor,
	// before the item existed in any scene, where there was nothing yet
	// to invalidate. Without this, a same-session replace to a
	// different-sized image would leave the scene's bounding-rect cache
	// stale, the exact class of bug already fixed multiple times this
	// session for shapes.
	//
	// Deliberately does NOT touch m_transform.pivot/pos() here, even
	// though a differently-sized pixmap generally means the old pivot
	// (a bounding-rect-relative point) no longer means the same thing:
	// crop() and replaceImage(), the only two callers that ever change
	// the pixmap's size, each have their own specific idea of what
	// should happen to position and pivot when they do (see their own
	// comments) -- a single generic rule here would fight with
	// whichever of them actually needs something more specific.
	prepareGeometryChange();
	pixmap_ = pixmap;
	emit pixmapChanged();
}

/**
	@brief DiagramImageItem::setScaleFactorX / setScaleFactorY / setRotationAngle
	Matching QetShapeItem's own setters for the identical fields --
	same pattern, same reasoning: change the one field, rebuild the
	matrix, notify. No pivot or position compensation needed here,
	unlike setPivot() below, since neither scale nor rotation moves the
	pivot itself.
*/
void DiagramImageItem::setScaleFactorX(qreal factor)
{
	if (qFuzzyCompare(m_transform.scaleX, factor)) return;
	prepareGeometryChange();
	m_transform.scaleX = factor;
	setTransform(m_transform.toMatrix());
	emit transformChanged();
}

void DiagramImageItem::setScaleFactorY(qreal factor)
{
	if (qFuzzyCompare(m_transform.scaleY, factor)) return;
	prepareGeometryChange();
	m_transform.scaleY = factor;
	setTransform(m_transform.toMatrix());
	emit transformChanged();
}

void DiagramImageItem::setRotationAngle(qreal angle)
{
	if (qFuzzyCompare(m_transform.rotation, angle)) return;
	prepareGeometryChange();
	m_transform.rotation = angle;
	setTransform(m_transform.toMatrix());
	emit transformChanged();
}

void DiagramImageItem::setSkewX(qreal skew)
{
	if (qFuzzyCompare(m_transform.skewX, skew)) return;
	prepareGeometryChange();
	m_transform.skewX = skew;
	setTransform(m_transform.toMatrix());
	emit transformChanged();
}

void DiagramImageItem::setSkewY(qreal skew)
{
	if (qFuzzyCompare(m_transform.skewY, skew)) return;
	prepareGeometryChange();
	m_transform.skewY = skew;
	setTransform(m_transform.toMatrix());
	emit transformChanged();
}

/**
	@brief DiagramImageItem::setPivot
	Unlike scale/rotation, moving the pivot alone WOULD visibly shift
	the image on screen, since the pivot is baked directly into the
	transform matrix -- compensatedPositionForNewPivot() (already
	proven correct for QetShapeItem's identical need) solves for
	whatever pos() keeps the image exactly where it already was, so
	only the *handle* moves, not the picture underneath it.
*/
void DiagramImageItem::setPivot(const QPointF &newPivot)
{
	if (m_transform.pivot == newPivot) return;
	const QPointF newPos = compensatedPositionForNewPivot(pos(), m_transform.pivot, newPivot, m_transform.linearPart());
	prepareGeometryChange();
	m_transform.pivot = newPivot;
	// Verified numerically that the underlying math keeps the image
	// exactly in place when the pivot moves -- this guard is about a
	// DIFFERENT problem: setPos() and setTransform() are two separate
	// QGraphicsItem operations, each independently triggering
	// itemChange(), so without this, repositionHandles() (and,
	// visibly, a render) could happen using the new pos() but the
	// still-old transform (or vice versa) in the moment between the
	// two calls -- a real, if brief, wobble, not a calculation error.
	// QetShapeItem's own setPivot() already needed the identical fix
	// for the identical reason.
	m_deferHandleReposition = true;
	// QGraphicsObject::setPos(), not the plain setPos() this class
	// would otherwise inherit: QetGraphicsItem overrides setPos() to
	// snap to the diagram's grid, which is exactly right for a user
	// dragging the whole image around, but silently corrupts this
	// specific, exactly-computed compensation -- rounding it to the
	// nearest grid point defeats the entire point of computing an
	// exact value in the first place, and was the actual cause of the
	// "picture drifts slightly" symptom (invisible at scale 1 with no
	// rotation, since the compensation is a no-op there regardless).
	QGraphicsObject::setPos(newPos);
	setTransform(m_transform.toMatrix());
	m_deferHandleReposition = false;
	repositionHandles();
	emit transformChanged();
}

/**
	@brief DiagramImageItem::setPivotRaw
	Sets m_transform.pivot directly, with NO position compensation at
	all -- deliberately, unlike setPivot() above. Exists solely for
	crop()'s own undo chain (see the Q_PROPERTY declaration's comment
	for why): crop() already computes the fully correct pos() itself,
	accounting for the crop, and pushes that as its own independent
	undo step, so compensating pos() *again* here would silently
	corrupt it back to a wrong value the moment this step replays.
	Never call this expecting the image to stay visually in place on
	its own -- it won't; the caller is responsible for that.
*/
void DiagramImageItem::setPivotRaw(const QPointF &newPivot)
{
	if (m_transform.pivot == newPivot) return;
	prepareGeometryChange();
	m_transform.pivot = newPivot;
	setTransform(m_transform.toMatrix());
	repositionHandles();
	emit transformChanged();
}

/**
	@brief DiagramImageItem::resetPivotToBoundingRectCenter
	Used after a resize drag ends (see handlerMouseReleaseEvent()) --
	NOT after crop() or replaceImage(), which each compute their own,
	more specific position handling instead (see setPixmap()'s comment
	for why a single generic rule doesn't fit both of those).
*/
void DiagramImageItem::resetPivotToBoundingRectCenter()
{
	m_pivotIsCustom = false;
	setPivot(boundingRect().center());
}

namespace {
	// Local-space (natural, unscaled pixmap size) position of each of
	// the 8 resize handles. Indices: 0=TL 1=TM 2=TR 3=ML 4=MR 5=BL
	// 6=BM 7=BR -- arbitrary but fixed, matched by oppositeHandleIndex()
	// and adjustsX()/adjustsY() below.
	QPointF handleNaturalPosition(int index, qreal w, qreal h)
	{
		switch (index)
		{
			case 0: return QPointF(0, 0);
			case 1: return QPointF(w / 2, 0);
			case 2: return QPointF(w, 0);
			case 3: return QPointF(0, h / 2);
			case 4: return QPointF(w, h / 2);
			case 5: return QPointF(0, h);
			case 6: return QPointF(w / 2, h);
			case 7: return QPointF(w, h);
		}
		return QPointF();
	}

	int oppositeHandleIndex(int index)
	{
		static const int opposite[8] = {7, 6, 5, 4, 3, 2, 1, 0};
		return opposite[index];
	}

	// Corners adjust both axes; edge midpoints adjust only the axis
	// perpendicular to their own edge.
	bool adjustsX(int index) { return index != 1 && index != 6; }
	bool adjustsY(int index) { return index != 3 && index != 4; }
}

/**
	@brief DiagramImageItem::cornerPosition
	Matches QetShapeItem::cornerPoint()'s own slot convention exactly
	(0=TL 1=TR 2=BR 3=BL), so the rotate math ported from there (see
	dragRotateHandle()) lines up without needing its own re-derivation.
*/
QPointF DiagramImageItem::cornerPosition(int cornerIndex, qreal w, qreal h)
{
	switch (cornerIndex & 3)
	{
		case 0: return QPointF(0, 0);
		case 1: return QPointF(w, 0);
		case 2: return QPointF(w, h);
		default: return QPointF(0, h);
	}
}

/**
	@brief DiagramImageItem::edgeMidpointPosition
	Matches QetShapeItem::edgeMidpoint()'s own slot convention exactly
	(0=N 1=E 2=S 3=W), for the same reason as cornerPosition() above.
*/
QPointF DiagramImageItem::edgeMidpointPosition(int edgeIndex, qreal w, qreal h)
{
	switch (edgeIndex & 3)
	{
		case 0: return QPointF(w / 2, 0);
		case 1: return QPointF(w, h / 2);
		case 2: return QPointF(w / 2, h);
		default: return QPointF(0, h / 2);
	}
}

/**
	@brief DiagramImageItem::scaleOnlyOffset / scaleAndShearOffset
	Verbatim ports of QetShapeItem's own identically-named helpers (see
	their definitions there for the derivation) -- the offset of a local
	point from the pivot, with scale (and, for the second, shear too)
	applied but rotation deliberately left out. Used by
	dragRotateHandle()/dragSkewHandle() to solve for rotation/skew
	directly rather than through mapFromScene(), which would divide out
	the very value being solved for.
*/
QPointF DiagramImageItem::scaleOnlyOffset(const QPointF &localPoint) const
{
	const QPointF offset = localPoint - m_transform.pivot;
	return QPointF(offset.x() * m_transform.scaleX, offset.y() * m_transform.scaleY);
}

QPointF DiagramImageItem::scaleAndShearOffset(const QPointF &localPoint) const
{
	const QPointF scaled = scaleOnlyOffset(localPoint);
	const qreal kx = qTan(qDegreesToRadians(m_transform.skewX));
	const qreal ky = qTan(qDegreesToRadians(m_transform.skewY));
	return QPointF(scaled.x() + kx * scaled.y(), scaled.y() + ky * scaled.x());
}

/**
	@brief DiagramImageItem::handlePosition
	Local-space position of handle at vector index `index`, for
	whichever role it currently holds -- the single source both
	rebuildHandles() and repositionHandles() read from, so the two can
	never disagree about where a handle belongs.
*/
QPointF DiagramImageItem::handlePosition(int index) const
{
	const qreal w = pixmap_.width(), h = pixmap_.height();
	const HandleRole role = m_handleRoles.at(index);
	if (role == HandleRole::Resize)
		return handleNaturalPosition(index, w, h);
	if (role == HandleRole::Rotate)
		return cornerPosition(index, w, h);           // Rotate handles are always the first 4 in RotateSkew mode, so index IS the corner slot
	if (role == HandleRole::SkewEdge)
		return edgeMidpointPosition(index - 4, w, h);  // ...and SkewEdge the next 4, offset by those same 4
	return m_transform.pivot;                          // Pivot
}

/**
	@brief DiagramImageItem::mousePressEvent
	Clicking an already-selected image cycles its handle mode, exactly
	matching QetShapeItem's own established convention for the same
	gesture -- deliberately kept consistent rather than inventing a
	different interaction just because this is a different class.
*/
void DiagramImageItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	const bool wasAlreadySelected = isSelected();
	event->ignore();
	QetGraphicsItem::mousePressEvent(event);

	if (event->button() == Qt::LeftButton)
	{
		if (wasAlreadySelected && isSelected())
			toggleHandleMode();
		event->accept();
	}
}

/**
	@brief DiagramImageItem::toggleHandleMode
*/
void DiagramImageItem::toggleHandleMode()
{
	prepareGeometryChange();
	m_handleMode = (m_handleMode == HandleMode::Size) ? HandleMode::RotateSkew : HandleMode::Size;
	rebuildHandles();
	refreshInteractionHints();
}

/**
	@brief DiagramImageItem::nextHandleMode
	@return whichever mode a click would switch to from here -- the
	other one, since there are only two.
*/
DiagramImageItem::HandleMode DiagramImageItem::nextHandleMode() const
{
	return (m_handleMode == HandleMode::Size) ? HandleMode::RotateSkew : HandleMode::Size;
}

/**
	@brief DiagramImageItem::handleModeLabel
*/
QString DiagramImageItem::handleModeLabel(HandleMode mode)
{
	return (mode == HandleMode::Size) ? tr("redimensionner") : tr("pivoter/incliner");
}

/**
	@brief DiagramImageItem::updateModeHint
	Keeps the tooltip in sync with what the next click would do --
	called on selection change (so it appears/disappears with the
	handles themselves) and after every mode switch. Ported from
	QetShapeItem's identical method: deliberately short, since this is
	a tooltip, not documentation -- the fuller gesture/modifier
	reference lives in the status bar instead (see
	currentModeStatusHint(), hoverEnterEvent()).
*/
void DiagramImageItem::updateModeHint()
{
	setToolTip(isSelected()
			? tr("Cliquer : mode %1").arg(handleModeLabel(nextHandleMode()))
			: QString());
}

/**
	@brief DiagramImageItem::refreshInteractionHints
	Keeps the tooltip text current, and -- if already being hovered --
	immediately re-shows both the tooltip and the status bar hint
	rather than leaving them stuck on whatever was true before. Ported
	from QetShapeItem's identical method, for the identical reason: Qt
	only re-evaluates a tooltip, and this class only re-shows the
	status bar, when the cursor *moves* -- selecting an image (often
	clicked while the mouse was already sitting on it) or cycling
	handle modes (definitely clicked while sitting on it) both change
	what should be shown without the cursor moving at all.
*/
void DiagramImageItem::refreshInteractionHints()
{
	updateModeHint();

	if (!isHovered() || !isSelected())
		return;

	showStatusHint(currentModeStatusHint());
}

/**
	@brief DiagramImageItem::currentModeStatusHint
	One-line reference for whatever handles are visible right now,
	shown in the status bar while hovering a selected image's body --
	the modifier keys in particular (Ctrl, Shift) have no other visible
	indication that they do anything at all. Also carries the same
	"next mode" information as the tooltip, since the status bar has
	room for the full picture in one place.
*/
QString DiagramImageItem::currentModeStatusHint() const
{
	QString hint = (m_handleMode == HandleMode::Size)
			? tr("Glisser un coin/bord : redimensionner (Ctrl = depuis le centre, Maj = conserver les proportions)")
			: tr("Glisser un coin : pivoter (Maj = par pas de 15°) ; glisser un bord : incliner (Maj = par pas de 15°) ; point rouge : déplacer le centre de rotation");
	hint += tr(" -- %1 : mode %2").arg(tr("Cliquer"), handleModeLabel(nextHandleMode()));
	return hint;
}

/**
	@brief DiagramImageItem::hoverEnterEvent
*/
void DiagramImageItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	QetGraphicsItem::hoverEnterEvent(event);
	refreshInteractionHints();
}

/**
	@brief DiagramImageItem::hoverLeaveEvent
*/
void DiagramImageItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	QetGraphicsItem::hoverLeaveEvent(event);
	clearStatusHint();
}

/**
	@brief DiagramImageItem::clearHandles
*/
void DiagramImageItem::clearHandles()
{
	if (!m_handler_vector.isEmpty())
	{
		qDeleteAll(m_handler_vector);
		m_handler_vector.clear();
	}
	m_handleRoles.clear();
}

/**
	@brief DiagramImageItem::rebuildHandles
	A deliberately small switch compared to QetShapeItem's own version:
	images have exactly one "shape" (a plain rectangle matching the
	pixmap's natural size), so there's no shape-type branching to do --
	just the two handle modes themselves.
*/
/**
	@brief DiagramImageItem::colorForHandleRole / hintForHandleRole
	Single source of truth for both rebuildHandles() (which sets each
	handle's native tooltip) and sceneEventFilter()'s hover dispatch
	(which shows the identical text in the status bar) -- keeping these
	as two independently-maintained copies is exactly how they'd
	quietly drift apart over time, as already happened once between
	writing this pair.
*/
QColor DiagramImageItem::colorForHandleRole(HandleRole role)
{
	switch (role)
	{
		case HandleRole::Resize: return Qt::blue;
		case HandleRole::Rotate: return Qt::darkGreen;
		case HandleRole::SkewEdge: return QColor(255, 140, 0);
		case HandleRole::Pivot: return Qt::red;
	}
	return Qt::blue;
}

QString DiagramImageItem::hintForHandleRole(HandleRole role)
{
	switch (role)
	{
		case HandleRole::Resize: return tr("Glisser : redimensionner (Maj = conserver les proportions, Ctrl = depuis le centre)");
		case HandleRole::Rotate: return tr("Glisser : pivoter (Maj = par pas de 15°)");
		case HandleRole::SkewEdge: return tr("Glisser : incliner (Maj = par pas de 15°)");
		case HandleRole::Pivot: return tr("Glisser : déplacer le centre de rotation");
	}
	return QString();
}

void DiagramImageItem::rebuildHandles()
{
	clearHandles();

	if (m_handleMode == HandleMode::Size)
	{
		for (int i = 0; i < 8; ++i)
			m_handleRoles << HandleRole::Resize;
	}
	else // RotateSkew: 4 corners (rotate), then 4 edges (skew), then 1 pivot -- handlePosition() relies on this exact order
	{
		for (int i = 0; i < 4; ++i) m_handleRoles << HandleRole::Rotate;
		for (int i = 0; i < 4; ++i) m_handleRoles << HandleRole::SkewEdge;
		m_handleRoles << HandleRole::Pivot;
	}

	if (m_handleRoles.isEmpty() || !scene())
		return;

	QVector<QPointF> positions;
	for (int i = 0; i < m_handleRoles.size(); ++i)
		positions << handlePosition(i);

	m_handler_vector = QetGraphicsHandlerItem::handlerForPoint(mapToScene(positions), QETUtils::graphicsHandlerSize(this));

	for (int i = 0; i < m_handler_vector.size(); ++i)
	{
		QetGraphicsHandlerItem *h = m_handler_vector.at(i);
		h->setZValue(zValue() + 1);
		h->setColor(colorForHandleRole(m_handleRoles.at(i)));
		h->setToolTip(hintForHandleRole(m_handleRoles.at(i)));
		h->setAcceptHoverEvents(true);
		scene()->addItem(h);
		h->installSceneEventFilter(this);
	}
}

/**
	@brief DiagramImageItem::repositionHandles
	Moves the *existing* handle items to match current geometry, without
	touching their identity -- safe to call on every frame of a live
	drag, matching QetShapeItem's own repositionHandles()/rebuildHandles()
	split for the identical reason.
*/
void DiagramImageItem::repositionHandles()
{
	if (m_handler_vector.isEmpty())
		return;
	if (m_handler_vector.size() != m_handleRoles.size())
	{
		rebuildHandles();
		return;
	}

	for (int i = 0; i < m_handleRoles.size(); ++i)
		m_handler_vector.at(i)->setPos(mapToScene(handlePosition(i)));
}

/**
	@brief DiagramImageItem::sceneEventFilter
	Dispatches handle interaction events -- structurally identical to
	QetShapeItem::sceneEventFilter(), reused as a pattern rather than
	shared as code, since the two classes' handle roles are different
	enough (no path/polygon/arc concepts here at all) that sharing the
	dispatcher itself would need an awkward, indirect abstraction for
	very little actual code saved.
*/
bool DiagramImageItem::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
	if (watched->type() != QetGraphicsHandlerItem::Type)
		return false;

	QetGraphicsHandlerItem *qghi = qgraphicsitem_cast<QetGraphicsHandlerItem *>(watched);
	const int index = m_handler_vector.indexOf(qghi);
	if (index == -1)
		return false;

	if (event->type() == QEvent::GraphicsSceneMousePress)
	{
		handlerMousePressEvent(index, static_cast<QGraphicsSceneMouseEvent *>(event)->modifiers());
		return true;
	}
	if (event->type() == QEvent::GraphicsSceneMouseMove)
	{
		handlerMouseMoveEvent(index, static_cast<QGraphicsSceneMouseEvent *>(event));
		return true;
	}
	if (event->type() == QEvent::GraphicsSceneMouseRelease)
	{
		handlerMouseReleaseEvent(index);
		return true;
	}
	if (event->type() == QEvent::GraphicsSceneHoverEnter)
	{
		// On top of the tooltip Qt already shows natively from the
		// handle's own setToolTip() (see rebuildHandles()) -- returning
		// false leaves that native tooltip handling alone, matching
		// QetShapeItem's identical dual approach for its own handles.
		showStatusHint(hintForHandleRole(m_handleRoles.at(index)));
		return false;
	}
	if (event->type() == QEvent::GraphicsSceneHoverLeave)
	{
		clearStatusHint();
		return false;
	}
	return false;
}

/**
	@brief DiagramImageItem::showStatusHint
*/
void DiagramImageItem::showStatusHint(const QString &text) const
{
	if (text.isEmpty() || !diagram() || diagram()->views().isEmpty())
		return;
	if (auto *editor = QETApp::diagramEditorAncestorOf(diagram()->views().constFirst()))
		editor->statusBar()->showMessage(text);
}

/**
	@brief DiagramImageItem::clearStatusHint
*/
void DiagramImageItem::clearStatusHint() const
{
	if (!diagram() || diagram()->views().isEmpty())
		return;
	if (auto *editor = QETApp::diagramEditorAncestorOf(diagram()->views().constFirst()))
		editor->statusBar()->clearMessage();
}

/**
	@brief DiagramImageItem::handlerMousePressEvent
	For a Resize handle, temporarily repositions the pivot for the
	duration of this drag: to the OPPOSITE corner/edge by default (so
	that one stays fixed, the usual convention), or to the CENTER if
	Ctrl is held at the moment of the press (so the image grows
	symmetrically from its middle instead). Decided once here, not
	re-checked on every subsequent move -- mirroring the identical fix
	already made for shape creation, and for the identical reason:
	continuously re-checking made releasing the modifier mid-drag
	revert a decision that felt already made, rather than a deliberate
	choice made once at the start.

	Verified numerically before relying on this: with the pivot fixed
	at whichever reference point applies, solving for a new scale that
	puts the dragged handle at the mouse automatically keeps that
	reference point exactly where it was, with zero drift, across all 8
	handles and a rotated, non-uniformly-scaled starting transform.
	compensatedPositionForNewPivot() is what keeps this repositioning
	itself invisible -- without it, temporarily moving the pivot would
	visibly jump the image the instant the drag starts.
*/
void DiagramImageItem::handlerMousePressEvent(int index, Qt::KeyboardModifiers mods)
{
	m_vector_index = index;
	m_original_pos = pos();
	m_original_transform = m_transform;

	if (m_handleRoles.at(index) == HandleRole::Resize)
	{
		m_resizeCenterAnchored = mods & Qt::ControlModifier;
		const QPointF referencePoint = m_resizeCenterAnchored
				? QPointF(pixmap_.width() / 2.0, pixmap_.height() / 2.0)
				: handleNaturalPosition(oppositeHandleIndex(index), pixmap_.width(), pixmap_.height());
		const QPointF newPos = compensatedPositionForNewPivot(pos(), m_transform.pivot, referencePoint, m_transform.linearPart());
		m_deferHandleReposition = true;
		m_transform.pivot = referencePoint;
		// QGraphicsObject::setPos(), not the plain setPos() -- see
		// setPivot()'s identical comment for why: QetGraphicsItem's
		// override snaps to the grid, which would corrupt this exact
		// compensation the same way it did there.
		QGraphicsObject::setPos(newPos);
		setTransform(m_transform.toMatrix());
		m_deferHandleReposition = false;
	}
}

/**
	@brief DiagramImageItem::handlerMouseMoveEvent
*/
void DiagramImageItem::handlerMouseMoveEvent(int index, QGraphicsSceneMouseEvent *event)
{
	const HandleRole role = m_handleRoles.at(index);

	if (role == HandleRole::Resize)
		dragResize(index, mapFromScene(event->scenePos()), event->modifiers());
	else if (role == HandleRole::Rotate)
		dragRotateHandle(index, event->scenePos(), event->modifiers());          // index IS the corner slot (0-3): Rotate handles are always first
	else if (role == HandleRole::SkewEdge)
		dragSkewHandle(index - 4, event->scenePos(), event->modifiers());        // offset by the 4 preceding Rotate handles
	else if (role == HandleRole::Pivot)
		dragPivot(mapFromScene(event->scenePos()));
}

/**
	@brief DiagramImageItem::handlerMouseReleaseEvent
	Two responsibilities, in order: first, Resize's own pivot cleanup
	(the pivot was temporarily relocated to the opposite corner or
	center in handlerMousePressEvent() purely to make the drag math
	simple, and has to move back now that the drag is over, via
	resetPivotToBoundingRectCenter(), which itself handles the position
	compensation that reset needs) -- but only when the pivot isn't
	user-customized, matching dragPivot()'s own comment for why.
	Second -- and previously entirely missing -- building the actual
	undo command for whatever changed during this drag: every drag
	method above mutates m_transform directly and calls setTransform()
	immediately, live, with no undo tracking of its own at all, exactly
	like QetShapeItem's identical handlerMouseMoveEvent()/dragResize()
	pair. QetShapeItem's own handlerMouseReleaseEvent() is where that
	gets reconciled into a single undo step per role, comparing
	m_original_transform/m_original_pos (captured at press time) against
	the now-already-applied live values -- this is a direct port of
	that same pattern for this class's own, smaller role set. Pushing a
	command whose redo() sets a property to what it's already live at
	is intentional, not wasted: redo() runs once immediately, as a
	harmless no-op, so the stack has a correct entry for Ctrl+Z without
	the live drag needing any awareness of undo at all. For Pivot
	specifically, chaining "pos" before "rawPivot" -- not the
	compensating "pivot" -- means undoing simply restores both to their
	exact recorded values, without a second, redundant compensation
	needing to run and (correctly, but confusingly) arrive at the same
	place a longer way around.
*/
void DiagramImageItem::handlerMouseReleaseEvent(int index)
{
	const HandleRole role = m_handleRoles.at(index);

	if (role == HandleRole::Resize && !m_pivotIsCustom)
		resetPivotToBoundingRectCenter();

	if (diagram())
	{
		QUndoCommand *undo = nullptr;

		switch (role)
		{
			case HandleRole::Resize:
				if (!qFuzzyCompare(m_transform.scaleX, m_original_transform.scaleX))
					undo = new QPropertyUndoCommand(this, "scaleFactorX", m_original_transform.scaleX, m_transform.scaleX);
				if (!qFuzzyCompare(m_transform.scaleY, m_original_transform.scaleY))
				{
					if (undo)
						new QPropertyUndoCommand(this, "scaleFactorY", m_original_transform.scaleY, m_transform.scaleY, undo);
					else
						undo = new QPropertyUndoCommand(this, "scaleFactorY", m_original_transform.scaleY, m_transform.scaleY);
				}
				break;

			case HandleRole::Rotate:
				if (!qFuzzyCompare(m_transform.rotation, m_original_transform.rotation))
					undo = new QPropertyUndoCommand(this, "rotationAngle", m_original_transform.rotation, m_transform.rotation);
				break;

			case HandleRole::SkewEdge:
				if (!qFuzzyCompare(m_transform.skewX, m_original_transform.skewX))
					undo = new QPropertyUndoCommand(this, "skewX", m_original_transform.skewX, m_transform.skewX);
				else if (!qFuzzyCompare(m_transform.skewY, m_original_transform.skewY))
					undo = new QPropertyUndoCommand(this, "skewY", m_original_transform.skewY, m_transform.skewY);
				break;

			case HandleRole::Pivot:
				if (m_transform.pivot != m_original_transform.pivot)
				{
					undo = new QUndoCommand(tr("Déplacer le centre de rotation d'une image"));
					new QPropertyUndoCommand(this, "pos", m_original_pos, pos(), undo);
					new QPropertyUndoCommand(this, "rawPivot", m_original_transform.pivot, m_transform.pivot, undo);
				}
				break;
		}

		if (undo)
		{
			if (undo->text().isEmpty())
				undo->setText(tr("Modifier une image"));
			diagram()->undoStack().push(undo);
		}
	}

	m_vector_index = -1;
}

/**
	@brief DiagramImageItem::dragResize
	Verified numerically (all 8 handles, a rotated and non-uniformly
	scaled starting transform) before writing this: undoing only
	rotation from the target -- never scale, since that's the very
	value being solved for -- is what avoids the feedback-loop mistake
	a full mapFromScene()-based solve would fall into (mapFromScene()
	divides out the CURRENT scale, not the new one, since it doesn't
	know a new one is being computed at all).
*/
void DiagramImageItem::dragResize(int index, const QPointF &localPos, Qt::KeyboardModifiers mods)
{
	const QPointF pivotScene = pos() + m_transform.pivot;
	QTransform undoRotation;
	undoRotation.rotate(-m_transform.rotation);
	const QPointF postScaleOffset = undoRotation.map(mapToScene(localPos) - pivotScene);

	const QPointF handleNatural = handleNaturalPosition(index, pixmap_.width(), pixmap_.height());
	const qreal handleDx = handleNatural.x() - m_transform.pivot.x();
	const qreal handleDy = handleNatural.y() - m_transform.pivot.y();

	qreal newScaleX = m_transform.scaleX;
	qreal newScaleY = m_transform.scaleY;
	if (adjustsX(index) && qAbs(handleDx) > 1e-6)
		newScaleX = postScaleOffset.x() / handleDx;
	if (adjustsY(index) && qAbs(handleDy) > 1e-6)
		newScaleY = postScaleOffset.y() / handleDy;

	if (mods & Qt::ShiftModifier)
	{
		// Preserve the ORIGINAL aspect ratio (captured at press time in
		// m_original_transform), not force scaleX==scaleY -- an image
		// that wasn't square to begin with should stay proportional to
		// itself, not become literally square the moment Shift is held.
		const qreal originalRatio = (qFuzzyIsNull(m_original_transform.scaleY))
				? 1.0 : m_original_transform.scaleX / m_original_transform.scaleY;
		if (adjustsX(index) && !adjustsY(index))
			newScaleY = newScaleX / (qFuzzyIsNull(originalRatio) ? 1.0 : originalRatio);
		else if (adjustsY(index) && !adjustsX(index))
			newScaleX = newScaleY * originalRatio;
		else
		{
			// Corner handle: whichever axis moved further from its own
			// starting value, in relative terms, drives the other.
			const qreal relX = qAbs(newScaleX / m_original_transform.scaleX);
			const qreal relY = qAbs(newScaleY / m_original_transform.scaleY);
			if (relX > relY)
				newScaleY = newScaleX / (qFuzzyIsNull(originalRatio) ? 1.0 : originalRatio);
			else
				newScaleX = newScaleY * originalRatio;
		}
	}

	// A pixmap can't meaningfully render at zero or negative scale --
	// clamped well above zero rather than letting it vanish or silently
	// flip (negative scale is a mirror, and mirroring is already its
	// own deliberate, explicit context-menu action, not something a
	// fast drag past the opposite edge should trigger by accident).
	const qreal MIN_SCALE = 0.02;
	if (newScaleX < MIN_SCALE) newScaleX = MIN_SCALE;
	if (newScaleY < MIN_SCALE) newScaleY = MIN_SCALE;

	m_transform.scaleX = newScaleX;
	m_transform.scaleY = newScaleY;
	setTransform(m_transform.toMatrix());
	repositionHandles();
	emit transformChanged();
}

/**
	@brief DiagramImageItem::dragRotateHandle
	Verified numerically before writing this (all 4 corners, WITH
	nonzero skew already present -- scaleAndShearOffset() is what
	makes the reference direction correctly account for that): the
	handle ends up pointing exactly at the mouse's angle from the
	pivot, and the pivot itself never moves. Ported from
	QetShapeItem::dragRotateHandle() -- see that copy's own comment for
	why this is a plain assignment rather than a recurrence (computing
	the angle via mapFromScene() would divide out the current rotation
	and turn each frame's result into a feedback loop).
*/
void DiagramImageItem::dragRotateHandle(int cornerIndex, const QPointF &scenePos, Qt::KeyboardModifiers mods)
{
	const QPointF scenePivot = pos() + m_transform.pivot;
	const qreal angleMouse = qRadiansToDegrees(qAtan2(scenePos.y() - scenePivot.y(), scenePos.x() - scenePivot.x()));

	const QPointF reference = scaleAndShearOffset(cornerPosition(cornerIndex, pixmap_.width(), pixmap_.height()));
	const qreal angleReference = qRadiansToDegrees(qAtan2(reference.y(), reference.x()));

	qreal angle = angleMouse - angleReference;
	if (mods & Qt::ShiftModifier)
		angle = qRound(angle / 15.0) * 15.0;

	m_transform.rotation = angle;
	setTransform(m_transform.toMatrix());
	repositionHandles();
	emit transformChanged();
}

/**
	@brief DiagramImageItem::dragSkewHandle
	Verified numerically before writing this (all 4 edges, WITH nonzero
	rotation already present): recovers the exact skew angle that would
	put the edge handle at a given scene position, for both skewX
	(N/S edges) and skewY (E/W edges). Ported from
	QetShapeItem::dragSkewHandle() -- see that copy's own comment for
	the closed-form derivation (solved for the one skew component being
	dragged, holding everything else fixed, since going through
	mapFromScene() would divide out the very value being solved for).
*/
void DiagramImageItem::dragSkewHandle(int edgeIndex, const QPointF &scenePos, Qt::KeyboardModifiers mods)
{
	const QPointF pivot = m_transform.pivot;
	const QPointF Q = scaleOnlyOffset(edgeMidpointPosition(edgeIndex, pixmap_.width(), pixmap_.height()));

	const qreal rad = qDegreesToRadians(m_transform.rotation);
	const qreal c = qCos(rad), s = qSin(rad);
	const QPointF targetRel = scenePos - pos() - pivot;
	const QPointF M(targetRel.x() * c + targetRel.y() * s,
	               -targetRel.x() * s + targetRel.y() * c);

	qreal degrees;
	if (edgeIndex == 0 || edgeIndex == 2)   // N/S edge -> skewX
	{
		if (qFuzzyIsNull(Q.y())) return;
		degrees = qRadiansToDegrees(qAtan((M.x() - Q.x()) / Q.y()));
		if (mods & Qt::ShiftModifier) degrees = qRound(degrees / 15.0) * 15.0;
		m_transform.skewX = degrees;
	}
	else                                     // E/W edge -> skewY
	{
		if (qFuzzyIsNull(Q.x())) return;
		degrees = qRadiansToDegrees(qAtan((M.y() - Q.y()) / Q.x()));
		if (mods & Qt::ShiftModifier) degrees = qRound(degrees / 15.0) * 15.0;
		m_transform.skewY = degrees;
	}

	setTransform(m_transform.toMatrix());
	repositionHandles();
	emit transformChanged();
}

/**
	@brief DiagramImageItem::dragPivot
	Marks the pivot as user-customized, the same way QetShapeItem's own
	pivot handle does -- so a later resize (see handlerMouseReleaseEvent())
	doesn't silently snap it back to the bounding-rect center the
	instant the user deliberately moved it somewhere else.
*/
void DiagramImageItem::dragPivot(const QPointF &localPos)
{
	m_pivotIsCustom = true;
	setPivot(localPos);
	repositionHandles();
}

/**
	@brief DiagramImageItem::restoreAspectRatio
	Context-menu action, the direct answer to "restoration of aspect
	ratio" from the original wishlist: makes scaleY match scaleX,
	keeping the current width fixed. The natural aspect ratio is
	already fully accounted for by the pixmap's own width/height (a
	uniform scale, by definition, can never distort it) -- the
	distortion is entirely scaleX and scaleY disagreeing with each
	other, so undoing it is exactly "make them agree", not a
	computation involving pixmap_'s own dimensions at all. An earlier
	version of this multiplied by the natural height/width ratio a
	second time, which double-counted it and left the image still
	visibly distorted, just less obviously so.
*/
void DiagramImageItem::restoreAspectRatio()
{
	if (!diagram() || diagram()->isReadOnly())
		return;

	if (qFuzzyCompare(m_transform.scaleY, m_transform.scaleX))
		return;

	auto *undo = new QPropertyUndoCommand(this, "scaleFactorY", m_transform.scaleY, m_transform.scaleX);
	undo->setText(tr("Restaurer les proportions d'une image"));
	diagram()->undoStack().push(undo);
}

/**
	@brief DiagramImageItem::itemChange
*/
QVariant DiagramImageItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemSelectedHasChanged)
	{
		if (value.toBool())
			rebuildHandles();
		else
		{
			prepareGeometryChange();
			clearHandles();
			m_handleMode = HandleMode::Size;
		}
		refreshInteractionHints();
	}
	else if (change == ItemPositionHasChanged || change == ItemTransformHasChanged)
	{
		if (!m_deferHandleReposition)
			repositionHandles();
	}
	else if (change == ItemSceneHasChanged)
	{
		if (!scene())
			setSelected(false);
	}

	return QGraphicsItem::itemChange(change, value);
}

/**
	@brief DiagramImageItem::computeDisplayPixmap
	Re-derives what pixmap_ should be from first principles: crop the
	true original down to the chosen region, then colour-key whichever
	colours have been picked out of it. Used whenever crop() or
	setTransparentColor() changes one of those two independently, so
	the other's effect is correctly re-applied on top rather than lost
	or compounded -- cropping after colours were already picked has to
	still show them keyed out; picking colours after a crop has to only
	ever consider what's still actually part of the image.
	@param base the true, uncropped original
	@param cropRect the region of base to keep, in base's own coordinates
	@param colors colours to key transparent within the cropped region
	@param tolerance how loosely to match those colours, 0-100
*/
QPixmap DiagramImageItem::computeDisplayPixmap(const QPixmap &base, const QRect &cropRect, const QList<QColor> &colors, int tolerance)
{
	const QPixmap cropped = cropRect == base.rect() ? base : base.copy(cropRect);
	if (colors.isEmpty())
		return cropped;
	return QPixmap::fromImage(ImageTransparentColorDialog::applyColorKey(cropped.toImage(), colors, tolerance));
}

/**
	@brief DiagramImageItem::boundingRect
	the outer bounds of the item as a rectangle,
	if no pixmap are set, return a default QRectF
	@return a QRectF represent the bounding rectangle
*/
QRectF DiagramImageItem::boundingRect() const
{
	if (!pixmap_.isNull()) {
		return (QRectF(pixmap_.rect()));
	} else {
		QRectF bound;
		return (bound);
	}
}

/**
	@brief DiagramImageItem::name
	@return the generic name of this item (picture)
*/
QString DiagramImageItem::name() const
{
	return tr("une image");
}

/**
	@brief DiagramImageItem::fromXml
	Load this image from xml element e
	@param e
	@return true if successfully loaded.
*/
bool DiagramImageItem::fromXml(const QDomElement &e)
{
	if (e.tagName() != "image") {
		return (false);
	}
	
	QDomNode image_node = e.firstChild();
	if (!image_node.isText()) {
		return (false);
	}

	//load xml image to QByteArray
	QByteArray array;
	array = QByteArray::fromBase64(e.text().toLatin1());

	//Set QPixmap from the array
	QPixmap pixmap;
	pixmap.loadFromData(array);
	setPixmap(pixmap);

	// Falls back to treating the loaded result as its own base, with no
	// remembered crop or colours -- correct both for a genuinely plain
	// image and for a file saved before these features existed.
	// Overwritten below if the file actually does carry this
	// information.
	m_base_pixmap = pixmap;
	m_crop_rect = pixmap.rect();
	m_transparent_colors.clear();
	m_transparent_tolerance = 10;

	const QDomElement colorsElement = e.firstChildElement("transparent_colors");
	bool hasColors = !colorsElement.isNull();
	if (hasColors)
	{
		m_transparent_tolerance = colorsElement.attribute("tolerance", "10").toInt();
		for (const QDomElement &colorElement : QET::findInDomElement(colorsElement, "color"))
		{
			m_transparent_colors.append(QColor(
					colorElement.attribute("r").toInt(),
					colorElement.attribute("g").toInt(),
					colorElement.attribute("b").toInt()));
		}
	}

	const QDomElement cropElement = e.firstChildElement("crop");
	bool hasCrop = !cropElement.isNull();
	if (hasCrop)
	{
		m_crop_rect = QRect(
				cropElement.attribute("x").toInt(),
				cropElement.attribute("y").toInt(),
				cropElement.attribute("w").toInt(),
				cropElement.attribute("h").toInt());
	}

	// Present, and only meaningful, whenever either of the above is --
	// the base pixmap on its own, with no crop or colours to apply to
	// it, wouldn't mean anything.
	if (hasColors || hasCrop)
	{
		const QDomElement baseElement = e.firstChildElement("image_base");
		if (!baseElement.isNull())
		{
			const QByteArray baseArray = QByteArray::fromBase64(baseElement.text().toLatin1());
			QPixmap basePixmap;
			if (basePixmap.loadFromData(baseArray))
				m_base_pixmap = basePixmap;
		}
		// m_crop_rect may still refer to a saved file's base image, not
		// pixmap (used as a fallback above only when nothing better is
		// available) -- clamp it to whatever base actually ended up
		// loaded, so an inconsistent or hand-edited file can't produce
		// an out-of-bounds crop rect later.
		m_crop_rect = m_crop_rect.intersected(m_base_pixmap.rect());
		if (m_crop_rect.isEmpty())
			m_crop_rect = m_base_pixmap.rect();
	}

	// Baseline: the plain "rotation"/"size" attributes, understood by
	// every version of this code, past and future -- size applied
	// uniformly to both axes, since a single float can't say otherwise.
	// Overwritten below if the file actually carries the richer
	// <transform> element (independent scaleX/scaleY, and/or a custom
	// pivot), the same two-tier fallback QetShapeItem's own fromXml()
	// already uses for its identical <transform> element.
	m_transform.rotation = e.attribute("rotation").toDouble();
	m_transform.scaleX = e.attribute("size").toDouble();
	m_transform.scaleY = m_transform.scaleX;
	m_transform.pivot = boundingRect().center();
	m_pivotIsCustom = false;

	const QDomElement transformElement = e.firstChildElement("transform");
	if (!transformElement.isNull())
	{
		m_transform.rotation = transformElement.attribute("rotation", "0").toDouble();
		m_transform.skewX    = transformElement.attribute("skewX", "0").toDouble();
		m_transform.skewY    = transformElement.attribute("skewY", "0").toDouble();
		m_transform.scaleX   = transformElement.attribute("scaleX", "1").toDouble();
		m_transform.scaleY   = transformElement.attribute("scaleY", "1").toDouble();
		m_transform.pivot    = QPointF(transformElement.attribute("pivotX", "0").toDouble(),
		                                transformElement.attribute("pivotY", "0").toDouble());
		m_pivotIsCustom = true;
	}
	setTransform(m_transform.toMatrix());

		//We directly call setPos from QGraphicsObject, because QetGraphicsItem will snap to grid
	QGraphicsObject::setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());
	setZValue(e.attribute("z", QString::number(this->zValue())).toDouble());
	is_movable_ = (e.attribute("is_movable").toInt());

	return (true);
}

/**
	@param document Le document XML a utiliser
	@return L'element XML representant l'image
*/
QDomElement DiagramImageItem::toXml(QDomDocument &document) const
{
	QDomElement result = document.createElement("image");
	//write some attribute
	result.setAttribute("x", QString::number(pos().x()));
	result.setAttribute("y", QString::number(pos().y()));
	result.setAttribute("z", QString::number(this->zValue()));
	// m_transform.rotation/scaleX, not rotation()/scale(): those are
	// QGraphicsItem's own convenience properties, and this class no
	// longer ever calls their setters at all -- it uses setTransform()
	// directly now, the only way to represent independent scaleX/scaleY
	// at all, so rotation()/scale() would always read back their
	// defaults (0 and 1) regardless of the image's actual, visible
	// state. "size" keeps scaleX specifically (not some average of
	// scaleX/scaleY) as its best-effort value for older code that only
	// ever understood a single uniform scale -- the <transform> element
	// below is what any code that understands both axes independently
	// should prefer instead, exactly mirroring how QetShapeItem's own
	// otherwise-identical <transform> element already works.
	result.setAttribute("rotation", QString::number(QET::correctAngle(m_transform.rotation)));
	result.setAttribute("size", QString::number(m_transform.scaleX));
	result.setAttribute("is_movable", bool(is_movable_));

	//write the pixmap in the xml element after he was been transformed to base64
	QByteArray array;
	QBuffer buffer(&array);
	buffer.open(QIODevice::ReadWrite);
	pixmap_.save(&buffer, "PNG");
	QDomText base64 = document.createTextNode(array.toBase64());
	result.appendChild(base64);

	// Full-fidelity transform, only written when it actually carries
	// something the rotation/size attributes above can't already
	// express -- exactly the same convention QetShapeItem's own
	// identical <transform> element already uses, for the same reason:
	// this keeps a plain, never-resized-independently image's saved
	// file byte-for-byte unchanged.
	const bool hasRichTransform = !qFuzzyCompare(m_transform.scaleX, m_transform.scaleY)
			|| !qFuzzyIsNull(m_transform.skewX) || !qFuzzyIsNull(m_transform.skewY) || m_pivotIsCustom;
	if (hasRichTransform)
	{
		QDomElement transformElement = document.createElement("transform");
		transformElement.setAttribute("rotation", QString::number(m_transform.rotation));
		transformElement.setAttribute("skewX", QString::number(m_transform.skewX));
		transformElement.setAttribute("skewY", QString::number(m_transform.skewY));
		transformElement.setAttribute("scaleX", QString::number(m_transform.scaleX));
		transformElement.setAttribute("scaleY", QString::number(m_transform.scaleY));
		transformElement.setAttribute("pivotX", QString::number(m_transform.pivot.x()));
		transformElement.setAttribute("pivotY", QString::number(m_transform.pivot.y()));
		result.appendChild(transformElement);
	}

	// Only written when there's actually something to remember -- a
	// plain image that's never been cropped or had transparency applied
	// shouldn't carry this extra weight at all. pixmap_ above already
	// reflects the current result on its own, so older code (or a file
	// that never used either feature) reads back exactly what it
	// always did; this is purely additional context for the dialogs'
	// own memory, letting them be reopened non-destructively.
	const bool hasCrop = (m_crop_rect != m_base_pixmap.rect());
	const bool hasColors = !m_transparent_colors.isEmpty();

	if (hasColors)
	{
		QDomElement colorsElement = document.createElement("transparent_colors");
		colorsElement.setAttribute("tolerance", m_transparent_tolerance);
		for (const QColor &color : m_transparent_colors)
		{
			QDomElement colorElement = document.createElement("color");
			colorElement.setAttribute("r", color.red());
			colorElement.setAttribute("g", color.green());
			colorElement.setAttribute("b", color.blue());
			colorsElement.appendChild(colorElement);
		}
		result.appendChild(colorsElement);
	}

	if (hasCrop)
	{
		QDomElement cropElement = document.createElement("crop");
		cropElement.setAttribute("x", m_crop_rect.x());
		cropElement.setAttribute("y", m_crop_rect.y());
		cropElement.setAttribute("w", m_crop_rect.width());
		cropElement.setAttribute("h", m_crop_rect.height());
		result.appendChild(cropElement);
	}

	if (hasCrop || hasColors)
	{
		QByteArray baseArray;
		QBuffer baseBuffer(&baseArray);
		baseBuffer.open(QIODevice::ReadWrite);
		m_base_pixmap.save(&baseBuffer, "PNG");
		QDomElement baseElement = document.createElement("image_base");
		baseElement.appendChild(document.createTextNode(baseArray.toBase64()));
		result.appendChild(baseElement);
	}

	return(result);
}

/**
	@brief DiagramImageItem::contextMenuEvent
	@param event
*/
void DiagramImageItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	if (!diagram())
	{
		QetGraphicsItem::contextMenuEvent(event);
		return;
	}

	if (diagram()->selectedItems().isEmpty())
		this->setSelected(true);

	if (isSelected() && scene()->selectedItems().size() == 1)
	{
		DiagramView *d_view = nullptr;
		for (QGraphicsView *view : diagram()->views())
		{
			if (view->isActiveWindow())
			{
				d_view = dynamic_cast<DiagramView *>(view);
				if (d_view)
					continue;
			}
		}

		if (d_view)
		{
			QScopedPointer<QMenu> menu(new QMenu());

			QAction *replace = menu.data()->addAction(tr("Remplacer l'image..."));
			connect(replace, &QAction::triggered, this, &DiagramImageItem::replaceImage);

			QAction *transparentColor = menu.data()->addAction(tr("Couleur transparente..."));
			connect(transparentColor, &QAction::triggered, this, &DiagramImageItem::setTransparentColor);

			QAction *cropAction = menu.data()->addAction(tr("Rogner..."));
			connect(cropAction, &QAction::triggered, this, &DiagramImageItem::crop);

			QAction *mirrorH = menu.data()->addAction(tr("Miroir horizontal"));
			QAction *mirrorV = menu.data()->addAction(tr("Miroir vertical"));
			connect(mirrorH, &QAction::triggered, this, [this]() { mirror(true); });
			connect(mirrorV, &QAction::triggered, this, [this]() { mirror(false); });

			QAction *restoreRatio = menu.data()->addAction(tr("Restaurer les proportions"));
			connect(restoreRatio, &QAction::triggered, this, &DiagramImageItem::restoreAspectRatio);

			menu.data()->addSeparator();
			QAction *properties = menu.data()->addAction(tr("Propriétés..."));
			connect(properties, &QAction::triggered, this, &DiagramImageItem::editProperty);

			menu.data()->addSeparator();
			menu.data()->addActions(d_view->contextMenuActions());
			menu.data()->exec(event->screenPos());
			event->accept();
			return;
		}
	}

	QetGraphicsItem::contextMenuEvent(event);
}

/**
	@brief DiagramImageItem::replaceImage
	Context-menu action: swaps the underlying pixmap for one loaded from
	a new file, keeping position, rotation and scale untouched -- only
	pixmap_ changes, everything else about how this item sits on the
	diagram is left exactly as it was. Reuses the same file dialog
	filter and error handling as DiagramEventAddImage::openDialog(), so
	picking a replacement looks and behaves like picking a new image did
	when this item was first inserted.
*/
void DiagramImageItem::replaceImage()
{
	if (!diagram() || diagram()->isReadOnly())
		return;

	QWidget *parentWidget = diagram()->views().isEmpty() ? nullptr : diagram()->views().first();
	const QString fileName = QFileDialog::getOpenFileName(
			parentWidget, tr("Selectionner une image..."),
			QETApp::pictureDir(), tr("Image Files (*.png *.jpg *.jpeg *.bmp *.svg)"));
	if (fileName.isEmpty())
		return;

	QImage image(fileName);
	if (image.isNull())
	{
		QMessageBox::critical(parentWidget, tr("Erreur"), tr("Impossible de charger l'image."));
		return;
	}

	const QPixmap oldPixmap = pixmap_;
	const QPixmap newPixmap = QPixmap::fromImage(image);

	// A wholesale replacement, not an edit of the existing image -- the
	// new picture has nothing to do with whatever colours were picked
	// or region was cropped for the old one, so this starts that
	// memory fresh rather than carrying over choices that would no
	// longer make sense.
	m_base_pixmap = newPixmap;
	m_crop_rect = newPixmap.rect();
	m_transparent_colors.clear();

	auto *undo = new QPropertyUndoCommand(this, "pixmap", oldPixmap, newPixmap);
	undo->setText(tr("Remplacer une image"));
	diagram()->undoStack().push(undo);
}

/**
	@brief DiagramImageItem::mirror
	Context-menu action: flips the pixmap itself, not a transform --
	unlike QetShapeItem::mirror(), which has rotation/skew to contend
	with, there's no equivalent linear-transform decomposition needed
	here: the bitmap is flipped once, directly, and stays flipped
	regardless of whatever rotation is applied on top afterward.
*/
void DiagramImageItem::mirror(bool horizontal)
{
	if (!diagram() || diagram()->isReadOnly())
		return;

	const QPixmap oldPixmap = pixmap_;
	const QTransform flip = horizontal ? QTransform(-1, 0, 0, 1, 0, 0) : QTransform(1, 0, 0, -1, 0, 0);
	const QPixmap newPixmap = pixmap_.transformed(flip);

	// The base is flipped the same way, to stay in sync with pixmap_ --
	// but the picked-colours list itself is left untouched: the actual
	// colour values don't change when the image is mirrored, only their
	// positions, so whatever was already keyed transparent should stay
	// remembered and still apply correctly to the flipped version.
	m_base_pixmap = m_base_pixmap.transformed(flip);

	// m_crop_rect, unlike the colour list, DOES need to change: it's
	// defined in terms of positions within the base, and those
	// positions just moved. Width/height and the other axis are
	// untouched -- only the axis being flipped needs its edge mirrored
	// (dimensions are unaffected by the flip, so it doesn't matter
	// whether this reads m_base_pixmap's size from before or after the
	// assignment above).
	if (horizontal)
		m_crop_rect = QRect(m_base_pixmap.width() - m_crop_rect.left() - m_crop_rect.width(),
				m_crop_rect.top(), m_crop_rect.width(), m_crop_rect.height());
	else
		m_crop_rect = QRect(m_crop_rect.left(), m_base_pixmap.height() - m_crop_rect.top() - m_crop_rect.height(),
				m_crop_rect.width(), m_crop_rect.height());

	auto *undo = new QPropertyUndoCommand(this, "pixmap", oldPixmap, newPixmap);
	undo->setText(horizontal ? tr("Miroir horizontal d'une image") : tr("Miroir vertical d'une image"));
	diagram()->undoStack().push(undo);
}

/**
	@brief DiagramImageItem::setTransparentColor
	Context-menu action: opens ImageTransparentColorDialog against
	m_base_pixmap (the pristine source), pre-populated with whatever
	colours and tolerance were remembered from a previous session --
	both problems fixed together, since they had the same root cause:
	passing pixmap_ (the already colour-keyed result) as if it were the
	source, with nowhere to remember which colours produced it. Applies
	the result the same way replaceImage() and mirror() do, through the
	"pixmap" property, so undo/redo stays consistent across all three;
	m_base_pixmap itself is deliberately left untouched here, since this
	action only ever changes which colours are keyed out of it, not the
	source those colours are keyed out of.
*/
/**
	@brief DiagramImageItem::setTransparentColor
	Context-menu action: opens ImageTransparentColorDialog against the
	CROPPED base (m_base_pixmap.copy(m_crop_rect)), not the full,
	uncropped original -- picking a colour from a region that's already
	been permanently cropped away would be picking a colour that isn't
	even part of the image anymore. Pre-populated with whatever colours
	and tolerance were remembered from a previous session. Applies the
	result the same way replaceImage() and mirror() do, through the
	"pixmap" property, so undo/redo stays consistent across all three;
	m_base_pixmap and m_crop_rect are deliberately left untouched here,
	since this action only ever changes which colours are keyed out,
	never the source region they're keyed out of.
*/
void DiagramImageItem::setTransparentColor()
{
	if (!diagram() || diagram()->isReadOnly())
		return;

	QWidget *parentWidget = diagram()->views().isEmpty() ? nullptr : diagram()->views().first();
	const QPixmap croppedBase = m_base_pixmap.copy(m_crop_rect);
	ImageTransparentColorDialog dialog(croppedBase, m_transparent_colors, m_transparent_tolerance, parentWidget);
	if (dialog.exec() != QDialog::Accepted)
		return;

	m_transparent_colors = dialog.pickedColors();
	m_transparent_tolerance = dialog.tolerance();

	const QPixmap oldPixmap = pixmap_;
	const QPixmap newPixmap = dialog.resultPixmap();

	auto *undo = new QPropertyUndoCommand(this, "pixmap", oldPixmap, newPixmap);
	undo->setText(tr("Définir une couleur transparente"));
	diagram()->undoStack().push(undo);
}

/**
	@brief DiagramImageItem::crop
	Context-menu action: opens ImageCropDialog against pixmap_ (the
	current, already colour-keyed display, so cropping is WYSIWYG
	against whatever is actually visible), then applies the chosen
	rectangle to both pixmap_ and m_base_pixmap together -- kept in
	sync the same way mirror() keeps them in sync, since cropping is a
	permanent, geometric change to the image's own content, unlike
	setTransparentColor()'s non-destructive colour keying.

	pos() also needs adjusting, not just pixmap_: setPixmap() (called
	via the "pixmap" undo command below) recomputes
	transformOriginPoint() from the new, smaller boundingRect(), but
	pos() itself is untouched by that -- without fixing it up here too,
	the surviving content would visually jump to wherever local (0,0)
	happens to land after shrinking, rather than staying exactly where
	it already was. Chained into one undo step together with the pixmap
	change, since undoing a crop has to restore both, or the restored
	(larger) image ends up in the wrong place.
*/
/**
	@brief DiagramImageItem::crop
	Context-menu action: opens ImageCropDialog against m_base_pixmap
	(the true, uncropped original), pre-populated with whatever crop
	rectangle was chosen in a previous session -- re-editable, not
	destructive: nothing about the original content is ever discarded,
	only which region of it is currently being shown, exactly the same
	principle setTransparentColor() already follows for its own choices.
	Recomputes pixmap_ via computeDisplayPixmap() so any already-picked
	transparent colours are correctly re-applied to the newly-cropped
	region, rather than lost (the crop dialog itself knows nothing
	about them).

	pos() also needs adjusting, not just pixmap_: setPixmap() (called
	via the "pixmap" undo command below) recomputes
	transformOriginPoint() from the new boundingRect(), but pos() itself
	is untouched by that -- without fixing it up here too, the
	surviving content would visually jump to wherever local (0,0) ends
	up after the crop rect changes, rather than staying exactly where
	it already was. This has to work whether this is the first crop
	ever applied or an adjustment of an existing one, so the position
	math is always done relative to the CURRENT crop rect (m_crop_rect,
	before it's updated below) -- when there's no previous crop, that's
	simply the whole base, which is what the very first version of this
	method assumed unconditionally.

	Chained into one undo step together with the pixmap change, since
	undoing a crop has to restore both, or the restored (larger) image
	ends up in the wrong place.
*/
void DiagramImageItem::crop()
{
	if (!diagram() || diagram()->isReadOnly())
		return;

	QWidget *parentWidget = diagram()->views().isEmpty() ? nullptr : diagram()->views().first();
	ImageCropDialog dialog(m_base_pixmap, m_crop_rect, parentWidget);
	if (dialog.exec() != QDialog::Accepted)
		return;

	const QRect newCropRect = dialog.cropRect();
	if (newCropRect.isEmpty() || newCropRect == m_crop_rect)
		return;   // nothing actually changed

	// newCropRect is in m_base_pixmap's own coordinates; converting its
	// center into the CURRENT local space (pixmap_'s own coordinates,
	// i.e. relative to the OLD m_crop_rect's own top-left) before
	// mapping to the scene through the transform that's still active
	// right now, prior to anything below changing it.
	const QPointF newCropCenterInCurrentLocal = QRectF(newCropRect).center() - QPointF(m_crop_rect.topLeft());
	const QPointF cropCenterScene = mapToScene(newCropCenterInCurrentLocal);
	const QPointF oldPos = pos();

	const QPixmap oldPixmap = pixmap_;
	const QPixmap newPixmap = computeDisplayPixmap(m_base_pixmap, newCropRect, m_transparent_colors, m_transparent_tolerance);
	m_crop_rect = newCropRect;

	// boundingRect() is exactly QRectF(pixmap_.rect()) (confirmed by
	// reading the actual implementation, not assumed) -- so the new
	// origin point can be derived directly from newPixmap here. Unlike
	// before this class gained a proper transform, setPixmap() no
	// longer manages the pivot automatically at all (see its own
	// comment for why) -- crop() now has to set it explicitly itself,
	// chained into the same undo step as the pixmap and position
	// changes, since all three genuinely change together here.
	const QPointF oldPivot = m_transform.pivot;
	const QPointF newOriginPoint = QRectF(newPixmap.rect()).center();
	const QPointF newPos = cropCenterScene - newOriginPoint;

	auto *undo = new QPropertyUndoCommand(this, "pixmap", oldPixmap, newPixmap);
	undo->setText(tr("Rogner une image"));
	new QPropertyUndoCommand(this, "pos", oldPos, newPos, undo);
	new QPropertyUndoCommand(this, "rawPivot", oldPivot, newOriginPoint, undo);
	m_pivotIsCustom = false;
	diagram()->undoStack().push(undo);
}
