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
#ifndef PART_ARC_H
#define PART_ARC_H

#include "abstractpartellipse.h"

class QPropertyUndoCommand;
class QetGraphicsHandlerItem;

/**
 * @brief The PartArc class
 * This class represents an elliptical arc primitive which may be used to
 * compose the drawing of an electrical element within the element editor.
 */
class PartArc : public AbstractPartEllipse
{
		Q_OBJECT

	public:
		PartArc(QETElementEditor *editor, QGraphicsItem *parent = nullptr);
		virtual ~PartArc();
	
	private:
		PartArc(const PartArc &);
		// methods
	public:
		enum { Type = UserType + 1101 };
			/**
			 * Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a PartArc.
			 * @return the QGraphicsItem type
			 */
		virtual int type() const { return Type; }
		virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = nullptr);

			//Name and XML
		virtual QString name()    const { return(QObject::tr("arc", "element part name")); }
		virtual QString xmlName() const { return(QString("arc")); }
		virtual const QDomElement toXml   (QDomDocument &) const;
		virtual void              fromXml (const QDomElement &);

		virtual QPainterPath shape() const;
		virtual QPainterPath shadowShape() const;
		virtual void setRect(const QRectF &rect) {AbstractPartEllipse::setRect(rect); adjusteHandlerPos();}
		virtual void setStartAngle(const int &start_angle) {AbstractPartEllipse::setStartAngle(start_angle); adjusteHandlerPos();}
		virtual void setSpanAngle(const int &span_angle) {AbstractPartEllipse::setSpanAngle(span_angle); adjusteHandlerPos();}

	protected:
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
		virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
		virtual bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);

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
		QPropertyUndoCommand *m_undo_command = nullptr;
		QPropertyUndoCommand *m_undo_command2 = nullptr;
		int m_resize_mode = 1,
			m_vector_index = -1;
		QPointF m_span_point;
		QVector<QetGraphicsHandlerItem *> m_handler_vector;
};
#endif
