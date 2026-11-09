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
#ifndef ESEVENTSETORIGIN_H
#define ESEVENTSETORIGIN_H

#include "eseventinterface.h"

#include <QPointF>

class ElementScene;
class QGraphicsSceneMouseEvent;

/**
	@brief The ESEventSetOrigin class
	Draw first, then say where the symbol is inserted: a click on a
	terminal, or on a grid point (a corner of the drawing), makes it the
	origin by moving every part, as one undo step. The origin is the point
	that lands on the folio grid when the symbol is placed.
*/
class ESEventSetOrigin : public ESEventInterface
{
	public:
		ESEventSetOrigin(ElementScene *scene);

		bool mouseMoveEvent    (QGraphicsSceneMouseEvent *event) override;
		bool mouseReleaseEvent (QGraphicsSceneMouseEvent *event) override;

			/// The point a click at @p scene_pos makes the origin: the
			/// connection point of a terminal within reach, else the
			/// nearest grid point
		static QPointF target(ElementScene *scene, const QPointF &scene_pos);
			/// Move every part of @p scene so @p point becomes the origin,
			/// as one undo step; false when it already is
		static bool moveOriginTo(ElementScene *scene, const QPointF &point);
};

#endif // ESEVENTSETORIGIN_H
