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
#ifndef QETSHAPEITEM_H
#define QETSHAPEITEM_H

#include "../QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "qetgraphicsitem.h"
#include "shapetransform.h"

#include <QPen>
#include <optional>
#include <utility>

class QDomElement;
class QDomDocument;
class QetGraphicsHandlerItem;
class QAction;

/**
	@brief The QetShapeItem class
	this class is used to draw a basic shape (line, rectangle, ellipse,
	polygon or free-form path) into a diagram, that can be saved to a
	.qet file. Beyond its local geometry, a shape may also carry a
	rotation/skew/scale (see ShapeTransform) around an arbitrary pivot.
*/
class QetShapeItem : public QetGraphicsItem
{
	Q_OBJECT

	Q_PROPERTY(QPen pen READ pen WRITE setPen NOTIFY penChanged)
	Q_PROPERTY(QBrush brush READ brush WRITE setBrush NOTIFY brushChanged)
	Q_PROPERTY(QRectF rect READ rect WRITE setRect)
	Q_PROPERTY(QLineF line READ line WRITE setLine)
	Q_PROPERTY(QPolygonF polygon READ polygon WRITE setPolygon)
	Q_PROPERTY(bool close READ isClosed WRITE setClosed NOTIFY closeChanged)
	Q_PROPERTY(qreal xRadius READ XRadius WRITE setXRadius NOTIFY XRadiusChanged)
	Q_PROPERTY(qreal yRadius READ YRadius WRITE setYRadius NOTIFY YRadiusChanged)

	// One property per ShapeTransform scalar -- this is what lets a plain
	// QPropertyUndoCommand(this, "rotation", oldValue, newValue) work for
	// every handle, exactly like xRadius/yRadius already do for corner
	// rounding.
	Q_PROPERTY(qreal rotation READ rotation WRITE setRotation NOTIFY transformChanged)
	Q_PROPERTY(qreal skewX READ skewX WRITE setSkewX NOTIFY transformChanged)
	Q_PROPERTY(qreal skewY READ skewY WRITE setSkewY NOTIFY transformChanged)
	Q_PROPERTY(qreal scaleFactorX READ scaleFactorX WRITE setScaleFactorX NOTIFY transformChanged)
	Q_PROPERTY(qreal scaleFactorY READ scaleFactorY WRITE setScaleFactorY NOTIFY transformChanged)
	Q_PROPERTY(QPointF pivot READ pivot WRITE setPivot NOTIFY transformChanged)

	Q_PROPERTY(qreal startAngle READ startAngle WRITE setStartAngle NOTIFY arcChanged)
	Q_PROPERTY(qreal endAngle READ endAngle WRITE setEndAngle NOTIFY arcChanged)

	signals:
		void penChanged();
		void brushChanged();
		void closeChanged();
		void XRadiusChanged();
		void YRadiusChanged();
		void transformChanged();
		void arcChanged();
		void geometryChanged();   // P1/P2, polygon points, or path nodes changed -- lets the properties panel stay in sync while a handle is dragged, not just when it's typed into

	public:
		enum ShapeType {Line	  =1,
						Rectangle =2,
						Ellipse	  =4,
						Polygon   =8,
						Path      =16 };
		Q_ENUM (ShapeType)

		enum ArcClosure {NoClosure = 0, Chord = 1, Pie = 2};
		Q_ENUM (ArcClosure)

		// Point of a Path shape. Anchors are in the same local coordinate
		// frame as an ordinary Polygon's points; handle offsets are stored
		// *relative to the anchor*, so moving a node never has to rewrite
		// its own handle coordinates.
		enum class NodeKind {Corner, Smooth, Symmetric};
		struct PathNode {
			QPointF anchor;
			NodeKind kind = NodeKind::Corner;
			std::optional<QPointF> inHandle;
			std::optional<QPointF> outHandle;

			bool operator==(const PathNode &other) const {
				return anchor == other.anchor && kind == other.kind
					&& inHandle == other.inHandle && outHandle == other.outHandle;
			}
		};

