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
#ifndef PARTPLCTABLE_H
#define PARTPLCTABLE_H

#include "customelementgraphicpart.h"
#include "../../QetGraphicsItemModeler/qetgraphicshandleritem.h"

#include <QVector>

/**
	@brief The PartPlcTable class
	This class represents a PLC I/O table preview in the element editor.
	It shows the user where the PLC table will appear at runtime, so they
	can position other element parts (rectangles, terminals, etc.) around it.
	The actual PLC data is read from ElementScene::elementData().
*/
class PartPlcTable : public CustomElementGraphicPart
{
	Q_OBJECT

	Q_PROPERTY(QRectF rect READ rect WRITE setRect)

	public:
		PartPlcTable(QETElementEditor *editor, QGraphicsItem *parent = nullptr);
		~PartPlcTable() override;

		enum { Type = UserType + 1120 };
		int     type  () const override { return Type; }
		void    paint (QPainter *, const QStyleOptionGraphicsItem *, QWidget * = nullptr) override;
		QString name  () const override { return QObject::tr("table PLC", "element part name"); }

		QString           xmlName () const override { return QString("plc_table"); }
		const QDomElement toXml   (QDomDocument &) const override;
		void              fromXml (const QDomElement &) override;

		QRectF rect() const;
		void   setRect(const QRectF &rect);

		QRectF  sceneGeometricRect() const override;
		QPainterPath shape () const override;
		QPainterPath shadowShape() const override;
		QRectF boundingRect() const override;
		bool   isUseless() const override;

		void startUserTransformation(const QRectF &) override;
		void handleUserTransformation(const QRectF &, const QRectF &) override;

		void addHandler() override;
		void removeHandler() override;

	protected:
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
		QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
		bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

	private:
		void switchResizeMode();
		void adjustHandlerPos();
		void handlerMousePressEvent   (QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event);
		void handlerMouseMoveEvent    (QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event);
		void handlerMouseReleaseEvent (QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event);
		QSizeF calculateTableSize() const;

	private:
		QRectF m_rect,
			   m_old_rect;
		QList<QPointF> saved_points_;
		int m_resize_mode = 1,
			m_vector_index = -1;
		QVector<QetGraphicsHandlerItem *> m_handler_vector;
};

#endif // PARTPLCTABLE_H
