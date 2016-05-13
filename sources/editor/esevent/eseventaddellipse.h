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
#ifndef ESEVENTADDELLIPSE_H
#define ESEVENTADDELLIPSE_H

#include "eseventinterface.h"
#include <QPointF>

class ElementScene;
class PartEllipse;
class QGraphicsSceneMouseEvent;

/**
 * @brief The ESEventAddEllipse class
 * This ESEvent manage creation of ellpise in an ElementScene
 */
class ESEventAddEllipse : public ESEventInterface
{
	public:
		ESEventAddEllipse(ElementScene *scene);
		~ESEventAddEllipse();

		virtual bool mousePressEvent   (QGraphicsSceneMouseEvent *event);
		virtual bool mouseMoveEvent    (QGraphicsSceneMouseEvent *event);
		virtual bool mouseReleaseEvent (QGraphicsSceneMouseEvent *event);

	private:
		PartEllipse *m_ellipse;
		QPointF      m_origin, m_new_pos;
};

#endif // ESEVENTADDELLIPSE_H
