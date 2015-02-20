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
#ifndef ELEMENT_CONTENT_H
#define ELEMENT_CONTENT_H
#include <QList>
class QGraphicsItem;
/**
	When edited using the element editor, electrical elements are decomposed into
	visual primitives. The ElementContent class represents a set of visual
	primitives composing all or a part of an electrical element.
	
	Note: currently, ElementContent is a simple typedef for
	QList\<QGraphicsItem *\>
*/
typedef QList<QGraphicsItem *> ElementContent;
#endif
