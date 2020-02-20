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
#include "qetgraphicstableitem.h"
#include "diagram.h"
#include "qetgraphicsheaderitem.h"

#include <QAbstractItemModel>
#include <QFontMetrics>
#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

/**
 * @brief QetGraphicsTableItem::QetGraphicsTableItem
 * Default constructor
 * @param parent
 */
QetGraphicsTableItem::QetGraphicsTableItem(QGraphicsItem *parent) :
	QetGraphicsItem(parent)
{
	setFlag(QGraphicsItem::ItemIsMovable, true);
	setFlag(QGraphicsItem::ItemIsSelectable, true);
	setAcceptHoverEvents(true);
	setUpHandler();

	m_margin = QMargins(5,3,15,3);
		//A litle bounding rect before model is set,
		//then user can already grab this item, even if model is not already set
	m_bounding_rect.setRect(m_br_margin/-2, m_br_margin/-2, 50, 50);

	connect(this, &QetGraphicsTableItem::xChanged, this, &QetGraphicsTableItem::adjustHandlerPos);
	connect(this, &QetGraphicsTableItem::yChanged, this, &QetGraphicsTableItem::adjustHandlerPos);

	m_header_item = new QetGraphicsHeaderItem(this);



	/*******ONLY FOR TEST DURING DEVEL*********/
	auto model = new QStandardItemModel(this);
	int r = 10;
	int c = 5;

	for (int row = 0; row < r; ++row)
	{
		for (int column = 0; column < c; ++column) {
			QStandardItem *item = new QStandardItem(QString("row %0, column %1").arg(row).arg(column));
			model->setItem(row, column, item);
		}
	}
	model->setHeaderData(0, Qt::Horizontal, "Label");
	model->setHeaderData(1, Qt::Horizontal, "Folio");
	model->setHeaderData(2, Qt::Horizontal, "Fonction");
	model->setHeaderData(3, Qt::Horizontal, "Fabricant");
	model->setHeaderData(4, Qt::Horizontal, "Installation");
	this->setModel(model);
	this->setPos(50,50);
}

QetGraphicsTableItem::~QetGraphicsTableItem()
{
}

/**
 * @brief QetGraphicsTableItem::setModel
 * Set the model presented by this item.
 * Since QetGraphicsTableItem don't take ownership of model,
 * if item already have a model, it's your responsibility to delete it.
 * @param model
 */
void QetGraphicsTableItem::setModel(QAbstractItemModel *model)
{
	m_model = model;
	reset();

	m_header_item->setModel(model);
	adjustColumnsWidth();
	m_header_item->setPos(0, -m_header_item->rect().height());
}

/**
 * @brief QetGraphicsTableItem::model
 * @return The model that this item is presenting
 */
QAbstractItemModel *QetGraphicsTableItem::model() const {
	return m_model;
}

/**
 * @brief QetGraphicsTableItem::reset
 * Reset the internal state of the item
 */
void QetGraphicsTableItem::reset()
{
	setUpColumnAndRowMinimumSize();
	setUpBoundingRect();
	adjustColumnsWidth();
}

/**
 * @brief QetGraphicsTableItem::boundingRect
 * Reimplemented from QGraphicsObject
 * @return
 */
QRectF QetGraphicsTableItem::boundingRect() const {
	return m_bounding_rect;
}

/**
 * @brief QetGraphicsTableItem::paint
 * Draw the table
 * @param painter
 * @param option
 * @param widget
 */
void QetGraphicsTableItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)

	painter->save();

	QPen pen;
	pen.setWidthF(0.7);
	pen.setColor(Qt::black);
	painter->setPen(pen);
	painter->setFont(m_font);

	painter->drawRect(m_current_rect);

	if(!m_model)
	{
		painter->restore();
		return;
	}

		//Draw vertical lines
	auto offset= 0;
	for(auto i=0 ; i<m_model->columnCount() ; ++i)
	{
		QPointF p1(offset+m_header_item->sectionSize(i), m_current_rect.top());
		QPointF p2(offset+m_header_item->sectionSize(i), m_current_rect.bottom());
		painter->drawLine(p1, p2);
		offset += m_header_item->sectionSize(i);
	}

		//Draw horizontal lines
	auto cell_height = m_current_rect.height()/m_model->rowCount();
	for(auto i= 1 ; i-1<m_model->rowCount() ; ++i)
	{
		QPointF p1(m_current_rect.left(), cell_height*i);
		QPointF p2(m_current_rect.right(), cell_height*i);
		painter->drawLine(p1, p2);
	}

		//Write text of each cell
	for (auto i= 0 ; i<m_model->rowCount() ; ++i)
	{
		QPointF top_left(m_margin.left(), i==0? 0 : cell_height*i);

		for(auto j= 0 ; j<m_model->columnCount() ; ++j)
		{
				//In first iteration the top left X is margin left, in all other iteration the top left X is stored in m_column_size
			if (j>0) {
				top_left.setX(top_left.x() + m_header_item->sectionSize(j-1));
			}
			QSize size(m_header_item->sectionSize(j),
					   cell_height - m_margin.top() - m_margin.bottom());
			painter->drawText(QRectF(top_left, size), Qt::AlignVCenter|Qt::AlignLeft, m_model->index(i, j).data().toString());
		}
	}

	painter->restore();
}