		// Orthogonal to ShapeType: which handle set is currently shown.
		// Cycled by clicking an already-selected shape without dragging:
		// Size -> Corner -> RotateSkew -> Size for Rectangle (the only
		// type with a corner-radius concept); Size -> NodeEdit ->
		// RotateSkew -> Size for Path (reveals control handles for
		// every node that has any); Size -> RotateSkew -> Size for
		// everything else. One unified click-cycle for every shape
		// type, rather than a separate, less discoverable gesture
		// (e.g. double-click) for any one shape's extra mode.
		enum class HandleMode {Size, Corner, NodeEdit, RotateSkew};

		// index conventions, deliberately matched to what already exists
		// rather than invented fresh:
		//   Resize       0..7, same order as QetGraphicsHandlerUtility::pointsForRect
		//                (this is exactly today's Rectangle/Ellipse handle set --
		//                 only the Ctrl/Shift dispatch around it is new)
		//   Rotate       0..3, corners: NW, NE, SE, SW
		//   SkewEdge     0..3, edges:   N,  E,  S,  W
		//   CornerRadius 0..1, same order as QetGraphicsHandlerUtility::pointForRadiusRect
		//   ArcEndpoint  0 = start angle, 1 = end angle
		enum class HandleRole {
			Resize,                     // Size mode
			Rotate, SkewEdge, Pivot,    // RotateSkew mode
			CornerRadius,               // Rectangle, always shown alongside Size handles
			ArcEndpoint,                // Ellipse, always shown
			PathAnchor, PathControlIn, PathControlOut  // Polygon/Path, node-edit mode (see setPathNodes())
		};

		enum { Type = UserType + 1008 };

		QetShapeItem(
				QPointF,
				QPointF = QPointF(0,0),
				ShapeType = Line,
				QGraphicsItem *parent = nullptr);
		~QetShapeItem() override;

		//Enable the use of qgraphicsitem_cast to safely cast a
		//QGraphicsItem into a QetShapeItem return the QGraphicsItem type
		int type() const override { return Type; }

			///METHODS
		QPen pen() const {return m_pen;}
		void setPen(const QPen &pen);
		QBrush brush() const {return m_brush;}
		void setBrush(const QBrush &brush);
		ShapeType shapeType() const {return m_shapeType;}

		virtual bool	    fromXml (const QDomElement &);
		virtual QDomElement toXml (QDomDocument &document) const;
		virtual bool toDXF (const QString &filepath,const QPen &pen);

		void editProperty() override;
		QString name() const override;

		void setP2      (const QPointF &P2);
		QLineF line() const{return QLineF(m_P1, m_P2);}
		bool setLine    (const QLineF &line);
		QRectF rect() const{return QRectF(m_P1, m_P2);}
		bool setRect    (const QRectF &rect);
		QPolygonF polygon() const {return m_polygon;}
		bool setPolygon (const QPolygonF &polygon);
		bool isClosed() const {return m_closed;}
		void setClosed (bool close);
		qreal XRadius() const {return m_xRadius;}
		void setXRadius(qreal X);
		qreal YRadius() const {return m_yRadius;}
		void setYRadius(qreal Y);

			//Transform: one accessor pair per ShapeTransform scalar (see
			//the Q_PROPERTY block above for why they are not grouped into
			//a single property).
		const ShapeTransform &shapeTransform() const {return m_transform;}
		qreal rotation() const {return m_transform.rotation;}
		void setRotation(qreal degrees);
		qreal skewX() const {return m_transform.skewX;}
		void setSkewX(qreal degrees);
		qreal skewY() const {return m_transform.skewY;}
		void setSkewY(qreal degrees);
		qreal scaleFactorX() const {return m_transform.scaleX;}
		void setScaleFactorX(qreal factor);
		qreal scaleFactorY() const {return m_transform.scaleY;}
		void setScaleFactorY(qreal factor);
		QPointF pivot() const {return m_transform.pivot;}
		void setPivot(const QPointF &pivot); // moves the pivot handle: compensates pos() so the shape does not jump
		void resetPivotToBoundingRectCenter();
		void enableNodeEditMode();   // Path only: switches to NodeEdit mode, so every node's control handles become visible

