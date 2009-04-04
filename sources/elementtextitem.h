/*
	Copyright 2006-2009 Xavier Guerrin
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
#ifndef ELEMENT_TEXT_ITEM_H
#define ELEMENT_TEXT_ITEM_H
#include "diagramtextitem.h"
#include <QtXml>
class Diagram;
/**
	Cette classe represente un element de texte editable.
	Il est possible pour ce champ de texte de rester dans le sens de la lecture
	malgre les rotations de son element parent.
*/
class ElementTextItem : public DiagramTextItem {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	ElementTextItem(QGraphicsItem * = 0, QGraphicsScene * = 0);
	ElementTextItem(const QString &, QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~ElementTextItem();
	
	// attributs
	public:
	enum { Type = UserType + 1003 };
	
	private:
	bool follow_parent_rotations;
	QPointF original_position;
	
	// methodes
	public:
	virtual int type() const { return Type; }
	/// @return le rectangle delimitant le champ de texte
	virtual QRectF boundingRect() const { return(QGraphicsTextItem::boundingRect().adjusted(0.0, -1.1, 0.0, 0.0)); }
	bool followParentRotations() const;
	void setFollowParentRotations(bool);
	void fromXml(const QDomElement &);
	QDomElement toXml(QDomDocument &) const;
	void setPos(const QPointF &);
	void setPos(qreal, qreal);
	QPointF pos() const;
	void setOriginalPos(const QPointF &);
	QPointF originalPos() const;
};

/**
	Permet de savoir si le champ de texte suit les rotations de son parent.
	@return true si le champ de texte suit les rotations de son parent, false
	sinon
*/
inline bool ElementTextItem::followParentRotations() const {
	return(follow_parent_rotations);
}

/**
	Permet de specifier si le champ de texte suit les rotations de son parent.
	@param frp true si le champ de texte doit suivre  les rotations de son
	parent, false pour qu'ils ne les suivent pas
*/
inline void ElementTextItem::setFollowParentRotations(bool frp) {
	follow_parent_rotations = frp;
}

#endif
