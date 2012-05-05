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
#ifndef PART_TERMINAL_H
#define PART_TERMINAL_H
#include "customelementpart.h"
#include "qet.h"
#include <QtGui>
/**
	Cette classe represente une borne pouvant etre utilisee pour composer le
	dessin d'un element dans l'editeur d'element.
*/
class PartTerminal : public CustomElementPart, public QGraphicsItem {
	public:
	// constructeurs, destructeur
	PartTerminal(QETElementEditor *, QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartTerminal();
	private:
	PartTerminal(const PartTerminal &);
	
	// attributs
	private:
	QET::Orientation _orientation;
	QPointF second_point;
	
	// methodes
	public:
	enum { Type = UserType + 1106 };
	/**
		permet de caster un QGraphicsItem en PartTerminal avec qgraphicsitem_cast
		@return le type de QGraphicsItem
	*/
	virtual int type() const { return Type; }
	virtual QString name() const { return(QObject::tr("borne", "element part name")); }
	virtual QString xmlName() const { return(QString("terminal")); }
	virtual void fromXml(const QDomElement &);
	virtual const QDomElement toXml(QDomDocument &) const;
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
	virtual QRectF boundingRect() const;
	QET::Orientation orientation() const;
	void setOrientation(QET::Orientation);
	virtual void setProperty(const QString &, const QVariant &);
	virtual QVariant property(const QString &);
	virtual bool isUseless() const;
	
	protected:
	QVariant itemChange(GraphicsItemChange, const QVariant &);
	
	private:
	void updateSecondPoint();
};
#endif
