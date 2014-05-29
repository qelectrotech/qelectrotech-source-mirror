/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef PART_RECTANGLE_H
#define PART_RECTANGLE_H
#include <QtGui>
#include "customelementgraphicpart.h"
/**
	This class represents a rectangle primitive which may be used to compose the
	drawing of an electrical element within the element editor.
*/
class PartRectangle :  public CustomElementGraphicPart, public QGraphicsRectItem {
	Q_OBJECT
	// constructors, destructor
	public:
	PartRectangle(QETElementEditor *, QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartRectangle();
	
	private:
	PartRectangle(const PartRectangle &);
	
	// methods
	public:
	enum { Type = UserType + 1109 };
	/**
		Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a
		PartRectangle.
		@return the QGraphicsItem type
	*/
	virtual int type() const { return Type; }
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0);
	virtual QString name() const { return(QObject::tr("rectangle", "element part name")); }
	virtual QString xmlName() const { return(QString("rect")); }
	virtual const QDomElement toXml(QDomDocument &) const;
	virtual void fromXml(const QDomElement &);
	virtual QPointF sceneTopLeft() const;
	virtual QRectF boundingRect() const;
	virtual bool isUseless() const;
	virtual QRectF sceneGeometricRect() const;
	virtual void startUserTransformation(const QRectF &);
	virtual void handleUserTransformation(const QRectF &, const QRectF &);

	///PROPERTY
	// X value
	Q_PROPERTY(qreal x READ x WRITE setX)
		qreal x() const {return mapToScene(rect().topLeft()).x();}
		void setX(qreal x);
	// Y value
	Q_PROPERTY(qreal y READ y WRITE setY)
		qreal y() const {return mapToScene(rect().topLeft()).y();}
		void setY(qreal y);
	// Width value
	Q_PROPERTY(qreal width READ width WRITE setWidth)
		qreal width() const {return rect().width();}
		void setWidth(qreal w);
	// Height value
	Q_PROPERTY(qreal height READ height WRITE setHeight)
		qreal height() const { return rect().height();}
		void setHeight(qreal h);
	
	protected:
	QVariant itemChange(GraphicsItemChange, const QVariant &);
	
	private:
	QList<QPointF> saved_points_;
};
#endif
