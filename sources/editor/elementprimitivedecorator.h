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
#ifndef ELEMENTPRIMITIVEDECORATOR_H
#define ELEMENTPRIMITIVEDECORATOR_H

#include <QGraphicsObject>
#include "qet.h"

class ElementEditionCommand;
class ElementScene;
class CustomElementPart;
class QetGraphicsHandlerItem;

/**
	This class represents a decorator rendered above selected items so users
	can manipulate (move, resize, ...) them.
	
	The implementation considers four kinds of bounding rects:
	  - the actual, effective bounding rect as returned by the boundingRect() method
	  - the original bounding rect, i.e. the rect containing all selected items at
	      the beginning of operations (or after a command object was generated)
	  - the new bounding rect, after the user moved or resized items
	  - the former bounding rect, due to implementation details
*/
class ElementPrimitiveDecorator : public QGraphicsObject
{
	Q_OBJECT
	
	public:
		ElementPrimitiveDecorator(QGraphicsItem * = nullptr);
		~ElementPrimitiveDecorator() override;
		
		enum { Type = UserType + 2200 };
		
			// methods
		QRectF internalBoundingRect() const;
		QRectF boundingRect () const override;
		void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = nullptr) override;
		int type() const override { return Type; }
		void setItems(const QList<QGraphicsItem *> &);
		void setItems(const QList<CustomElementPart *> &);
		QList<CustomElementPart *> items() const;
		QList<QGraphicsItem *> graphicsItems() const;
	
	public slots:
		void adjust();
	
	signals:
		void actionFinished(ElementEditionCommand *);
	
	protected:
		void mousePressEvent(QGraphicsSceneMouseEvent *) override;
		void mouseMoveEvent(QGraphicsSceneMouseEvent *) override;
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *) override;
		void keyPressEvent(QKeyEvent *) override;
		void keyReleaseEvent(QKeyEvent *) override;
		QPointF deltaForRoundScaling(const QRectF &, const QRectF &, qreal);
		QPointF snapConstPointToGrid(const QPointF &) const;
		void snapPointToGrid(QPointF &) const;
		bool mustSnapToGrid(QGraphicsSceneMouseEvent *);
		QET::ScalingMethod scalingMethod(QGraphicsSceneMouseEvent *);
		QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
		bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;
	
	private:
		void init();
		void saveOriginalBoundingRect();
		void adjustEffectiveBoundingRect();
		void startMovement();
		void applyMovementToRect(int, const QPointF &, QRectF &);
		CustomElementPart *singleItem() const;
		void translateItems(const QPointF &);
		void scaleItems(const QRectF &, const QRectF &);
		QRectF getSceneBoundingRect(QGraphicsItem *) const;
		QVector <QPointF> getResizingsPoints() const;
	
		
	private:
		void adjusteHandlerPos();
		void handlerMousePressEvent   (QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event);
		void handlerMouseMoveEvent    (QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event);
		void handlerMouseReleaseEvent (QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event);
		
		void addHandler();
		void removeHandler();
		
		
		
		
		QList<CustomElementPart *> decorated_items_;
		QRectF effective_bounding_rect_; ///< actual, effective bounding rect -- never shrinks
		QRectF original_bounding_rect_; ///< original bounding rect
		QRectF modified_bounding_rect_; ///< new bounding rect, after the user moved or resized items
	
			/**
				Index of the square leading the current operation (resizing, etc.) or -1 if no
				operation is occurring, -2 for a move operation.
			*/
		int current_operation_square_;
		int grid_step_x_;              ///< Grid horizontal step
		int grid_step_y_;              ///< Grid horizontal step
		QPointF first_pos_;            ///< First point involved within the current resizing operation
		QPointF latest_pos_;           ///< Latest point involved within the current resizing operation
		QPointF mouse_offset_;         ///< Offset between the mouse position and the point to be snapped to grid when moving selection
		bool moving_by_keys_;          ///< Whether we are currently moving our decorated items using the arrow keys
		QPointF keys_movement_;           ///< Movement applied to our decorated items using the arrow keys
		QVector<QetGraphicsHandlerItem *> m_handler_vector;
		int m_vector_index = -1;
};

#endif
