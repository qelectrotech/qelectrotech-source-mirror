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
#ifndef GHOST_ELEMENT_H
#define GHOST_ELEMENT_H
#include "customelement.h"
class Diagram;
class QGraphicsItem;
class ElementsLocation;
class Terminal;
/**
	La classe GhostElement herite de la classe CustomElement. Un GhostElement
	est destine a remplacer visuellement un CustomElement dont la definition
	n'a pu etre trouvee. Ainsi, au lieu de ne pas charger un element, et donc
	de perdre potentiellement :
	  * sa position, son orientation, ses textes,
	  * les conducteurs qui y sont lies,
	on peut lui substituer un GhostElement. Celui-ci extrapolera depuis le
	schema la position des bornes et des champs de texte.
	Visuellement, il sera represente par un rectangle.
*/
class GhostElement : public CustomElement {
	
	Q_OBJECT
	
	// constructeur, destructeur
	public:
	GhostElement(const ElementsLocation &, QGraphicsItem * = 0, Diagram * = 0);
	virtual ~GhostElement();
	
	// methodes
	public:
	bool fromXml(QDomElement &, QHash<int, Terminal *> &);
	
	protected:
	QRectF minimalBoundingRect() const;
	bool terminalsFromXml(QDomElement &, QHash<int, Terminal *> &);
	void generateDrawings();
	void generateDrawing(QPainter *);
};
#endif
