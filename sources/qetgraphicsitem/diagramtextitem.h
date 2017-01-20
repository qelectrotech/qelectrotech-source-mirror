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
#ifndef DIAGRAM_TEXT_ITEM_H
#define DIAGRAM_TEXT_ITEM_H

#include <QGraphicsTextItem>

class Diagram;
class QDomElement;
class QDomDocument;

/**
	This class represents a selectable, movable and editable text field on a
	diagram.
	@see QGraphicsItem::GraphicsItemFlags
*/
class DiagramTextItem : public QGraphicsTextItem
{
	Q_OBJECT

	public:
		DiagramTextItem(QGraphicsItem * = 0);
		DiagramTextItem(const QString &, QGraphicsItem * = 0);

	private:
		void build();
	
	public:
		enum { Type = UserType + 1004 };
		virtual int type() const { return Type; }

		Diagram *diagram() const;
		virtual void fromXml(const QDomElement &) = 0;
		virtual QDomElement toXml(QDomDocument &) const;
		qreal rotationAngle() const;
		void setRotationAngle(const qreal &);
		void rotateBy(const qreal &);
		void edit();

		QPointF mapMovementToScene    (const QPointF &) const;
		QPointF mapMovementFromScene  (const QPointF &) const;
		QPointF mapMovementToParent   (const QPointF &) const;
		QPointF mapMovementFromParent (const QPointF &) const;

		void setFontSize(int &s);
		void setNoEditable(bool e = true) {no_editable = e;}

	protected:
		virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
		virtual void focusInEvent(QFocusEvent *);
		virtual void focusOutEvent(QFocusEvent *);

		virtual void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event);
		virtual void mousePressEvent       (QGraphicsSceneMouseEvent *event);
		virtual void mouseMoveEvent        (QGraphicsSceneMouseEvent *event);
		virtual void mouseReleaseEvent     (QGraphicsSceneMouseEvent *event);

		virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *);
		virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *);
		virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *);

		virtual void applyRotation(const qreal &);

	signals:
			/// signal emitted after text was changed
		void diagramTextChanged(DiagramTextItem *, const QString &, const QString &);
	
	protected:
		bool m_mouse_hover;
		QString previous_text_;
		qreal rotation_angle_;
		bool no_editable;
		bool m_first_move;
		QPointF m_mouse_to_origin_movement;
};
#endif
