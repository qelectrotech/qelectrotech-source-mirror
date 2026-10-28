/*
	Copyright 2006-2026 The QElectroTech Team
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
#ifndef CONDUCTORDEFINITIONMARK_H
#define CONDUCTORDEFINITIONMARK_H

#include <QGraphicsObject>

class Conductor;

/**
	@brief The ConductorDefinitionMark class
	A short slanted stroke across a conductor, with the conductor's colour
	written at its upper end and its section at its lower end: the way a
	wire's colour and section are marked on a schematic.

	It sits on the conductor at a distance from its start kept in the
	conductor's properties, and is dragged along the conductor with the
	mouse; it never leaves it. A conductor shows one when its properties
	ask for it (ConductorProperties::m_definition_mark).
*/
class ConductorDefinitionMark : public QGraphicsObject
{
	Q_OBJECT

	public:
		explicit ConductorDefinitionMark(Conductor *conductor);

		enum { Type = UserType + 1013 };
		int type() const override { return Type; }

		QRectF boundingRect() const override;
		QPainterPath shape() const override;
		void paint(QPainter *painter,
			   const QStyleOptionGraphicsItem *option,
			   QWidget *widget) override;

		void refresh();
		qreal distance() const { return m_distance; }

		static qreal length(const QList<QPointF> &points);
		static QPointF pointAt(const QList<QPointF> &points,
				       qreal distance, bool *vertical = nullptr);
		static qreal distanceOf(const QList<QPointF> &points,
					const QPointF &point);
		static qreal defaultDistance(const QList<QPointF> &points);

	protected:
		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
		void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

	private:
		QList<QPointF> points() const;
		void place(qreal distance);
		QRectF colorRect() const;
		QRectF sectionRect() const;
		QRectF textRect(const QString &text, bool first_side) const;
		void drawText(QPainter *painter, const QRectF &rect,
			      const QString &text) const;

		Conductor *m_conductor;
		qreal m_distance = 0;
		qreal m_distance_before_drag = -1;
		bool m_vertical = false;
		bool m_hovered = false;
};

#endif // CONDUCTORDEFINITIONMARK_H
