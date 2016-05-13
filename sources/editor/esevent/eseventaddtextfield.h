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
#ifndef ESEVENTADDTEXTFIELD_H
#define ESEVENTADDTEXTFIELD_H

#include "eseventinterface.h"

class ElementScene;
class PartTextField;
class QGraphicsSceneMouseEvent;

/**
 * @brief The ESEventAddTextField class
 * This ESEvent manage creation of text field in an ElementScene
 */
class ESEventAddTextField : public ESEventInterface
{
	public:
		ESEventAddTextField(ElementScene *scene);
		virtual ~ESEventAddTextField();

		virtual bool mouseMoveEvent    (QGraphicsSceneMouseEvent *event);
		virtual bool mouseReleaseEvent (QGraphicsSceneMouseEvent *event);

	private:
		PartTextField *m_text;
};

#endif // ESEVENTADDTEXTFIELD_H
