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

#include <QPolygonF>
#include "customelementgraphicpart.h"

/**
 * @brief The PartPolygon class
 * This class represents a polygon primitive which may be used to compose the
 * drawing of an electrical element within the element editor.
 */
class PartPolygon : public CustomElementGraphicPart
{
		Q_OBJECT

		Q_PROPERTY(bool closed READ isClosed WRITE setClosed)

		// constructors, destructor
	public:
		PartPolygon(QETElementEditor *editor, QGraphicsItem *parent = 0);
		virtual ~PartPolygon();
	
	private:
		PartPolygon(const PartPolygon &);

		// methods
	public:
		enum { Type = UserType + 1105 };
			/**
			* Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a PartPolygon.
			* @return the QGraphicsItem type
			*/
		virtual int type() const { return Type; }
		void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

		virtual QString name() const { return(QObject::tr("polygone", "element part name")); }
		virtual QString xmlName() const { return(QString("polygon")); }
		void fromXml(const QDomElement &);
		const QDomElement toXml(QDomDocument &) const;

		virtual QPainterPath shape () const;
		virtual QRectF boundingRect() const;
		virtual bool isUseless() const;
		virtual QRectF sceneGeometricRect() const;

		virtual void startUserTransformation(const QRectF &);
		virtual void handleUserTransformation(const QRectF &, const QRectF &);
		virtual QET::ScalingMethod preferredScalingMethod() const;

		QPolygonF polygon () const;
		void setPolygon   (const QPolygonF &polygon);

		void addPoint        (const QPointF &point);
		void setLastPoint    (const QPointF &point);
		void removeLastPoint ();	

		bool isClosed  () const {return m_closed;}
		void setClosed (bool c) {m_closed = c;}
	
	private:
		bool m_closed;
		QList<QPointF> saved_points_;
		QPolygonF m_polygon;
};
#endif
