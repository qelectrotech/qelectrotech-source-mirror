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
#include "qetgraphicstableitem.h"
#include "diagram.h"
#include "qetgraphicsheaderitem.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"

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
	connect(m_header_item, &QetGraphicsHeaderItem::sectionResized, this, &QetGraphicsTableItem::headerSectionResized);
	connect(m_header_item, &QetGraphicsHeaderItem::heightResized, this, [this]() {
		m_header_item->setPos(0, 0-m_header_item->rect().height());
	});
}

QetGraphicsTableItem::~QetGraphicsTableItem()
{}

/**
 * @brief QetGraphicsTableItem::setModel
 * Set the model presented by this item.
 * Since QetGraphicsTableItem don't take ownership of model,
 * if item already have a model, it's your responsibility to delete it.
 * @param model
 */
void QetGraphicsTableItem::setModel(QAbstractItemModel *model)
{
	if (m_model) {
		disconnect(m_model, &QAbstractItemModel::dataChanged, this, &QetGraphicsTableItem::dataChanged);
	}
	m_model = model;
	m_header_item->setModel(model);

	setUpColumnAndRowMinimumSize();
	adjustSize();

	m_header_item->setPos(0, -m_header_item->rect().height());
	connect(m_model, &QAbstractItemModel::dataChanged, this, &QetGraphicsTableItem::dataChanged);
}

/**
 * @brief QetGraphicsTableItem::model
 * @return The model that this item is presenting
 */
QAbstractItemModel *QetGraphicsTableItem::model() const {
	return m_model;
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
	painter->setFont(m_model->data(model()->index(0,0), Qt::FontRole).value<QFont>());

	painter->drawRect(0,0, m_header_item->rect().width(), m_current_size.height());

	if(!m_model)
	{
		painter->restore();
		return;
	}

		//Draw vertical lines
	auto offset= 0;
	for(auto i=0 ; i<m_model->columnCount() ; ++i)
	{
		QPointF p1(offset+m_header_item->sectionSize(i), 0);
		QPointF p2(offset+m_header_item->sectionSize(i), m_current_size.height());
		painter->drawLine(p1, p2);
		offset += m_header_item->sectionSize(i);
	}

		//Draw horizontal lines
	auto cell_height =  static_cast<double>(m_current_size.height())/static_cast<double>(m_model->rowCount());
	for(auto i= 1 ; i-1<m_model->rowCount() ; ++i)
	{
		QPointF p1(m_header_item->rect().left(), cell_height*i);
		QPointF p2(m_header_item->rect().right(), cell_height*i);
		painter->drawLine(p1, p2);
	}

		//Write text of each cell
	for (auto i= 0 ; i<m_model->rowCount() ; ++i)
	{
		QPointF top_left(m_margin.left(), i==0? m_margin.top() : cell_height*i + m_margin.top());

		for(auto j= 0 ; j<m_model->columnCount() ; ++j)
		{
				//In first iteration the top left X is margin left, in all other iteration the top left X is stored in m_column_size
			if (j>0) {
				top_left.setX(top_left.x() + m_header_item->sectionSize(j-1));
			}
			QSize size(m_header_item->sectionSize(j) - m_margin.left() - m_margin.right(),
					   static_cast<int>(cell_height) - m_margin.top() - m_margin.bottom());
			painter->drawText(QRectF(top_left, size),
							  m_model->data(m_model->index(0,0), Qt::TextAlignmentRole).toInt(),
							  m_model->index(i, j).data().toString());
		}
	}

	painter->restore();
}

/**
 * @brief QetGraphicsTableItem::setMargins
 * @param margins
 */
void QetGraphicsTableItem::setMargins(const QMargins &margins)
{
	m_margin = margins;
	setUpColumnAndRowMinimumSize();
	adjustSize();
	update();
}

/**
 * @brief QetGraphicsTableItem::setSize
 * Set the current size of the table to @size
 * @param size
 */
void QetGraphicsTableItem::setSize(const QSize &size)
{
	auto new_size = size;
	if (new_size.width() < minimumSize().width())  {
		new_size.setWidth(minimumSize().width());
	}
	if (new_size.height() < minimumSize().height()) {
		new_size.setHeight(minimumSize().height());
	}

	prepareGeometryChange();
	m_current_size = new_size;
	adjustColumnsWidth();
	setUpBoundingRect();
}

/**
 * @brief QetGraphicsTableItem::size
 * @return The current size of the table
 */
QSize QetGraphicsTableItem::size() const
{
	QSize size_(m_header_item->rect().width(), m_current_size.height());
	return size_;
}

/**
 * @brief QetGraphicsTableItem::minimumSize
 * @return the minimum size the table can be
 * The returned size take care of the table's minimum width, but also the header item's minimum width
 */
