/*
	Copyright 2006-2017 The QElectroTech Team
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

class QetGraphicsHandlerItem;

/**
 * This class represents a rectangle primitive which may be used to compose the
 * drawing of an electrical element within the element editor.
 * All coordinates is in item coordinate, except pos()
*/
class PartRectangle :  public CustomElementGraphicPart
{
		Q_OBJECT

		Q_PROPERTY(QRectF rect READ rect WRITE setRect)
		Q_PROPERTY(qreal xRadius READ XRadius WRITE setXRadius NOTIFY XRadiusChanged)
		Q_PROPERTY(qreal yRadius READ YRadius WRITE setYRadius NOTIFY YRadiusChanged)

		// constructors, destructor
	public:
		PartRectangle(QETElementEditor *, QGraphicsItem *parent = nullptr);
		~PartRectangle() override;
	
	private:
		PartRectangle(const PartRectangle &);

	signals:
		void rectChanged();
		void XRadiusChanged();
		void YRadiusChanged();
	
		// methods
	public:
		enum { Type = UserType + 1109 };
			/**
			 * Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a PartRectangle.
			 * @return the QGraphicsItem type
			 */
		int     type  () const override { return Type; }
		void    paint (QPainter *, const QStyleOptionGraphicsItem *, QWidget * = nullptr) override;
		QString name  () const override { return(QObject::tr("rectangle", "element part name")); }

		QString           xmlName () const override { return(QString("rect")); }
		const QDomElement toXml   (QDomDocument &) const override;
		void              fromXml (const QDomElement &) override;

		QRectF rect() const;
		void   setRect(const QRectF &rect);
		qreal XRadius() const {return m_xRadius;}
		void setXRadius(qreal X);
		qreal YRadius() const {return m_yRadius;}
		void setYRadius(qreal Y);

		QRectF  sceneGeometricRect() const override;
		virtual QPointF sceneTopLeft() const;

		QPainterPath shape () const override;
		QPainterPath shadowShape() const override;
		QRectF boundingRect() const override;
		bool   isUseless() const override;

		void startUserTransformation(const QRectF &) override;
		void handleUserTransformation(const QRectF &, const QRectF &) override;

	protected:
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
		QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
		bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

	private:
		void switchResizeMode();
		void adjusteHandlerPos();
		void handlerMousePressEvent   (QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event);
		void handlerMouseMoveEvent    (QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event);
		void handlerMouseReleaseEvent (QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event);
		void sceneSelectionChanged ();
		
		void addHandler();
		void removeHandler();
	
	private:
		QRectF m_rect,
			   m_old_rect;
		QList<QPointF> saved_points_;
		int m_resize_mode = 1,
			m_vector_index = -1;
		QVector<QetGraphicsHandlerItem *> m_handler_vector;
		qreal m_xRadius = 0,
			  m_yRadius = 0,
			  m_old_xRadius,
			  m_old_yRadius;
		bool m_modifie_radius_equaly = false;
};
#endif
