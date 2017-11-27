/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "elementtextitemgroup.h"
#include "dynamicelementtextitem.h"
#include "element.h"
#include "diagram.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>

bool sorting(QGraphicsItem *qgia, QGraphicsItem *qgib)
{
	return qgia->pos().y() < qgib->pos().y();
}

/**
 * @brief ElementTextItemGroup::ElementTextItemGroup
 * @param parent
 */
ElementTextItemGroup::ElementTextItemGroup(const QString &name, Element *parent) :
	QGraphicsItemGroup(parent),
	m_name(name),
	m_element(parent)
{
	setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsFocusable);
}

ElementTextItemGroup::~ElementTextItemGroup()
{}

/**
 * @brief ElementTextItemGroup::addToGroup
 * @param item
 */
void ElementTextItemGroup::addToGroup(QGraphicsItem *item)
{
	if(item->type() == DynamicElementTextItem::Type)
	{
		item->setFlag(QGraphicsItem::ItemIsSelectable, false);
		QGraphicsItemGroup::addToGroup(item);	
		updateAlignement();
		
		DynamicElementTextItem *deti = qgraphicsitem_cast<DynamicElementTextItem *>(item);
		connect(deti, &DynamicElementTextItem::fontSizeChanged,      this, &ElementTextItemGroup::updateAlignement);
		connect(deti, &DynamicElementTextItem::textChanged,          this, &ElementTextItemGroup::updateAlignement);
		connect(deti, &DynamicElementTextItem::textFromChanged,      this, &ElementTextItemGroup::updateAlignement);
		connect(deti, &DynamicElementTextItem::infoNameChanged,      this, &ElementTextItemGroup::updateAlignement);
		connect(deti, &DynamicElementTextItem::compositeTextChanged, this, &ElementTextItemGroup::updateAlignement);	
	}
}

/**
 * @brief ElementTextItemGroup::removeFromGroup
 * @param item
 */
void ElementTextItemGroup::removeFromGroup(QGraphicsItem *item)
{
	QGraphicsItemGroup::removeFromGroup(item);
	item->setFlag(QGraphicsItem::ItemIsSelectable, true);
	updateAlignement();
	
	if(DynamicElementTextItem *deti = qgraphicsitem_cast<DynamicElementTextItem *>(item))
	{
		disconnect(deti, &DynamicElementTextItem::fontSizeChanged,      this, &ElementTextItemGroup::updateAlignement);
		disconnect(deti, &DynamicElementTextItem::textChanged,          this, &ElementTextItemGroup::updateAlignement);
		disconnect(deti, &DynamicElementTextItem::textFromChanged,      this, &ElementTextItemGroup::updateAlignement);
		disconnect(deti, &DynamicElementTextItem::infoNameChanged,      this, &ElementTextItemGroup::updateAlignement);
		disconnect(deti, &DynamicElementTextItem::compositeTextChanged, this, &ElementTextItemGroup::updateAlignement);
	}
}

/**
 * @brief ElementTextItemGroup::setAlignement
 * Set the alignement of this group
 * @param alignement
 */
void ElementTextItemGroup::setAlignement(Qt::Alignment alignement)
{
	m_alignement = alignement;
	updateAlignement();
}

Qt::Alignment ElementTextItemGroup::alignment() const
{
	return m_alignement;
}

/**
 * @brief ElementTextItemGroup::setAlignement
 * Update the alignement of the items in this group, according
 * to the current alignement.
 * @param alignement
 */
void ElementTextItemGroup::updateAlignement()
{
	QList <QGraphicsItem *> texts = childItems();
	if (texts.size() > 1)
	{
		prepareGeometryChange();
		std::sort(texts.begin(), texts.end(), sorting);
		
		qreal y_offset =0;
		
		if(m_alignement == Qt::AlignLeft)
		{
			QPointF ref = texts.first()->pos();
				
			for(QGraphicsItem *item : texts)
			{
				item->setPos(ref.x(), ref.y()+y_offset);
				y_offset+=item->boundingRect().height();
			}
			return;
		}
		else if(m_alignement == Qt::AlignVCenter)
		{
			QPointF ref(texts.first()->pos().x() + texts.first()->boundingRect().width()/2,
						texts.first()->pos().y());
			
			for(QGraphicsItem *item : texts)
			{
				item->setPos(ref.x() - item->boundingRect().width()/2,
							 ref.y() + y_offset);
				y_offset+=item->boundingRect().height();
			}
			return;
				
		}
		else if (m_alignement == Qt::AlignRight)
		{
			QPointF ref(texts.first()->pos().x() + texts.first()->boundingRect().width(),
						texts.first()->pos().y());
			
			for(QGraphicsItem *item : texts)
			{
				item->setPos(ref.x() - item->boundingRect().width(),
							 ref.y() + y_offset);
				y_offset+=item->boundingRect().height();
			}
			return;
		}
	}
}

/**
 * @brief ElementTextItemGroup::setName
 * @param name Set the name of this group
 */
void ElementTextItemGroup::setName(QString name)
{
	m_name = name;
}

/**
 * @brief ElementTextItemGroup::texts
 * @return Every texts in this group
 */
QList<DynamicElementTextItem *> ElementTextItemGroup::texts() const
{
	QList<DynamicElementTextItem *> list;
	for(QGraphicsItem *qgi : childItems())
	{
		if(qgi->type() == DynamicElementTextItem::Type)
			list << static_cast<DynamicElementTextItem *>(qgi);
	}
	return list;
}

