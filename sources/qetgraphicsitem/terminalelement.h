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
#ifndef TERMINALELEMENT_H
#define TERMINALELEMENT_H

#include "customelement.h"

class TerminalElement : public CustomElement
{
		Q_OBJECT
	public:
		TerminalElement(const ElementsLocation &, QGraphicsItem * = 0, int * = 0);
		TerminalElement (ElementLocation &location, QGraphicsItem *parent = nullptr, int *state = nullptr);
		~TerminalElement();
};

#endif // TERMINALELEMENT_H
