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
#ifndef INDEPENDENT_TEXT_ITEM_H
#define INDEPENDENT_TEXT_ITEM_H
#include <QtGui>
#include "diagramtextitem.h"
/**
	Cette classe represente un champ de texte editable independant sur le schema.
	Il peut etre pivote et deplace.
*/
class IndependentTextItem : public DiagramTextItem {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	IndependentTextItem(Diagram * = 0);
	IndependentTextItem(const QString &, Diagram* = 0);
	virtual ~IndependentTextItem();
	
	// attributs
	public:
	enum { Type = UserType + 1005 };
	
	// methodes
	public:
	/**
		Cette methode permet d'utiliser qgraphicsitem_cast sur cet objet
		@return le type de QGraphicsItem
	*/
	virtual int type() const { return Type; }
	virtual void fromXml(const QDomElement &);
	virtual QDomElement toXml(QDomDocument &) const;
	
	protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
	
	private:
	bool first_move_;
	QPointF mouse_to_origin_movement_;
};
#endif
