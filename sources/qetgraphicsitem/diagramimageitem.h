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
#ifndef DIAGRAM_IMAGE_ITEM_H
#define DIAGRAM_IMAGE_ITEM_H

#include "qetgraphicsitem.h"
#include "shapetransform.h"

#include <QColor>
#include <QList>
#include <QVector>

class QDomElement;
class QDomDocument;
class QGraphicsSceneContextMenuEvent;
class QetGraphicsHandlerItem;

/**
	This class represents a selectable, movable and editable image on a
	diagram.
	@see QGraphicsItem::GraphicsItemFlags
*/
class DiagramImageItem : public QetGraphicsItem {
	Q_OBJECT
	Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap NOTIFY pixmapChanged)
	Q_PROPERTY(qreal scaleFactorX READ scaleFactorX WRITE setScaleFactorX NOTIFY transformChanged)
	Q_PROPERTY(qreal scaleFactorY READ scaleFactorY WRITE setScaleFactorY NOTIFY transformChanged)
	Q_PROPERTY(qreal rotationAngle READ rotationAngle WRITE setRotationAngle NOTIFY transformChanged)
	Q_PROPERTY(qreal skewX READ skewX WRITE setSkewX NOTIFY transformChanged)
	Q_PROPERTY(qreal skewY READ skewY WRITE setSkewY NOTIFY transformChanged)
	Q_PROPERTY(QPointF pivot READ pivot WRITE setPivot NOTIFY transformChanged)
	// A second, deliberately non-compensating property on the SAME
	// underlying value -- setPivot() (above) intentionally adjusts
	// pos() to keep the image visually in place, which is exactly
	// wrong for crop()'s own undo chain: crop() already computes the
	// correct final pos() itself (accounting for the crop, not just
	// the pivot move) and pushes it as its own separate "pos" command,
	// so a chained "pivot" step going through the compensating setter
	// would silently overwrite that already-correct pos() a second
	// time. rawPivot exists solely for that one caller.
	Q_PROPERTY(QPointF rawPivot READ pivot WRITE setPivotRaw NOTIFY transformChanged)

	// constructors, destructor
	public:
	DiagramImageItem(QetGraphicsItem * = nullptr);
	DiagramImageItem(const QPixmap &pixmap, QetGraphicsItem * = nullptr);
	~DiagramImageItem() override;
	
	// attributes
	public:
	enum { Type = UserType + 1007 };

	// A deliberately smaller, image-specific set than QetShapeItem's own
	// HandleMode/HandleRole: images have no path/polygon/arc concepts at
	// all, so reusing those enums directly would pull in a great deal of
	// irrelevant baggage for no benefit. RotateSkew's math (rotate,
	// skew, and their shared scaleAndShearOffset()/scaleOnlyOffset()
	// helpers) mirrors QetShapeItem's own identical formulas -- adapted
	// to this class's own, simpler 4-corner/4-edge/1-pivot handle set,
	// not shared code, since the two classes' broader handle roles
	// differ too much (no path/polygon/arc concepts here) for sharing
	// the dispatcher itself to be worth an indirect abstraction.
	enum class HandleMode { Size, RotateSkew };
	enum class HandleRole { Resize, Rotate, SkewEdge, Pivot };
	
	// methods
	public:
	/**
		Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a
		DiagramImageItem
		@return the QGraphicsItem type
	*/
	int type() const override { return Type; }
	
	virtual bool fromXml(const QDomElement &);
	virtual QDomElement toXml(QDomDocument &) const;
	void editProperty() override;
	void setPixmap(const QPixmap &pixmap);
	QPixmap pixmap() const { return pixmap_; }
	QRectF boundingRect() const override;
	QString name() const override;

	qreal scaleFactorX() const { return m_transform.scaleX; }
	qreal scaleFactorY() const { return m_transform.scaleY; }
	void setScaleFactorX(qreal factor);
	void setScaleFactorY(qreal factor);
	qreal rotationAngle() const { return m_transform.rotation; }
	void setRotationAngle(qreal angle);
	qreal skewX() const { return m_transform.skewX; }
	void setSkewX(qreal skew);
	qreal skewY() const { return m_transform.skewY; }
	void setSkewY(qreal skew);
	QPointF pivot() const { return m_transform.pivot; }
	void setPivot(const QPointF &pivot);
	void setPivotRaw(const QPointF &pivot);

	signals:
	void pixmapChanged();
	void transformChanged();

	protected:
	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override;
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
	void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
	void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
	bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;
	QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

	private:
	void replaceImage();
	void mirror(bool horizontal);
	void setTransparentColor();
	void crop();
	void restoreAspectRatio();
	static QPixmap computeDisplayPixmap(const QPixmap &base, const QRect &cropRect, const QList<QColor> &colors, int tolerance);

	void toggleHandleMode();
	HandleMode nextHandleMode() const;
	static QString handleModeLabel(HandleMode mode);
	void updateModeHint();
	void refreshInteractionHints();
	QString currentModeStatusHint() const;
	void rebuildHandles();
	void repositionHandles();
	void clearHandles();
	void resetPivotToBoundingRectCenter();
	void handlerMousePressEvent(int index, Qt::KeyboardModifiers mods);
	void handlerMouseMoveEvent(int index, QGraphicsSceneMouseEvent *event);
	void handlerMouseReleaseEvent(int index);
	void dragResize(int index, const QPointF &localPos, Qt::KeyboardModifiers mods);
	void dragRotateHandle(int cornerIndex, const QPointF &scenePos, Qt::KeyboardModifiers mods);
	void dragSkewHandle(int edgeIndex, const QPointF &scenePos, Qt::KeyboardModifiers mods);
	void dragPivot(const QPointF &localPos);
	QPointF scaleOnlyOffset(const QPointF &localPoint) const;
	QPointF scaleAndShearOffset(const QPointF &localPoint) const;
	QPointF handlePosition(int index) const;
	static QPointF cornerPosition(int cornerIndex, qreal w, qreal h);
	static QPointF edgeMidpointPosition(int edgeIndex, qreal w, qreal h);
	static QColor colorForHandleRole(HandleRole role);
	static QString hintForHandleRole(HandleRole role);
	void showStatusHint(const QString &text) const;
	void clearStatusHint() const;

	protected:
	QPixmap pixmap_;
	// The true, pristine original -- never itself cropped or colour-
	// keyed. pixmap_ (the displayed result) is always re-derived from
	// this plus m_crop_rect and m_transparent_colors/tolerance, via
	// computeDisplayPixmap(). Without keeping this separate, re-opening
	// either the crop or transparency dialog after using the other
	// would show an already-modified image as if it were the source --
	// areas already cropped away or coloured out would be gone for
	// good, with no way to recover or adjust them, only start over.
	// Updated by whatever genuinely replaces or reorients the image's
	// actual content (construction, replaceImage(), and mirror(), which
	// also mirrors m_crop_rect to keep referring to the same region of
	// the now-flipped base) -- never by crop() or setTransparentColor()
	// themselves, which only ever change which subset of this base is
	// shown.
	QPixmap m_base_pixmap;
	QRect m_crop_rect;   // relative to m_base_pixmap; equals m_base_pixmap.rect() when nothing has been cropped
	QList<QColor> m_transparent_colors;
	int m_transparent_tolerance = 10;

	// Independent scaleX/scaleY here is the actual point of this whole
	// member: QGraphicsItem::scale() is a single, uniform float, which
	// is exactly why an image could never break its own aspect ratio
	// before this. skewX/skewY exist in the struct but are never
	// written by anything below -- deliberately deferred, not an
	// oversight (see the .cpp for why).
	ShapeTransform m_transform;
	bool m_pivotIsCustom = false;
	HandleMode m_handleMode = HandleMode::Size;
	QVector<QetGraphicsHandlerItem *> m_handler_vector;
	QVector<HandleRole> m_handleRoles;
	int m_vector_index = -1;
	QPointF m_original_pos;   // scene position at the start of a resize/rotate/pivot drag, for Escape-to-cancel
	ShapeTransform m_original_transform;
	bool m_deferHandleReposition = false;   // see setPivot()'s comment
	bool m_resizeCenterAnchored = false;   // decided once, at press time -- see handlerMousePressEvent()'s comment for why, mirroring the identical fix already made for shape creation
};
#endif
