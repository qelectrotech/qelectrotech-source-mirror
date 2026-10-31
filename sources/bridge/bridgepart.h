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
#ifndef BRIDGEPART_H
#define BRIDGEPART_H

#include "bridge.h"

#include "../qetgraphicsitem/qetgraphicsitem.h"

#include <QPointer>

/**
	@brief The line a bridge is drawn as on one folio.

	Like CablePart: everything is a view of the Bridge, the geometry is
	kept in the bridge itself, so the line and the bridge can never
	disagree. The line is visible as any other line of the drawing, in
	the colour of its bridge -- which is black unless the user chose
	another one.

	The shape is the user's own: a bridge is drawn where he wants it and
	keeps the form he gave it. Nothing here rearranges a line by itself.
*/
class BridgePart : public QetGraphicsItem
{
		Q_OBJECT

	public:
		explicit BridgePart(const BridgePartData &data, QGraphicsItem *parent = nullptr);
		~BridgePart() override;

		enum {Type = UserType + 1013};
		int type() const override {return Type;}

		void setBridge(Bridge *bridge);
		Bridge *bridge() const;
		QUuid partUuid() const;
			/**
				While a terminal hovers over this bridge and the bridge
				cannot take one more, the line is drawn in the colour of
				a problem: the hand has to see what stops the drop, not
				only find a message about it afterwards.
			*/
		void setRefused(bool refused);
		bool isRefused() const;
			/**
				Every bridge line drawn anywhere in the application: what
				is happening under the hand paints them without hunting
				through the folios for them.
			*/
		static QList<BridgePart *> allParts();

		void setLine(const QPointF &p1, const QPointF &p2);
			/**
				A line moved about -- a paste puts it down elsewhere, a
				duplicate stamps it at an offset, the hand drags it --
				is a moved line: these two hide the base class, which
				refuses to move an item whose shape is the user's own.
				The moving itself is folded into the ends by itemChange().
			*/
		void setPos(const QPointF &p);
		void setPos(qreal x, qreal y);
		QLineF line() const;
		QPointF firstPoint() const {return m_p1;}
		QPointF secondPoint() const {return m_p2;}

		QRectF boundingRect() const override;
		QPainterPath shape() const override;
		void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
		QString name() const override;

		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
			///Opens the dialog editing the properties of the whole bridge
		void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
		void editProperty() override;

	private:
		QColor drawColor() const;
		qreal lineWidth() const;

	private:
		QPointer<Bridge> m_bridge;
		QUuid m_part_uuid;
		QPointF m_p1;
		QPointF m_p2;
			///The whole line is being dragged to a new place
		bool m_moving_line = false;
		QPointF m_move_anchor;
			///1 or 2 while one of the ends is dragged around, 0 otherwise
		int m_stretch_end = 0;
			///Where the line stood when the drag began, to put it back
			///when what it would take cannot be built
		QLineF m_stretched_from;
			///Drawn in the colour of a problem while a terminal hovers
			///over a bridge which cannot take it
		bool m_refused = false;
};

#endif // BRIDGEPART_H
