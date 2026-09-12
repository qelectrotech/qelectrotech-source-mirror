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
#ifndef DIAGRAMEVENTADDSHAPE_H
#define DIAGRAMEVENTADDSHAPE_H

#include "../qetgraphicsitem/qetshapeitem.h"
#include "diagrameventinterface.h"

class QGraphicsEllipseItem;

/**
	@brief The DiagramEventAddShape class
	This event manage the creation of a shape.
*/
class DiagramEventAddShape : public DiagramEventInterface
{
		Q_OBJECT

	public:
		DiagramEventAddShape(Diagram *diagram, QetShapeItem::ShapeType shape_type);

		~DiagramEventAddShape() override;
		void mousePressEvent       (QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent        (QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent     (QGraphicsSceneMouseEvent *event) override;
		void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event) override;
		void keyPressEvent         (QKeyEvent *event) override;
		void keyReleaseEvent       (QKeyEvent *event) override;
		void init() override;

	private:
		void updateHelpCross (const QPointF &p);
		void applyPosition (const QPointF &pos, Qt::KeyboardModifiers mods);
		void updateCreationHint () const;
		QString beforeClickHint () const;
		QString afterClickHint () const;
		void showCenterMarker (const QPointF &scenePos);
		void hideCenterMarker ();
		void reapplyLastPosition (QKeyEvent *event);

	protected:
		QetShapeItem::ShapeType  m_shape_type;
		QetShapeItem            *m_shape_item;
		QGraphicsLineItem       *m_help_horiz, *m_help_verti;
		QPointF                  m_anchor_point;   // the shape's first-click point -- meaningful once m_shape_item exists
		QGraphicsEllipseItem    *m_center_marker = nullptr;   // shown only while Ctrl-anchoring is actually in effect, so it doubles as confirmation that it is
		bool                     m_center_anchored = false;   // decided once, at the first click -- see applyPosition()'s doc comment for why
		QPointF                  m_last_mouse_scene_pos;      // raw, unsnapped -- lets a modifier-only change re-snap correctly when reapplied
};

#endif // DIAGRAMEVENTADDSHAPE_H
