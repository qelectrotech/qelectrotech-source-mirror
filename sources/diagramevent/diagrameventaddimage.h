/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DIAGRAMEVENTADDIMAGE_H
#define DIAGRAMEVENTADDIMAGE_H

#include "diagrameventinterface.h"

#include <QPointF>

class Diagram;
class DiagramImageItem;

/**
	@brief The DiagramEventAddImage class
	This diagram event, handle the adding of an image in a diagram.
*/
class DiagramEventAddImage : public DiagramEventInterface
{
	Q_OBJECT

	public:
		DiagramEventAddImage(Diagram *diagram);
		~DiagramEventAddImage() override;

		void mousePressEvent       (QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent        (QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent     (QGraphicsSceneMouseEvent *event) override;
		void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event) override;
		void wheelEvent            (QGraphicsSceneWheelEvent *event) override;

		 bool isNull () const;
	private:
		void openDialog();
		void showHint() const;

		DiagramImageItem *m_image;
		bool m_is_added;
		bool m_pressed = false;    // left button held: dragging out a size, not just positioning
		bool m_resize_engaged = false;   // latched once the drag threshold is crossed, matching the pen tool's own curve-drag threshold convention -- so dragging out and back near the start point doesn't "snap back" to original size before release
		QPointF m_press_pos;       // scene position of the left-button press, the resize anchor
};

#endif // DIAGRAMEVENTADDIMAGE_H
