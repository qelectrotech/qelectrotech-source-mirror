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
#ifndef PART_POLYGON_H
#define PART_POLYGON_H
#include <QtGui>
#include "customelementgraphicpart.h"
/**
	Cette classe represente un polygone pouvant etre utilise pour composer le
	dessin d'un element dans l'editeur d'element.
*/
class PartPolygon : public QGraphicsPolygonItem, public CustomElementGraphicPart {
	// constructeurs, destructeur
	public:
	PartPolygon(QETElementEditor *, QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartPolygon();
	
	private:
	PartPolygon(const PartPolygon &);
	
	// attributs
	private:
	bool closed;
	
	// methodes
	public:
	enum { Type = UserType + 1105 };
	/**
		permet de caster un QGraphicsItem en PartPolygon avec qgraphicsitem_cast
		@return le type de QGraphicsItem
	*/
	virtual int type() const { return Type; }
	virtual QString name() const { return(QObject::tr("polygone", "element part name")); }
	virtual QString xmlName() const { return(QString("polygon")); }
	void fromXml(const QDomElement &);
	const QDomElement toXml(QDomDocument &) const;
	virtual QRectF boundingRect() const;
	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
	void setClosed(bool c);
	bool isClosed() const;
	void setProperty(const QString &, const QVariant &);
	virtual QVariant property(const QString &);
	virtual bool isUseless() const;
	
	protected:
	QVariant itemChange(GraphicsItemChange, const QVariant &);
};

/**
	Specifie si le polygone doit etre ferme
	@param c true pour un polygone ferme, false sinon
*/
inline void PartPolygon::setClosed(bool c) {
	closed = c;
}

/**
	Indique si le polygone est ferme
	@return true si le polygone est ferme, false sinon
*/
inline bool PartPolygon::isClosed() const {
	return(closed);
}

#endif