			//Arc: only meaningful when shapeType() == Ellipse. A full
			//ellipse is just an arc with span 360 -- there is no separate
			//Arc shape type.
		qreal startAngle() const {return m_startAngle;}
		void setStartAngle(qreal degrees);
		qreal endAngle() const {return m_endAngle;}
		void setEndAngle(qreal degrees);
		qreal spanAngle() const {return m_endAngle - m_startAngle;}
		bool isFullEllipse() const {return qFuzzyCompare(qAbs(spanAngle()), qreal(360));}
		ArcClosure arcClosure() const {return m_arcClosure;}
		void setArcClosure(ArcClosure closure);

			//Path (Bezier): only meaningful when shapeType() == Path.
		const QVector<PathNode> &pathNodes() const {return m_nodes;}
		void setPathNodes(const QVector<PathNode> &nodes);

			//Methods available for polygon shape
		int  pointsCount  () const;
		void setNextPoint (QPointF P);
		void removePoints (int number = 1);

		QRectF boundingRect() const override;
		QPainterPath shape()  const override;

	protected:
		void paint(
				QPainter *painter,
				const QStyleOptionGraphicsItem *option,
				QWidget *widget) override;
		void hoverEnterEvent (QGraphicsSceneHoverEvent *event) override;
		void hoverLeaveEvent (QGraphicsSceneHoverEvent *event) override;
		void mousePressEvent (QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent (QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent (QGraphicsSceneMouseEvent *event) override;
		QVariant itemChange(
				GraphicsItemChange change,
				const QVariant &value) override;
		bool sceneEventFilter(
				QGraphicsItem *watched,
				QEvent *event) override;
		void contextMenuEvent(
				QGraphicsSceneContextMenuEvent *event) override;

	private:
		void toggleHandleMode();
		HandleMode nextHandleMode() const;   // what a click would switch to from here -- shared by toggleHandleMode() and the tooltip
		void updateModeHint();               // keeps the tooltip in sync with nextHandleMode()
		void refreshInteractionHints();       // updateModeHint(), plus an immediate re-show of tooltip/status bar if currently hovered
		void showStatusHint(const QString &text) const;
		void clearStatusHint() const;
		QString currentModeStatusHint() const;   // richer, one-line gesture/modifier reference for the status bar, shown when hovering the shape body
		QString handleRoleTooltip(HandleRole role, int slot) const;   // shown natively by Qt when hovering that specific handle, and pushed to the status bar too
		static QString handleModeLabel(HandleMode mode);
		static bool isResizeCornerSlot(int slot);   // true for the 4 corner slots (of 8) in QetGraphicsHandlerUtility::pointsForRect's ordering
		void rebuildHandles();       // (re)creates handler items -- only when the *set* of handles changes
		void repositionHandles();    // moves existing handler items -- safe to call every frame of a live drag
		void insertPoint();
		void removePoint();
		void convertToPathExplicitly();   // context-menu action; see promoteRectangleOrEllipseToPolygon()
		void mirror(bool horizontal);   // context-menu action: flips scaleFactorX (horizontal) or scaleFactorY (vertical) around the current pivot
		void setNodeKind(int nodeIndex, NodeKind kind);   // context-menu action on a Path node

		void handlerMousePressEvent(int handlerIndex);
		void handlerMouseMoveEvent(int handlerIndex, QGraphicsSceneMouseEvent *event);
		void handlerMouseReleaseEvent(int handlerIndex);

			// One dispatch function per handle role -- called from
			// handlerMouseMoveEvent() with the mouse position already
			// mapped to local coordinates and any grid snap applied.
			// dragResize() covers both the plain and Ctrl/Shift/Alt-modified
			// interpretations of the whole Size handle set.
		void dragResize      (int index, const QPointF &localPos, Qt::KeyboardModifiers mods);
		void dragRotateHandle(int cornerIndex, const QPointF &scenePos, Qt::KeyboardModifiers mods);
		void dragSkewHandle  (int edgeIndex,   const QPointF &scenePos, Qt::KeyboardModifiers mods);
		void dragPivotHandle (const QPointF &localPos);
		void dragArcEndpoint (int which,       const QPointF &localPos, Qt::KeyboardModifiers mods);
		void dragCornerRadius(int which,       const QPointF &localPos);
		void dragPathAnchor  (int which,       const QPointF &localPos, Qt::KeyboardModifiers mods);
		void dragPathControlHandle(bool isOutHandle, int nodeIndex, const QPointF &localPos, Qt::KeyboardModifiers mods);
		void mirrorOppositeHandle(PathNode &node, bool justChangedIsOut);   // shared by dragPathControlHandle() and dragCurveSegment()
		void dragCurveSegment(int segmentIndex, qreal t, const QPointF &localPos);   // Inkscape-style "grab the curve itself"

		void promoteRectangleOrEllipseToPolygon(int detachedResizeIndex, const QPointF &newLocalPos);
		std::pair<int, qreal> nearestPathSegment(const QPointF &localPos) const;   // {segmentIndex, t}, -1 if fewer than 2 nodes
		void insertPathPoint(int segmentIndex, qreal t);   // De Casteljau split -- see .cpp for why
		void removePathPoint(int nodeIndex);
		QDomElement snapshotXml() const;   // helper for PromoteShapeCommand: toXml() into a throwaway document

		QRectF localRect() const;
		QPainterPath outline() const;   // raw, unstroked path for the current type; shared by shape() and paint()
		QVector<QPointF> currentHandlePositions() const;   // in m_handleRoles/m_handleSlot order, local coordinates
		QPointF handlePositionFor(HandleRole role, int slot) const;
		QPointF rotateHandleReference(int slot) const;   // the point a Rotate handle tracks, before rotation is applied
		QPointF scaleOnlyOffset(const QPointF &localPoint) const;        // (localPoint - pivot), scaled, in the pre-shear frame
		QPointF scaleAndShearOffset(const QPointF &localPoint) const;    // same, with current shear also applied -- the pre-rotation frame
		static QColor colorForHandleRole(HandleRole role);
		static QRectF lockAspectRatio(const QRectF &oldRect, QRectF newRect, int resizeIndex, bool mirrored);
		static QPointF cornerPoint(const QRectF &rect, int cornerIndex);   // 0=NW,1=NE,2=SE,3=SW
		static QPointF edgeMidpoint(const QRectF &rect, int edgeIndex);    // 0=N, 1=E, 2=S, 3=W

			///ATTRIBUTES
	private:
		ShapeType	 m_shapeType;
		QPen		 m_pen;
		QBrush		 m_brush;
		QPointF		 m_P1,
				 m_P2,
				 m_old_P1,
				 m_old_P2,
				 m_context_menu_pos;
		QPolygonF	 m_polygon, m_old_polygon;
		bool		 m_hovered;
		int		 m_vector_index;
		bool		 m_closed = false,
				 m_modifie_radius_equaly = false;
		QVector<QetGraphicsHandlerItem *> m_handler_vector;
		QAction		 *m_insert_point,
				 *m_remove_point;
		qreal		 m_xRadius = 0,
				 m_yRadius = 0,
				 m_old_xRadius,
				 m_old_yRadius;

		ShapeTransform   m_transform;
		ShapeTransform   m_old_transform;
		HandleMode       m_handleMode = HandleMode::Size;
		QVector<HandleRole> m_handleRoles;   // parallel to m_handler_vector, one role per handle
		QVector<int>        m_handleSlot;    // parallel to m_handler_vector, meaning depends on role (see HandleRole)
		QPointF          m_old_pos;

		qreal            m_startAngle = 0;
		qreal            m_endAngle = 360;
		ArcClosure       m_arcClosure = NoClosure;
		bool             m_pivotIsCustom = false;   // false: pivot auto-follows the bounding-rect center on every geometry edit
		bool             m_deferHandleReposition = false;   // true while setPivot() is applying its two related updates together

		QVector<PathNode> m_nodes;
		QVector<PathNode> m_old_nodes;              // saved at handle press, for undo
		int              m_curveDragSegment = -1;   // >=0 while dragging the curve itself (not a handle) between two nodes
		qreal            m_curveDragT = 0;          // parameter along that segment where the drag started
		QPointF          m_curveDragOriginalP1, m_curveDragOriginalP2;   // absolute control points at drag start, fixed for the whole drag
		QPointF          m_curveDragPressPos;        // local position at press, to tell a plain click from a real drag
		bool             m_curveDragEngaged = false; // false until the drag actually exceeds a small threshold
};
#endif // QETSHAPEITEM_H
