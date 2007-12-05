/*
	Copyright 2006-2007 Xavier Guerrin
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
#ifndef PART_ELLIPSE_H
#define PART_ELLIPSE_H
#include <QtGui>
#include "customelementgraphicpart.h"
class EllipseEditor;
/**
	Cette classe represente une ellipse pouvant etre utilisee pour composer le
	dessin d'un element dans l'editeur d'element.
*/
class PartEllipse : public QGraphicsEllipseItem, public CustomElementGraphicPart {
	// constructeurs, destructeur
	public:
	PartEllipse(QETElementEditor *, QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartEllipse();
	
	private:
	PartEllipse(const PartEllipse &);
	
	// attributs
	private:
	EllipseEditor *informations;
	
	// methodes
	public:
	enum { Type = UserType + 1103 };
	virtual int type() const { return Type; }
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0);
	virtual QString name() const { return(QObject::tr("ellipse")); }
	virtual const QDomElement toXml(QDomDocument &) const;
	virtual void fromXml(const QDomElement &);
	virtual QPointF sceneTopLeft() const;
	virtual void setProperty(const QString &, const QVariant &);
	virtual QVariant property(const QString &);
	
	protected:
	QVariant itemChange(GraphicsItemChange, const QVariant &);
};
#endif
