/*
		Copyright 2006-2020 QElectroTech Team
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

/**
 * @brief The QetGraphicsTableItem class
 * This item display a table destined to represent the content of a QAbstractItemModel
 * The table have a few parameters to edit her visual aspect.
 * Margins, to edit the margin between the cell and the text.
 * Text font.
 * Text alignment in the cell
 * These two last parameters are not settable directly with the table but trough the model to be displayed by the table.
 * The table search for font and alignment only in the index(0,0) for all the table.
 * By consequence, set data in other index than 0,0 is useless also the alignment and font can't be set individually for each cell.
 */
class QetGraphicsTableItem : public QetGraphicsItem
{
	Q_OBJECT

	Q_PROPERTY(QMargins margins READ margins WRITE setMargins)
	Q_PROPERTY(QSize size READ size WRITE setSize)
	Q_PROPERTY(int displayNRow READ displayNRow WRITE setDisplayNRow)

    public:
        QetGraphicsTableItem(QGraphicsItem *parent= nullptr);
		virtual ~QetGraphicsTableItem() override;

		enum { Type = UserType + 1300 };
		int type() const override { return Type; }

		void setModel(QAbstractItemModel *model = nullptr);
		QAbstractItemModel *model() const;

		virtual QRectF boundingRect() const override;
		virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
		QMargins margins() const {return m_margin;}
		void setMargins(const QMargins &margins);
		QetGraphicsHeaderItem *headerItem() const {return m_header_item;}
		void setSize(const QSize &size);
		QSize size() const;
		QSize minimumSize() const;
		void setDisplayNRow(const int &number);
		int displayNRow() const;
		void setPreviousTable(QetGraphicsTableItem *table = nullptr);
		void setNextTable(QetGraphicsTableItem *table = nullptr);
		void setTableName(const QString &name);
		QString tableName() const;
		int displayNRowOffset() const;
		QetGraphicsTableItem *previousTable() const;
		QetGraphicsTableItem *nextTable() const;
		void setToMinimumHeight();
		void initLink();
		QUuid uuid() const {return m_uuid;}

		QDomElement toXml(QDomDocument &dom_document) const;
		void fromXml(const QDomElement &dom_element);
		static QString xmlTagName() {return QString("graphics_table");}

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
		void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
		void headerSectionResized();
		void adjustSize();
		void previousTableDisplayRowChanged();

    private:
        QAbstractItemModel *m_model= nullptr;

		QMargins m_margin;
		QVector<int> m_minimum_column_width;
		int m_minimum_row_height;
		int m_number_of_displayed_row = 0;
		QSize m_current_size,
			  m_old_size,
			  m_pending_size;

		int m_br_margin= 10;
		QRectF m_bounding_rect;

		QetGraphicsHandlerItem m_handler_item;
		QetGraphicsHeaderItem *m_header_item = nullptr;

		QetGraphicsTableItem *m_previous_table = nullptr,
							 *m_next_table = nullptr;

		QString m_name;
		QUuid m_uuid = QUuid::createUuid(),
			  m_pending_previous_table_uuid;
};

#endif // QetGraphicsTableItem_H
