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
#ifndef ELEMENT_MEASURE_ITEM_H
#define ELEMENT_MEASURE_ITEM_H

#include <QGraphicsItem>
#include <QPointF>
#include <QColor>

/**
	@brief The ElementMeasureItem class
	A visual CAD measurement overlay for the Element Editor.
	Displays distance in millimeters, dx, dy, and angular dimensions between two points.
	Does not inherit from CustomElementPart, ensuring it is never saved to .elmt XML.
*/
class ElementMeasureItem : public QGraphicsItem
{
	public:
		enum { Type = UserType + 1025 };

		ElementMeasureItem(const QPointF &p1, const QPointF &p2, QGraphicsItem *parent = nullptr);
		~ElementMeasureItem() override = default;

		int type() const override { return Type; }

		void setP1(const QPointF &p1);
		void setP2(const QPointF &p2);
		QPointF p1() const { return m_p1; }
		QPointF p2() const { return m_p2; }

		qreal lengthMM() const;
		qreal deltaXMM() const;
		qreal deltaYMM() const;
		qreal angleDeg() const;

		void setFinished(bool finished);
		bool isFinished() const { return m_finished; }

		QRectF boundingRect() const override;
		void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

	private:
		QPointF m_p1;
		QPointF m_p2;
		bool m_finished = false;
};

#endif // ELEMENT_MEASURE_ITEM_H
