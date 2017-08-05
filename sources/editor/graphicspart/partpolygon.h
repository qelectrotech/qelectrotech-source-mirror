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
#ifndef PART_POLYGON_H
#define PART_POLYGON_H

#include <QPolygonF>
#include "customelementgraphicpart.h"

class QPropertyUndoCommand;
class QetGraphicsHandlerItem;

/**
 * @brief The PartPolygon class
 * This class represents a polygon primitive which may be used to compose the
 * drawing of an electrical element within the element editor.
 */
class PartPolygon : public CustomElementGraphicPart
{
		Q_OBJECT

		Q_PROPERTY(bool closed READ isClosed WRITE setClosed)
		Q_PROPERTY(QPolygonF polygon READ polygon WRITE setPolygon)

		// constructors, destructor
	public:
		PartPolygon(QETElementEditor *editor, QGraphicsItem *parent = nullptr);
		~PartPolygon() override;
	
	private:
		PartPolygon(const PartPolygon &);

	signals:
		void closedChange();
		void polygonChanged();

		// methods
	public:
		enum { Type = UserType + 1105 };
			/**
			* Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a PartPolygon.
			* @return the QGraphicsItem type
			*/
		int type() const override { return Type; }
		void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override;

		QString name() const override { return(QObject::tr("polygone", "element part name")); }
		QString xmlName() const override { return(QString("polygon")); }
		void fromXml(const QDomElement &) override;
		const QDomElement toXml(QDomDocument &) const override;

		QPainterPath shape () const override;
		QPainterPath shadowShape() const override;
		QRectF boundingRect() const override;
		bool isUseless() const override;
		QRectF sceneGeometricRect() const override;

		void startUserTransformation(const QRectF &) override;
		void handleUserTransformation(const QRectF &, const QRectF &) override;
		QET::ScalingMethod preferredScalingMethod() const override;

		QPolygonF polygon () const;
		void setPolygon   (const QPolygonF &polygon);

		void addPoint        (const QPointF &point);
		void setLastPoint    (const QPointF &point);
		void removeLastPoint ();	

		bool isClosed  () const {return m_closed;}
		void setClosed (bool close);

	protected:
		QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
		bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;
	
	private:
		void adjusteHandlerPos();
		void handlerMousePressEvent   (QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event);
		void handlerMouseMoveEvent    (QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event);
		void handlerMouseReleaseEvent (QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event);
		void sceneSelectionChanged ();
		
		void addHandler();
		void removeHandler();
		
		
		bool m_closed;
		QList<QPointF> saved_points_;
		QPolygonF m_polygon;
		QPropertyUndoCommand *m_undo_command;
		int m_vector_index = -1;
		QVector<QetGraphicsHandlerItem *> m_handler_vector;
};
#endif
