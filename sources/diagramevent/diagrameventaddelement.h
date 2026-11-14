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
#ifndef DIAGRAMEVENTADDELEMENT_H
#define DIAGRAMEVENTADDELEMENT_H

#include <ElementsCollection/elementslocation.h>
#include "diagrameventinterface.h"

#include <QPointF>

#include <functional>

class Element;
class QGraphicsLineItem;
class QGraphicsPathItem;
class QStatusBar;
class QUndoCommand;

/**
	@brief The DiagramEventAddElement class
	This diagram event add a new element, for each left click button at the position of click.
	Space key rotate current element by 90°, right click button finish this event.
	With the project's automatic connections on, or its "auto conductor"
	and AutoConnect::dragPlacesWithAutoConductor(),
	pressing and dragging places one copy on every line that crosses the
	drag (corners, T-junctions, folio reports); see
	AutoConnect::dragPlacements(). Without automatic connections the copies
	are joined once, with ordinary wires.
*/
class DiagramEventAddElement : public DiagramEventInterface
{
		Q_OBJECT

	public:
		DiagramEventAddElement(ElementsLocation &location, Diagram *diagram, QPointF pos = QPointF(0,0));
		~DiagramEventAddElement() override;

		void mouseMoveEvent        (QGraphicsSceneMouseEvent *event) override;
		void mousePressEvent       (QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent     (QGraphicsSceneMouseEvent *event) override;
		void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event) override;
		void keyPressEvent (QKeyEvent *event) override;
		void init() override;
			/// The symbol that follows the mouse, until it is dropped
		Element *pendingElement() const { return m_element; }
			/// Place one symbol only, without numbering it: @p placed is
			/// given it and the command placing it, then this event ends
		void placeOnce(std::function<void(Element *, QUndoCommand *)> placed)
		{ m_placed = std::move(placed); }

	private:
		bool buildElement();
		Element *createElement() const;
		void addElement();
		void updateDrag(const QPointF &to);
		void endDrag();
		void addElements(const QList<QPointF> &positions);

	private:
		ElementsLocation m_location;
		Element *m_element;
		QString m_integrate_path;
		QPointer<QStatusBar> m_status_bar;
		std::function<void(Element *, QUndoCommand *)> m_placed;

			/// Placing along a drag
		bool m_pressed = false,
			 m_dragging = false,
			 m_drag_cancelled = false;
		QPointF m_press_pos;
		QGraphicsLineItem *m_drag_line = nullptr;
		QGraphicsPathItem *m_preview_wires = nullptr;	///< grey, to each copy's partners
		QList<Element *> m_ghosts;		///< previews of the copies
		QList<QPointF> m_drag_positions;
};

#endif // DIAGRAMEVENTADDELEMENT_H
