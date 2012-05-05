/*
	Copyright 2006-2012 Xavier Guerrin
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
	Lors de son edition dans l'editeur d'element, un element est decompose en
	parties graphiques. La classe ElementContent represente un ensemble de parties
	graphiques constituant tout ou partie d'un element.
	Note : pour le moment, ElementContent est un typedef pour QList\<QGraphicsItem *\>
	@see la documentation Qt de la classe QList
*/
typedef QList<QGraphicsItem *> ElementContent;
#endif
