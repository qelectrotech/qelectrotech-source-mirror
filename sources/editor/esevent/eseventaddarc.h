/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef ESEVENTADDARC_H
#define ESEVENTADDARC_H

#include "eseventinterface.h"

class ElementScene;
class PartArc;
class QGraphicsSceneMouseEvent;

/**
 * @brief The ESEventAddArc class
 * This ESEvent manage creation of arc in an ElementScene
 */
class ESEventAddArc : public ESEventInterface
{
	public:
		ESEventAddArc(ElementScene *scene);
		virtual ~ESEventAddArc();

		virtual bool mousePressEvent   (QGraphicsSceneMouseEvent *event);
		virtual bool mouseMoveEvent    (QGraphicsSceneMouseEvent *event);
		virtual bool mouseReleaseEvent (QGraphicsSceneMouseEvent *event);
		virtual bool keyPressEvent     (QKeyEvent *event);

	private:
		void updateArc ();

	private:
		PartArc *m_arc;
		QPointF  m_origin, m_mouse_pos;
		bool     m_inverted;
};

#endif // ESEVENTADDARC_H
