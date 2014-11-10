/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef ESEVENTADDTERMINAL_H
#define ESEVENTADDTERMINAL_H

#include "eseventinterface.h"

class ElementScene;
class PartTerminal;
class QGraphicsSceneMouseEvent;

/**
 * @brief The ESEventAddTerminal class
 * This ESEvent manage creation of terminal in an ElementScene
 */
class ESEventAddTerminal : public ESEventInterface
{
	public:
		ESEventAddTerminal(ElementScene *scene);
		virtual ~ESEventAddTerminal();

		virtual bool mouseMoveEvent    (QGraphicsSceneMouseEvent *event);
		virtual bool mouseReleaseEvent (QGraphicsSceneMouseEvent *event);
		virtual bool keyPressEvent     (QKeyEvent *event);

	private:
		PartTerminal *m_terminal;
};

#endif // ESEVENTADDTERMINAL_H
