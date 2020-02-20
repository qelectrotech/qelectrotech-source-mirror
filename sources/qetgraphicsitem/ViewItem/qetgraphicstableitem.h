/*
        Copyright 2006-2019 QElectroTech Team
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
#ifndef QetGraphicsTableItem_H
#define QetGraphicsTableItem_H

#include <QFont>

#include "qetgraphicsitem.h"
#include "qetapp.h"
#include "QetGraphicsItemModeler/qetgraphicshandleritem.h"

class QAbstractItemModel;
class QetGraphicsHeaderItem;

class QetGraphicsTableItem : public QetGraphicsItem
{
    public:
        QetGraphicsTableItem(QGraphicsItem *parent= nullptr);
		virtual ~QetGraphicsTableItem() override;

        void setModel(QAbstractItemModel *model);
		QAbstractItemModel *model() const;
		void reset();

		virtual QRectF boundingRect() const override;
		virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

	protected:
		virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
		virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
		virtual bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

    private:
        void modelReseted();
		void setUpColumnAndRowMinimumSize();
		void setUpBoundingRect();
		void adjustHandlerPos();
		void setUpHandler();
		void handlerMousePressEvent   (QGraphicsSceneMouseEvent *event);
		void handlerMouseMoveEvent    (QGraphicsSceneMouseEvent *event);
		void handlerMouseReleaseEvent (QGraphicsSceneMouseEvent *event);
		void adjustColumnsWidth();

    private:
        QAbstractItemModel *m_model= nullptr;
		QFont m_font = QETApp::diagramTextsFont();

		QVector<int> m_minimum_column_width;

		int m_row_height;
		QMargins m_margin;
		QRect m_minimum_rect,
			  m_current_rect;

		QRectF m_bounding_rect;
		int m_br_margin= 10;

		QetGraphicsHandlerItem m_handler_item;
		QetGraphicsHeaderItem *m_header_item = nullptr;
};

#endif // QetGraphicsTableItem_H
