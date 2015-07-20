/*
	Copyright 2006-2015 The QElectroTech Team
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

#include "customelementgraphicpart.h"
#include "QetGraphicsItemModeler/qetgraphicshandlerutility.h"

class ChangePartCommand;

/**
 * This class represents a rectangle primitive which may be used to compose the
 * drawing of an electrical element within the element editor.
 * All coordinates is in item coordinate, except pos()
*/
class PartRectangle :  public CustomElementGraphicPart
{
		Q_OBJECT

		Q_PROPERTY(QPointF rectTopLeft READ rectTopLeft WRITE setRectTopLeft)
		Q_PROPERTY(qreal width         READ width       WRITE setWidth)
		Q_PROPERTY(qreal height        READ height      WRITE setHeight)
		Q_PROPERTY(QRectF rect         READ rect        WRITE setRect)

		// constructors, destructor
	public:
		PartRectangle(QETElementEditor *, QGraphicsItem *parent = 0);
		virtual ~PartRectangle();
	
	private:
		PartRectangle(const PartRectangle &);
	
		// methods
	public:
		enum { Type = UserType + 1109 };
			/**
			 * Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a PartRectangle.
			 * @return the QGraphicsItem type
			 */
		virtual int     type  () const { return Type; }
		virtual void    paint (QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0);
		virtual QString name  () const { return(QObject::tr("rectangle", "element part name")); }

		virtual QString           xmlName () const { return(QString("rect")); }
		virtual const QDomElement toXml   (QDomDocument &) const;
		virtual void              fromXml (const QDomElement &);

		QRectF rect() const;
		void   setRect(const QRectF &rect);

		QPointF rectTopLeft    () const;
		void    setRectTopLeft (const QPointF &point);

		qreal width    () const {return rect().width();}
		void  setWidth (qreal w);

		qreal height    () const { return rect().height();}
		void  setHeight (qreal h);

		virtual QRectF  sceneGeometricRect() const;
		virtual QPointF sceneTopLeft() const;

		virtual QPainterPath shape () const;
		virtual QRectF boundingRect() const;
		virtual bool   isUseless() const;

		virtual void startUserTransformation(const QRectF &);
		virtual void handleUserTransformation(const QRectF &, const QRectF &);

	protected:
		virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	
	private:
		QRectF m_rect;
		QList<QPointF> saved_points_;
		QetGraphicsHandlerUtility m_handler;
		int m_handler_index;
		ChangePartCommand *m_undo_command;
};
#endif