/**
 * @brief ElementTextItemGroup::diagram
 * @return The diagram of this group, or nullptr if this group is not in a diagram
 */
Diagram *ElementTextItemGroup::diagram() const
{
	if(scene())
		return static_cast<Diagram *>(scene());
	else
		return nullptr;
}

/**
 * @brief ElementTextItemGroup::toXml
 * Export data of this group to xml
 * @param dom_document
 * @return 
 */
QDomElement ElementTextItemGroup::toXml(QDomDocument &dom_document) const
{
	QDomElement dom_element = dom_document.createElement(this->xmlTaggName());
	dom_element.setAttribute("name", m_name);

	QMetaEnum me = QMetaEnum::fromType<Qt::Alignment>();
	dom_element.setAttribute("alignment", me.valueToKey(m_alignement));
	
	QDomElement dom_texts = dom_document.createElement("texts");
	for(DynamicElementTextItem *deti : texts())
	{
		QDomElement text = dom_document.createElement("text");
		text.setAttribute("uuid", deti->uuid().toString());
		dom_texts.appendChild(text);
	}
	
	dom_element.appendChild(dom_texts);
	return dom_element;
}

/**
 * @brief ElementTextItemGroup::fromXml
 * Import data of this group from xml
 * @param dom_element
 */
void ElementTextItemGroup::fromXml(QDomElement &dom_element)
{
	if (dom_element.tagName() != xmlTaggName()) {
		qDebug() << "ElementTextItemGroup::fromXml : Wrong tagg name";
		return;
	}
	
	m_name = dom_element.attribute("name", "no name");
	QMetaEnum me = QMetaEnum::fromType<Qt::Alignment>();
	m_alignement = Qt::Alignment(me.keyToValue(dom_element.attribute("alignment").toStdString().data()));
	
	for(QDomElement text : QET::findInDomElement(dom_element, "texts", "text"))
	{
		DynamicElementTextItem *deti = nullptr;
		QUuid uuid(text.attribute("uuid"));
		
		for(DynamicElementTextItem *txt : m_element->dynamicTextItems())
			if(txt->uuid() == uuid)
				deti = txt;
		
		if (deti)
			m_element->addTextToGroup(deti, this);
	}
}

/**
 * @brief ElementTextItemGroup::paint
 * @param painter
 * @param option
 * @param widget
 */
void ElementTextItemGroup::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if(isSelected())
	{
		painter->save();
		QPen t;
		t.setColor(Qt::gray);
		t.setStyle(Qt::DashDotLine);
		t.setCosmetic(true);
		painter->setPen(t);
		painter->drawRoundRect(boundingRect().adjusted(1, 1, -1, -1), 10, 10);
		painter->restore();
	}
}

/**
 * @brief ElementTextItemGroup::boundingRect
 * @return 
 */
QRectF ElementTextItemGroup::boundingRect() const
{
		//If we refer to the Qt doc, the bounding rect of a QGraphicsItemGroup,
		//is the bounding of all childrens in the group
		//When add an item in the group, the bounding rect is good, but
		//if we move an item already in the group, the bounding rect of the group stay unchanged.
		//We reimplement this function to avoid this behavior.
	QRectF rect;
	for(QGraphicsItem *qgi : childItems())
	{
		QRectF r(qgi->pos(), QSize(qgi->boundingRect().width(), qgi->boundingRect().height()));
		rect = rect.united(r);
	}
	return rect;
}

/**
 * @brief ElementTextItemGroup::mousePressEvent
 * @param event
 */
void ElementTextItemGroup::mousePressEvent(QGraphicsSceneMouseEvent *event)
{	
	if(event->button() == Qt::LeftButton)
	{
		m_first_move = true;
		if(event->modifiers() & Qt::ControlModifier)
			setSelected(!isSelected());
	}
	
	QGraphicsItemGroup::mousePressEvent(event);
}

/**
 * @brief ElementTextItemGroup::mouseMoveEvent
 * @param event
 */
void ElementTextItemGroup::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if(isSelected() && event->buttons() & Qt::LeftButton)
	{
		if(diagram() && m_first_move)
			diagram()->beginMoveElementTexts(this);
		
		QPointF old_pos = pos();
		if(m_first_move)
			m_mouse_to_origin_movement = old_pos - event->buttonDownScenePos(Qt::LeftButton);
		
		QPointF expected_pos = event->scenePos() + m_mouse_to_origin_movement;
		setPos(Diagram::snapToGrid(expected_pos));
		
		QPointF effective_movement = pos() - old_pos;
		if(diagram())
			diagram()->continueMoveElementTexts(effective_movement);
	}
	else
		event->ignore();
	
	if(m_first_move)
		m_first_move = false;
}

/**
 * @brief ElementTextItemGroup::mouseReleaseEvent
 * @param event
 */
void ElementTextItemGroup::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if(diagram())
		diagram()->endMoveElementTexts();
	
	if(!(event->modifiers() & Qt::ControlModifier))
		QGraphicsItemGroup::mouseReleaseEvent(event);
}

/**
 * @brief ElementTextItemGroup::keyPressEvent
 * @param event
 */
void ElementTextItemGroup::keyPressEvent(QKeyEvent *event)
{
	prepareGeometryChange();
	if(event->key() == Qt::Key_A)
		setAlignement(Qt::AlignLeft);
	else if (event->key() == Qt::Key_Z)
		setAlignement(Qt::AlignVCenter);
	else if (event->key() == Qt::Key_E)
		setAlignement(Qt::AlignRight);
}

