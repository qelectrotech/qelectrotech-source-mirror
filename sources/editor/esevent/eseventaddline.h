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
#ifndef ESEVENTADDLINE_H
#define ESEVENTADDLINE_H

#include "eseventinterface.h"

class ElementScene;
class PartLine;
class QGraphicsSceneMouseEvent;

/**
 * @brief The ESEventAddLine class
 * This ESEvent manage creation of line in a ElementScene
 */
class ESEventAddLine : public ESEventInterface
{
	public:
		ESEventAddLine(ElementScene *scene);
		virtual ~ESEventAddLine();

		virtual bool mousePressEvent   (QGraphicsSceneMouseEvent *event);
		virtual bool mouseMoveEvent    (QGraphicsSceneMouseEvent *event);
		virtual bool mouseReleaseEvent (QGraphicsSceneMouseEvent *event);

	private:
		PartLine *m_line;
};

#endif // ESEVENTADDLINE_H
