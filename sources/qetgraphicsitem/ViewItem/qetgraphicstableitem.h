/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef QetGraphicsTableItem_H
#define QetGraphicsTableItem_H

#include "../../QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "../../qetapp.h"
#include "../../qetgraphicsitem/qetgraphicsitem.h"

#include <QFont>

class QAbstractItemModel;
class QetGraphicsHeaderItem;

/**
	@brief The QetGraphicsTableItem class
	This item display a table destined to represent the content of a QAbstractItemModel
	The table have a few parameters to edit her visual aspect.
	Margins, to edit the margin between the cell and the text.
	Text font.
	Text alignment in the cell
	These three parameters are not settable directly with the table but through the model to be displayed by the table.
	The table search these parameters only in the index(0,0) for all the table.
	By consequence, set data in other index than 0,0 is useless also these parameter can't be set individually for each cell.
	The margins is stored in the model in index Qt::UserRole+1 and for value a QString. See QETUtils::marginsFromString and  QETUtils::marginsToString
*/
class QetGraphicsTableItem : public QetGraphicsItem
{
	Q_OBJECT

	Q_PROPERTY(QSize size READ size WRITE setSize)
	Q_PROPERTY(QVariantMap customFormat READ customFormat WRITE setCustomFormat)
	Q_PROPERTY(int displayNRow READ displayNRow WRITE setDisplayNRow)

	signals:
			/// Something the table saves changed (size, name, rows shown,
			/// model, column widths, links)
		void changed();

	public :
		static void adjustTableToFolio(
				QetGraphicsTableItem *table,
				QMargins margins = QMargins(20,20,20,0));
		static void checkInsufficientRowsCount(
				QetGraphicsTableItem *first_table);

		QetGraphicsTableItem(QGraphicsItem *parent= nullptr);
		virtual ~QetGraphicsTableItem() override;

		enum { Type = UserType + 1300 };
		int type() const override { return Type; }

		void editProperty() override;
		QVariantMap customFormat() const { return m_custom_format; }
        void setCustomFormat(const QVariantMap &format);
        bool customFormatting() const { return m_custom_format.value("enabled").toBool(); }
        int formattedRows() const;
        void refreshCustomGeometry();
        // Manual sizes are stored in customFormat; optional undo is one command.
        bool resizeColumn18(int column,int width,bool undo=true);
        bool resizeBoundary20(int column,int row,int delta,const QVariantMap &start,bool undo=true);
        QSet<QPair<int,int>> selectedCells20() const {return m_cells20;}
        void setSelectedCells20(const QSet<QPair<int,int>> &cells){m_cells20=cells;update();emit cellSelectionChanged20();}
        QRectF cellRect20(int column,int row) const;
        QPair<int,int> cellAt20(const QPointF &position) const;
        bool setCellText25(int column,int row,const QString &text,bool restore=false);
        void editCellText25(int column,int row,QWidget *parent=nullptr);
        void reviewCellText25(QWidget *parent=nullptr);
        bool resizeRow18(int absoluteModelRow,int height,bool undo=true);
        bool autoFitColumn18(int column,bool undo=true);
        bool autoFitRow18(int absoluteModelRow,bool undo=true);

        void setModel(QAbstractItemModel *model = nullptr);
		QAbstractItemModel *model() const;

		virtual QRectF boundingRect() const override;
		virtual void paint(
				QPainter *painter,
				const QStyleOptionGraphicsItem *option,
				QWidget *widget) override;
		QetGraphicsHeaderItem *headerItem() const
		{return m_header_item;}
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
		int displayedRowCount() const;
		QPointer<QetGraphicsTableItem> previousTable() const;
		QPointer<QetGraphicsTableItem> nextTable() const;
		void setToMinimumHeight();
		void initLink();
		bool hasPendingLink() const {return !m_pending_previous_table_uuid.isNull();}
		QUuid previousTableUuid() const;
		QUuid uuid() const {return m_uuid;}
		int minimumRowHeight() const;

		QDomElement toXml(QDomDocument &dom_document) const;
		void fromXml(const QDomElement &dom_element);
		static QString xmlTagName() {return QString("graphics_table");}
		virtual bool toDXF (const QString &filepath);

	signals:
        void customFormatChanged18(const QVariantMap &format);
        void cellSelectionChanged20();

	protected:

        void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
        void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;
        void focusOutEvent(QFocusEvent *event) override;
        bool sceneEvent(QEvent *event) override;
		virtual void hoverEnterEvent(
				QGraphicsSceneHoverEvent *event) override;
		virtual void hoverLeaveEvent(
				QGraphicsSceneHoverEvent *event) override;
		virtual bool sceneEventFilter(
				QGraphicsItem *watched,
				QEvent *event) override;
		virtual QVariant itemChange(
				GraphicsItemChange change,
				const QVariant &value) override;

	private:

        bool canResize18() const;
        bool resizeBoundary18(const QPointF &position,QWidget *viewport,int &column,int &row) const;
        bool commitResize18(const QVariantMap &before,const QVariantMap &after,bool undo,const QString &text);
        void cancelResize18();
        QSet<QPair<int,int>> m_cells20;
        QPair<int,int> m_anchor20={-1,-3};
        QVariantMap m_resize_before18;
        QPointF m_resize_press18,m_resize_position25,m_resize_scene25;
        int m_resize_column18=-1,m_resize_row18=-1,m_resize_start18=0;
        bool m_resize_active18=false,m_resize_cancelled18=false;
		void modelReseted();
		void setUpColumnAndRowMinimumSize();
		void setUpBoundingRect();
		void adjustHandlerPos();
		void setUpHandler();
		void handlerMousePressEvent   (QGraphicsSceneMouseEvent *event);
		void handlerMouseMoveEvent    (QGraphicsSceneMouseEvent *event);
		void handlerMouseReleaseEvent (QGraphicsSceneMouseEvent *event);
		void adjustColumnsWidth();
		void dataChanged(
				const QModelIndex &topLeft,
				const QModelIndex &bottomRight,
				const QVector<int> &roles);
		void headerSectionResized();
		void adjustSize();
		void previousTableDisplayRowChanged();
		void removeUselessNextTable(bool recursive = true);

		QVariantMap m_custom_format;
		QPointer<QAbstractItemModel> m_model;

		QVector<int> m_minimum_column_width;
		int
		m_minimum_row_height,
		m_number_of_row_to_display = 0,
		m_br_margin = 10;

		QSize
		m_current_size,
		m_old_size,
		m_pending_size;

		QRectF m_bounding_rect;

		QetGraphicsHandlerItem m_handler_item;
		QetGraphicsHeaderItem *m_header_item = nullptr;

		QPointer<QetGraphicsTableItem> m_previous_table,
		m_next_table;

		QString m_name;
		QUuid
		m_uuid = QUuid::createUuid(),
		m_pending_previous_table_uuid;
};

#endif // QetGraphicsTableItem_H
