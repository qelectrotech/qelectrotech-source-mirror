/*
	Copyright 2006-2016 The QElectroTech Team
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

#include "diagrameventinterface.h"
#include "elementslocation.h"

class Element;

/**
 * @brief The DiagramEventAddElement class
 * This diagram event add a new element, for each left click button at the position of click.
 * Space key rotate current element by 90°, right click button finish this event.
 */
class DiagramEventAddElement : public DiagramEventInterface
{
		Q_OBJECT

	public:
		DiagramEventAddElement(ElementsLocation &location, Diagram *diagram, QPointF pos = QPointF(0,0));
		virtual ~DiagramEventAddElement();

		virtual bool mouseMoveEvent        (QGraphicsSceneMouseEvent *event);
		virtual bool mousePressEvent       (QGraphicsSceneMouseEvent *event);
		virtual bool mouseReleaseEvent     (QGraphicsSceneMouseEvent *event);
		virtual bool mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event);
		virtual bool keyPressEvent (QKeyEvent *event);
		virtual void init();

	private:
		bool buildElement();
		void addElement();

	private:
		ElementsLocation m_location;
		Element *m_element;
		QString m_integrate_path;
};

#endif // DIAGRAMEVENTADDELEMENT_H
