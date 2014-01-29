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
#ifndef PART_POLYGON_H
#define PART_POLYGON_H
#include <QtGui>
#include "customelementgraphicpart.h"
/**
	This class represents a polygon primitive which may be used to compose the
	drawing of an electrical element within the element editor.
*/
class PartPolygon : public QGraphicsPolygonItem, public CustomElementGraphicPart {
	// constructors, destructor
	public:
	PartPolygon(QETElementEditor *, QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartPolygon();
	
	private:
	PartPolygon(const PartPolygon &);
	
	// attributes
	private:
	bool closed;
	
	// methods
	public:
	enum { Type = UserType + 1105 };
	/**
		Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a
		PartPolygon.
		@return the QGraphicsItem type
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
	virtual QRectF sceneGeometricRect() const;
	virtual void startUserTransformation(const QRectF &);
	virtual void handleUserTransformation(const QRectF &, const QRectF &);
	virtual QET::ScalingMethod preferredScalingMethod() const;
	
	protected:
	QVariant itemChange(GraphicsItemChange, const QVariant &);
	
	private:
	QList<QPointF> saved_points_;
};

/**
	Whether the polygon should be closed.
	@param c true for the polygon to be closed, false otherwise
*/
inline void PartPolygon::setClosed(bool c) {
	closed = c;
}

/**
	Indicate whether the polygon is closed.
	@return true if the polygon is closed, false otherwise
*/
inline bool PartPolygon::isClosed() const {
	return(closed);
}

#endif