/**
 * @brief QetGraphicsTableItem::hoverEnterEvent
 * Reimplemented from QetGraphicsItem
 * @param event
 */
void QetGraphicsTableItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	adjustHandlerPos();
	this->scene()->addItem(&m_handler_item);
	m_handler_item.installSceneEventFilter(this);
	QGraphicsObject::hoverEnterEvent(event);
}

/**
 * @brief QetGraphicsTableItem::hoverLeaveEvent
 * Reimplemented from QetGraphicsItem
 * @param event
 */
void QetGraphicsTableItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	m_handler_item.scene()->removeItem(&m_handler_item);
	QGraphicsObject::hoverLeaveEvent(event);
}

/**
 * @brief QetGraphicsTableItem::sceneEventFilter
 * Reimplemented from QetGraphicsItem
 * @param watched
 * @param event
 * @return
 */
bool QetGraphicsTableItem::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
	if (watched == &m_handler_item)
	{
		if(event->type() == QEvent::GraphicsSceneMousePress) //Click
		{
			handlerMousePressEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
			return true;
		}
		else if(event->type() == QEvent::GraphicsSceneMouseMove) //Move
		{
			handlerMouseMoveEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
			return true;
		}
		else if (event->type() == QEvent::GraphicsSceneMouseRelease) //Release
		{
			handlerMouseReleaseEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
			return true;
		}
	}

	return false;
}

/**
 * @brief QetGraphicsTableItem::setUpColumnAndRowMinimumSize
 * Calcule the minimum row height and the minimum column width for each columns
 */
void QetGraphicsTableItem::setUpColumnAndRowMinimumSize()
{
	if (!m_model) {
		return;
	}

	QFontMetrics metrics(m_font);
		//Set the height of row;
	m_row_height = metrics.boundingRect("HEIGHT TEST").height() + m_margin.top() + m_margin.bottom();

	m_minimum_column_width.clear();
	m_minimum_column_width.resize(m_model->columnCount());

		//Get the maximum width of each columns
	for (auto i= 0 ; i<m_model->rowCount() ; ++i)
	{
		for(auto j= 0 ; j<m_model->columnCount() ; ++j)
		{
			auto index = m_model->index(i, j);
			auto width = metrics.boundingRect(index.data().toString()).width();
			m_minimum_column_width.replace(j, std::max(m_minimum_column_width.at(j), width+m_margin.left() + m_margin.right()));
		}
	}

		//Set the minimum size of the table
	m_minimum_rect.setRect(0, 0,
						   std::accumulate(m_minimum_column_width.begin(), m_minimum_column_width.end(), 0),
						   m_row_height*m_model->rowCount());

	m_current_rect = m_minimum_rect;
}

/**
 * @brief QetGraphicsTableItem::setUpBoundingRect
 */
void QetGraphicsTableItem::setUpBoundingRect() {
	m_bounding_rect = m_current_rect.adjusted(-m_br_margin, -m_br_margin, m_br_margin, m_br_margin);
}

/**
 * @brief QetGraphicsTableItem::adjustHandlerPos
 * Adjust the pos of the handler item
 */
void QetGraphicsTableItem::adjustHandlerPos()
{
	if(m_handler_item.scene()) {
		m_handler_item.setPos(mapToScene(m_current_rect.bottomRight()));
	}
}

/**
 * @brief QetGraphicsTableItem::setUpHandler
 */
void QetGraphicsTableItem::setUpHandler()
{
	m_handler_item.setColor(Qt::blue);
	m_handler_item.setZValue(this->zValue() + 1);
}

void QetGraphicsTableItem::handlerMousePressEvent(QGraphicsSceneMouseEvent *event) {
	Q_UNUSED(event)
}

void QetGraphicsTableItem::handlerMouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	auto new_handler_pos = Diagram::snapToGrid(event->scenePos());
	QSize size = QRectF(m_current_rect.topLeft(), mapFromScene(new_handler_pos)).size().toSize();

	m_handler_item.setPos(mapToScene(std::max(m_minimum_rect.width(), size.width()),
									 std::max(m_minimum_rect.height(), size.height())));

	QRect new_rect = QRectF(QPointF(0,0), mapFromScene(m_handler_item.scenePos())).toRect();

	if (new_rect != m_current_rect)
	{
		prepareGeometryChange();
		m_current_rect = new_rect;
		setUpBoundingRect();
		adjustColumnsWidth();
	}
}

void QetGraphicsTableItem::handlerMouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	Q_UNUSED(event)
}

/**
 * @brief QetGraphicsTableItem::adjustColumnsWidth
 * Adjust the size of each column according to the current table width by setting the sectionSize of the header item
 */
void QetGraphicsTableItem::adjustColumnsWidth()
{
	auto minimum_width = std::accumulate(m_minimum_column_width.begin(), m_minimum_column_width.end(), 0);
	auto a = m_current_rect.width() - minimum_width;
	auto b = a/m_model->columnCount();

	for(auto i= 0 ; i<m_minimum_column_width.size() ; ++i) {
		m_header_item->resizeSection(i, m_minimum_column_width.at(i) + b);
	}
}
