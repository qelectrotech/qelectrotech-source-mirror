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
#include "qetgraphicsheaderitem.h"
#include "qabstractitemmodel.h"
#include "qetxml.h"

#include <QFontMetrics>
#include <QPainter>

static int no_model_height = 20;
static int no_model_width = 40;
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
	if (m_model) {
		disconnect(m_model, &QAbstractItemModel::headerDataChanged, this, &QetGraphicsHeaderItem::headerDataChanged);
		disconnect(m_model, &QAbstractItemModel::modelReset, this, &QetGraphicsHeaderItem::modelReseted);
		disconnect(m_model, &QAbstractItemModel::columnsInserted, this, &QetGraphicsHeaderItem::modelReseted);
	}

    m_model = model;
	if (m_model)
	{
		connect(m_model, &QAbstractItemModel::headerDataChanged, this, &QetGraphicsHeaderItem::headerDataChanged);
		connect(m_model, &QAbstractItemModel::modelReset, this, &QetGraphicsHeaderItem::modelReseted);
		connect(m_model, &QAbstractItemModel::columnsInserted, this, &QetGraphicsHeaderItem::modelReseted);
		setUpMinimumSectionsSize();
		m_current_sections_width.clear();
		m_current_sections_width.resize(m_sections_minimum_width.size());

	} else {
		setUpMinimumSectionsSize();
	}
	adjustSize();
}

/**
 * @brief QetGraphicsHeaderItem::model
 * @return the model that this item is presenting
 */
QAbstractItemModel *QetGraphicsHeaderItem::model() const {
	return m_model;
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
	painter->drawRect(m_current_rect);

	if (!m_model)
	{
		painter->restore();
		return;
	}
	painter->setFont(m_model->headerData(0, Qt::Horizontal, Qt::FontRole).value<QFont>());

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
		QSize size(m_current_sections_width.at(i) - m_margin.left() - m_margin.right(), m_section_height - m_margin.top() - m_margin.bottom());
		painter->drawText(QRectF(top_left, size),
						  m_model->headerData(0, Qt::Horizontal, Qt::TextAlignmentRole).toInt(),
						  m_model->headerData(i, Qt::Horizontal).toString());

		top_left.setX(top_left.x() + m_current_sections_width.at(i));
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

/**
 * @brief QetGraphicsHeaderItem::resizeSection
 * @param logicalIndex
 * @param size
 */
void QetGraphicsHeaderItem::resizeSection(int logicalIndex, int size)
{
	if (!m_model)
	{
		m_current_sections_width.clear();
		m_current_sections_width.append(no_model_width);
		m_sections_minimum_width.clear();
		m_sections_minimum_width.append(no_model_width);
		m_current_rect.setWidth(no_model_width);
		setUpBoundingRect();
		update();
		emit sectionResized(0, no_model_width);
		return;
	}

	if (logicalIndex >= m_current_sections_width.size() ||
		m_current_sections_width.at(logicalIndex) == size) {
		return;
	}

	if (m_model &&
		logicalIndex<m_model->columnCount() &&
		size >= m_sections_minimum_width.at(logicalIndex))
	{
		prepareGeometryChange();
		m_current_sections_width.replace(logicalIndex, size);
		m_current_rect.setWidth(std::accumulate(m_current_sections_width.begin(), m_current_sections_width.end(), 0));
		setUpBoundingRect();
		update();
		emit sectionResized(logicalIndex, size);
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
 * @brief QetGraphicsHeaderItem::setMargins
 * @param margins
 */
void QetGraphicsHeaderItem::setMargins(const QMargins &margins)
{
	m_margin = margins;
	headerDataChanged(Qt::Horizontal, 0,1);
}

/**
 * @brief QetGraphicsHeaderItem::toXml
 * save the header to xml
 * @param document
 * @return
 */
QDomElement QetGraphicsHeaderItem::toXml(QDomDocument &document) const
{
	auto dom_element = document.createElement(xmlTagName());
	dom_element.appendChild(QETXML::marginsToXml(document, m_margin));

	return dom_element;
}

/**
 * @brief QetGraphicsHeaderItem::fromXml
 * Restore the header from xml
 * @param element
 */
void QetGraphicsHeaderItem::fromXml(const QDomElement &element)
{
	if (element.tagName() != xmlTagName()) {
		return;
	}

	m_margin = QETXML::marginsFromXml(element.firstChildElement("margins"));
}

/**
 * @brief QetGraphicsHeaderItem::setUpMinimumSectionsSize
 * Setup the minimum section size and height of the item.
 * Not that this function doesn't change the current size of this item.
 */
void QetGraphicsHeaderItem::setUpMinimumSectionsSize()
{
	if (!m_model)
	{
		m_minimum_section_height = no_model_height;
		m_sections_minimum_width.clear();
		m_sections_minimum_width.append(no_model_width);
		m_minimum_width = no_model_width;
		return;
	}

	QFontMetrics metrics(m_model->headerData(0, Qt::Horizontal, Qt::FontRole).value<QFont>());
		//Set the height of row;
	m_minimum_section_height = metrics.boundingRect("HEIGHT TEST").height() + m_margin.top() + m_margin.bottom();

	m_sections_minimum_width.clear();
	m_sections_minimum_width.resize(m_model->columnCount());

	for (auto i= 0 ; i<m_model->columnCount() ; ++i)
	{
		auto str = m_model->headerData(i, Qt::Horizontal).toString();
		m_sections_minimum_width.replace(i, metrics.boundingRect(str).width() + m_margin.left() + m_margin.right());
	}

	m_minimum_width = std::accumulate(m_sections_minimum_width.begin(), m_sections_minimum_width.end(), 0);
}

/**
 * @brief QetGraphicsHeaderItem::setUpBoundingRect
 * Setup the bounding rect of the item
 */
void QetGraphicsHeaderItem::setUpBoundingRect() {
	m_bounding_rect = m_current_rect.adjusted(-10, -10, 10, 10);
}

/**
 * @brief QetGraphicsHeaderItem::headerDataChanged
 * Update the header when data of displayed model change
 * @param orientation
 * @param first
 * @param last
 */
void QetGraphicsHeaderItem::headerDataChanged(Qt::Orientations orientation, int first, int last)
{
	Q_UNUSED(orientation)
	Q_UNUSED(first)
	Q_UNUSED(last)

	setUpMinimumSectionsSize();
	adjustSize();
}

/**
 * @brief QetGraphicsHeaderItem::adjustSize
 * If needed, this function resize the current height and section
 * according to there minimum
 */
void QetGraphicsHeaderItem::adjustSize()
{
	if (m_section_height != m_minimum_section_height)
	{
		m_section_height = m_minimum_section_height;
		m_current_rect.setHeight(m_section_height);
		emit heightResized();
	}

	if(m_current_sections_width.size() == m_sections_minimum_width.size())
	{
		auto old_sections_width = m_current_sections_width;

		for (int i=0 ; i<m_current_sections_width.size() ; ++i)
		{
			if (old_sections_width.at(i) < m_sections_minimum_width.at(i)) {
				resizeSection(i, m_sections_minimum_width.at(i));
			}
		}
	}

	update();
}

void QetGraphicsHeaderItem::modelReseted()
{
	setUpMinimumSectionsSize();
	m_current_sections_width.clear();
	m_current_sections_width.resize(m_sections_minimum_width.size());
	adjustSize();
}
