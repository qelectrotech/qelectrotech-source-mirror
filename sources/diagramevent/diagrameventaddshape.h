/*
	Copyright 2006-2017 The QElectroTech Team
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

#include "diagrameventinterface.h"
#include "qetshapeitem.h"

/**
 * @brief The DiagramEventAddShape class
 * This event manage the creation of a shape.
 */
class DiagramEventAddShape : public DiagramEventInterface
{
		Q_OBJECT

	public:
		DiagramEventAddShape(Diagram *diagram, QetShapeItem::ShapeType shape_type);

		virtual ~DiagramEventAddShape();
		virtual bool mousePressEvent       (QGraphicsSceneMouseEvent *event);
		virtual bool mouseMoveEvent        (QGraphicsSceneMouseEvent *event);
		virtual bool mouseReleaseEvent     (QGraphicsSceneMouseEvent *event);
		virtual bool mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event);
		virtual void init();

	private:
		void updateHelpCross (const QPointF &p);

	protected:
		QetShapeItem::ShapeType  m_shape_type;
		QetShapeItem            *m_shape_item;
		QGraphicsLineItem       *m_help_horiz, *m_help_verti;
};

#endif // DIAGRAMEVENTADDSHAPE_H
