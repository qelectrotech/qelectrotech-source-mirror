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
#ifndef DIAGRAMEVENTADDPATH_H
#define DIAGRAMEVENTADDPATH_H

#include "../qetgraphicsitem/qetshapeitem.h"
#include "diagrameventinterface.h"

class QGraphicsLineItem;

/**
	@brief The DiagramEventAddPath class
	Pen tool: interactively draw a new Path (Bezier) shape, following the
	same vocabulary every vector editor's pen tool uses:
	  - click places a Corner node;
	  - press-drag-release places a Smooth node, with the drag defining a
	    pair of mirrored handles (same convention as
	    QetShapeItem::dragPathControlHandle's "Smooth" mirroring);
	  - clicking back on the first node closes the path;
	  - double-click, Enter, or Escape (once 2+ nodes exist) finishes it
	    open;
	  - right-click steps back one node;
	  - right-click or Escape with nothing placed yet cancels the tool.
	While running, m_nodes always carries one extra "preview" node at the
	end, tracking the mouse so a live rubber-band segment is always
	visible -- confirmedNodeCount() excludes it; every public gesture
	handler is responsible for stripping it before treating the list as
	"the path so far" (see finishPath(), which does this once for every
	finishing gesture).
*/
class DiagramEventAddPath : public DiagramEventInterface
{
		Q_OBJECT

	public:
		DiagramEventAddPath(Diagram *diagram);
		~DiagramEventAddPath() override;

		void mousePressEvent       (QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent        (QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent     (QGraphicsSceneMouseEvent *event) override;
		void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event) override;
		void keyPressEvent         (QKeyEvent *event) override;
		void init() override;

	private:
		void updateHelpCross (const QPointF &p);
		void showHint () const;
		void finishPath (bool closed);
		void cancelPath ();
		bool nearFirstNode (const QPointF &scenePos) const;
		int  confirmedNodeCount () const;   // m_nodes always carries one trailing "preview" node while running; this excludes it
		QPointF snapped (const QPointF &scenePos, Qt::KeyboardModifiers mods) const;

		QetShapeItem                    *m_shape_item;
		QVector<QetShapeItem::PathNode>  m_nodes;
		int                               m_dragging_node = -1;
		QGraphicsLineItem                *m_help_horiz, *m_help_verti;

		// Scene units within which a click on an existing path is
		// treated as "on the first node" and closes the shape, rather
		// than adding yet another node right next to it.
		static constexpr qreal CLOSE_THRESHOLD = 12.0;
};

#endif // DIAGRAMEVENTADDPATH_H
