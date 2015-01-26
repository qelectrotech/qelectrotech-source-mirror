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
#ifndef PART_ARC_H
#define PART_ARC_H
#include <QtGui>
#include "customelementgraphicpart.h"
/**
	This class represents an elliptical arc primitive which may be used to
	compose the drawing of an electrical element within the element editor.
*/
class PartArc : public CustomElementGraphicPart, public QGraphicsEllipseItem {
	Q_OBJECT
	// constructors, destructor
	public:
	PartArc(QETElementEditor *, QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartArc();
	
	private:
	PartArc(const PartArc &);
	
	// attributes
	private:
	int _angle;
	int start_angle;
	
	// methods
	public:
	enum { Type = UserType + 1101 };
	/**
		Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a
		PartArc.
		@return the QGraphicsItem type
	*/
	virtual int type() const { return Type; }
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0);
	virtual QString name() const { return(QObject::tr("arc", "element part name")); }
	virtual QString xmlName() const { return(QString("arc")); }
	virtual const QDomElement toXml(QDomDocument &) const;
	virtual void fromXml(const QDomElement &);
	virtual QPointF sceneTopLeft() const;
	virtual QRectF boundingRect() const;
	virtual bool isUseless() const;
	virtual QRectF sceneGeometricRect() const;
	virtual void startUserTransformation(const QRectF &);
	virtual void handleUserTransformation(const QRectF &, const QRectF &);

	///PROPERT
	// X value
	Q_PROPERTY(qreal x READ x WRITE setX)
		qreal x() const {return mapToScene(rect().center()).x() ;}
		void setX(const qreal x);
	//Y value
	Q_PROPERTY(qreal y READ y WRITE setY)
		qreal y() const {return mapToScene(rect().center()).y();}
		void setY(const qreal y);
	// horizontal diameter
	Q_PROPERTY(qreal diameter_h READ width WRITE setWidth)
		qreal width() const {return rect().width();}
		void setWidth(const qreal w);
	// vertical diameter
	Q_PROPERTY(qreal diameter_v READ height WRITE setHeight)
		qreal height() const {return rect().height();}
		void setHeight (const qreal h);
	// start angle
	Q_PROPERTY(int start_angle READ startAngle WRITE setStartAngle)
		int startAngle() const {return start_angle;}
		void setStartAngle(const int sa){start_angle = sa;}
	// angle value
	Q_PROPERTY(int angle READ angle WRITE setAngle)
		int angle() const {return _angle;}
		void setAngle(const int a) {_angle = a;}

	
	protected:
	QVariant itemChange(GraphicsItemChange, const QVariant &);
	
	private:
	QList<QPointF> saved_points_;
};
#endif
