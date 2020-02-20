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
#include "qetgraphicsheaderitem.h"
#include "qabstractitemmodel.h"

#include <QFontMetrics>
#include <QPainter>

/**
 * @brief QetGraphicsHeaderItem::QetGraphicsHeaderItem
 * @param parent
 */
QetGraphicsHeaderItem::QetGraphicsHeaderItem(QGraphicsItem *parent) :
    QGraphicsObject(parent)
{
	m_margin = QMargins(5,5,5,5);
}

/**
 * @brief QetGraphicsHeaderItem::setModel
 * Set the model presented by this item
 * Since QetGraphicsHeaderItem don't take ownership of model,
 * if item already have a model, it's your responsability to delete it.
 * @param model
 */
void QetGraphicsHeaderItem::setModel(QAbstractItemModel *model)
{
    m_model = model;
	reset();
}

/**
 * @brief QetGraphicsHeaderItem::model
 * @return the model that this item is presenting
 */
QAbstractItemModel *QetGraphicsHeaderItem::model() const {
	return m_model;
}

/**
 * @brief QetGraphicsHeaderItem::reset
 * Reset the internal state of the item
 */
void QetGraphicsHeaderItem::reset() {
	setUpMinimumSectionsSize();
}

/**
 * @brief QetGraphicsHeaderItem::boundingRect
 * Reimplemented from QGraphicsObject::boundingRect() const;
 * @return
 */
QRectF QetGraphicsHeaderItem::boundingRect() const {
	return m_bounding_rect;
}

/**
 * @brief QetGraphicsHeaderItem::paint
 * Reimplemented from QGraphicsObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) const;
 * @param painter
 * @param option
 * @param widget
 */
void QetGraphicsHeaderItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)

	painter->save();

	QPen pen;
	pen.setWidthF(0.7);
	pen.setColor(Qt::black);

	QBrush brush = painter->brush();
	brush.setColor(Qt::lightGray);
	brush.setStyle(Qt::SolidPattern);
	painter->setBrush(brush);

	painter->setPen(pen);
	painter->setFont(m_font);
	painter->drawRect(m_current_rect);

	if (!m_model)
	{
		painter->restore();
		return;
	}

		//Draw vertical lines
	auto offset= 0;
	for(auto size : m_current_sections_width)
	{
		QPointF p1(offset+size, m_current_rect.top());
		QPointF p2(offset+size, m_current_rect.bottom());
		painter->drawLine(p1, p2);
		offset += size;
	}

		//Write text of each cell
	QPointF top_left(m_margin.left(), m_margin.top());
	for (auto i= 0 ; i<m_model->columnCount() ; ++i)
	{
		QSize size(m_current_sections_width.at(i), m_section_height - m_margin.top() - m_margin.bottom());
		painter->drawText(QRectF(top_left, size), Qt::AlignCenter, m_model->headerData(i, Qt::Horizontal).toString());
		top_left.setX(top_left.x() + size.width());
	}

	painter->restore();
}

/**
 * @brief QetGraphicsHeaderItem::rect
 * @return the current rect of the item aka the size of rectangle painted.
 */
QRect QetGraphicsHeaderItem::rect() const {
	return m_current_rect;
}

void QetGraphicsHeaderItem::resizeSection(int logicalIndex, int size)
{
	if (m_model && logicalIndex<m_model->columnCount())
	{
		prepareGeometryChange();
		m_current_sections_width.replace(logicalIndex, size);
		m_current_rect.setWidth(std::accumulate(m_current_sections_width.begin(), m_current_sections_width.end(), 0));
		setUpBoundingRect();
	}
}

/**
 * @brief QetGraphicsHeaderItem::sectionSize
 * @param logical_index
 * @return the width (or height for vertical headers) of the given logicalIndex.
 */
int QetGraphicsHeaderItem::sectionSize(int logical_index) const
{
	if (logical_index>=0 && logical_index<m_current_sections_width.size()) {
		return m_current_sections_width.at(logical_index);
	} else {
		return -1;
	}
}

/**
 * @brief QetGraphicsHeaderItem::setUpMinimumSectionsSize
 * Setup the minimum section size of the item
 */
void QetGraphicsHeaderItem::setUpMinimumSectionsSize()
{
	if (!m_model) {
		return;
	}

	QFontMetrics metrics(m_font);
		//Set the height of row;
	m_section_height = metrics.boundingRect("HEIGHT TEST").height() + m_margin.top() + m_margin.bottom();

	m_sections_minimum_width.clear();
	m_sections_minimum_width.resize(m_model->columnCount());

	for (auto i= 0 ; i<m_model->columnCount() ; ++i)
	{
		auto str = m_model->headerData(i, Qt::Horizontal).toString();
		m_sections_minimum_width.replace(i, metrics.boundingRect(str).width() + m_margin.left() + m_margin.right());
	}

	m_current_sections_width = m_sections_minimum_width;

	m_minimum_rect.setRect(0,0, std::accumulate(m_sections_minimum_width.begin(), m_sections_minimum_width.end(), 0), m_section_height);
	m_current_rect = m_minimum_rect;

	setUpBoundingRect();
}

/**
 * @brief QetGraphicsHeaderItem::setUpBoundingRect
 * Setup the bounding rect of the item
 */
void QetGraphicsHeaderItem::setUpBoundingRect() {
	m_bounding_rect = m_current_rect.adjusted(-10, -10, 10, 10);
}
