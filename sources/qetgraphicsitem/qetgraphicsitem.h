/*
	Copyright 2006-2025 The QElectroTech Team
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
#ifndef QETGRAPHICSITEM_H
#define QETGRAPHICSITEM_H

#include "../qet.h"

#include <QGraphicsObject>

class Diagram;

class QetGraphicsItem : public QGraphicsObject
{
	Q_OBJECT

	public:
		//constructor destructor
		QetGraphicsItem(QGraphicsItem *parent = nullptr);
		~QetGraphicsItem() override = 0;

		//public method
		Diagram *diagram () const;
		virtual void setPos (const QPointF &p);
		virtual void setPos (qreal x, qreal y);

		virtual bool isMovable () const
{return is_movable_;}
		virtual void setMovable (bool movable) { is_movable_ = movable;}

		bool isHovered() const;

		virtual void editProperty () {}
		virtual QString name ()const
{return QString("");}
		
		QET::GraphicsItemState state() const;

	//protected method
	protected:
		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
		void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

	protected:
		bool is_movable_;
		bool m_first_move;
		bool snap_to_grid_;
		QPointF m_mouse_to_origin_movement;
		QET::GraphicsItemState m_state = QET:: GIOK;

	private:
		bool m_hovered{false};

};

#endif // QETGRAPHICSITEM_H