QSize QetGraphicsTableItem::minimumSize() const
{
	QSize s(std::accumulate(m_minimum_column_width.begin(), m_minimum_column_width.end(), 0), m_minimum_row_height*m_model->rowCount());
	return s;
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
 * this function doesn't change the geometry of the table.
 */
void QetGraphicsTableItem::setUpColumnAndRowMinimumSize()
{
	if (!m_model) {
		return;
	}

	QFontMetrics metrics(m_model->data(model()->index(0,0), Qt::FontRole).value<QFont>());
		//Set the height of row;
	m_minimum_row_height = metrics.boundingRect("HEIGHT TEST").height() + m_margin.top() + m_margin.bottom();

	m_minimum_column_width = m_header_item->minimumSectionWidth();

		//Get the maximum width of each columns
	for (auto row= 0 ; row<m_model->rowCount() ; ++row)
	{
		for(auto col= 0 ; col<m_model->columnCount() ; ++col)
		{
			auto index = m_model->index(row, col);
			auto width = metrics.boundingRect(index.data().toString()).width();
			m_minimum_column_width.replace(col, std::max(m_minimum_column_width.at(col), width + m_margin.left() + m_margin.right()));
		}
	}
}

/**
 * @brief QetGraphicsTableItem::setUpBoundingRect
 */
void QetGraphicsTableItem::setUpBoundingRect()
{
	QSize header_size = m_header_item->rect().size();
	QRect rect(0, -header_size.height(), header_size.width(), m_current_size.height() + header_size.height());
	m_bounding_rect = rect.adjusted(-m_br_margin, -m_br_margin, m_br_margin, m_br_margin);
}

/**
 * @brief QetGraphicsTableItem::adjustHandlerPos
 * Adjust the pos of the handler item
 */
void QetGraphicsTableItem::adjustHandlerPos() {
	m_handler_item.setPos(mapToScene(QRect(QPoint(0,0), size()).bottomRight()));
}

/**
 * @brief QetGraphicsTableItem::setUpHandler
 */
void QetGraphicsTableItem::setUpHandler()
{
	m_handler_item.setColor(Qt::blue);
	m_handler_item.setZValue(this->zValue() + 1);
}

void QetGraphicsTableItem::handlerMousePressEvent(QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(event)
	m_old_size = size();
		//User start to resize the table, disconnect the signal to avoid double paint.
	disconnect(m_header_item, &QetGraphicsHeaderItem::sectionResized, this, &QetGraphicsTableItem::headerSectionResized);
}

void QetGraphicsTableItem::handlerMouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	auto new_handler_pos = Diagram::snapToGrid(event->scenePos());
	QSize size_ = QRectF(QPointF(0,0), mapFromScene(new_handler_pos)).size().toSize();

	QPoint new_pos(std::max(minimumSize().width(), size_.width()),
				   std::max(minimumSize().height(), size_.height()));
	m_handler_item.setPos(mapToScene(new_pos));

	QSize new_size(new_pos.x(), new_pos.y());
	if (new_size != size()) {
		setSize(new_size);
	}
}

void QetGraphicsTableItem::handlerMouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(event)
	if (diagram())
	{
		auto undo = new QPropertyUndoCommand(this, "size", m_old_size, size());
		undo->setAnimated();
		undo->setText(tr("Modifier la géometrie d'un tableau"));
		diagram()->undoStack().push(undo);
	}
		//User finish to resize the table, we can reconnect now
	connect(m_header_item, &QetGraphicsHeaderItem::sectionResized, this, &QetGraphicsTableItem::headerSectionResized);
}

/**
 * @brief QetGraphicsTableItem::adjustColumnsWidth
 * Adjust the size of each column according to the current table width by setting the sectionSize of the header item
 */
void QetGraphicsTableItem::adjustColumnsWidth()
{
	auto a = m_current_size.width() - minimumSize().width();
	auto b = a/std::max(1,m_model->columnCount()); //avoid divide by 0

	for(auto i= 0 ; i<m_model->columnCount() ; ++i) {
		m_header_item->resizeSection(i, std::max(m_minimum_column_width.at(i), m_header_item->minimumSectionWidth().at(i)) + b);
	}
}

void QetGraphicsTableItem::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
	Q_UNUSED(topLeft)
	Q_UNUSED(bottomRight)
	Q_UNUSED(roles)

	auto size_ = size();
	setUpColumnAndRowMinimumSize();
	adjustSize();
	setSize(size_);
}

/**
 * @brief QetGraphicsTableItem::headerSectionResized
 * Connected to the header signal QetGraphicsTableItem sectionResized
 */
void QetGraphicsTableItem::headerSectionResized()
{
	auto header_size = m_header_item->rect().size();
	auto size_ = size();
	size_.setWidth(header_size.width());

	m_current_size = size_;
	prepareGeometryChange();
	setUpBoundingRect();
}

/**
 * @brief QetGraphicsTableItem::adjustSize
 * If needed, this function resize the current height and width of table
 * according to there minimum
 */
void QetGraphicsTableItem::adjustSize()
{
	if (m_current_size.height() < minimumSize().height())
	{
		prepareGeometryChange();
		m_current_size.setHeight(minimumSize().height());
		setUpBoundingRect();
		update();
	}

	if (m_current_size.width() < minimumSize().width())
	{
		prepareGeometryChange();
		m_current_size.setWidth(minimumSize().width());
		adjustColumnsWidth();
		setUpBoundingRect();
		update();
	}
}
