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
#include "addelementtextcommand.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"

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
	m_name(name)
{
	setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
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
			//Befor add text to group we must to set the text and the group to the same rotation
		item->setRotation(0);
		item->setFlag(QGraphicsItem::ItemIsSelectable, false);
		
		qreal rot = this->rotation();
		this->setRotation(0);
		
		QGraphicsItemGroup::addToGroup(item);
		updateAlignment();
		
		this->setRotation(rot);
		
		DynamicElementTextItem *deti = qgraphicsitem_cast<DynamicElementTextItem *>(item);
		connect(deti, &DynamicElementTextItem::fontSizeChanged,      this, &ElementTextItemGroup::updateAlignment);
		connect(deti, &DynamicElementTextItem::textChanged,          this, &ElementTextItemGroup::updateAlignment);
		connect(deti, &DynamicElementTextItem::textFromChanged,      this, &ElementTextItemGroup::updateAlignment);
		connect(deti, &DynamicElementTextItem::infoNameChanged,      this, &ElementTextItemGroup::updateAlignment);
		connect(deti, &DynamicElementTextItem::compositeTextChanged, this, &ElementTextItemGroup::updateAlignment);	
	}
}

/**
 * @brief ElementTextItemGroup::removeFromGroup
 * @param item
 */
void ElementTextItemGroup::removeFromGroup(QGraphicsItem *item)
{
	QGraphicsItemGroup::removeFromGroup(item);
		//the item transformation is not reseted, we must to do it, because for exemple if the group rotation is 45°
		//When item is removed from group, visually the item is unchanged (so 45°) but if we call item->rotation() the returned value is 0.
	item->resetTransform();
	item->setRotation(this->rotation());
	item->setFlag(QGraphicsItem::ItemIsSelectable, true);
	updateAlignment();
	
	if(DynamicElementTextItem *deti = qgraphicsitem_cast<DynamicElementTextItem *>(item))
	{
		disconnect(deti, &DynamicElementTextItem::fontSizeChanged,      this, &ElementTextItemGroup::updateAlignment);
		disconnect(deti, &DynamicElementTextItem::textChanged,          this, &ElementTextItemGroup::updateAlignment);
		disconnect(deti, &DynamicElementTextItem::textFromChanged,      this, &ElementTextItemGroup::updateAlignment);
		disconnect(deti, &DynamicElementTextItem::infoNameChanged,      this, &ElementTextItemGroup::updateAlignment);
		disconnect(deti, &DynamicElementTextItem::compositeTextChanged, this, &ElementTextItemGroup::updateAlignment);
	}
}

/**
 * @brief ElementTextItemGroup::setAlignement
 * Set the alignement of this group
 * @param alignement
 */
void ElementTextItemGroup::setAlignment(Qt::Alignment alignement)
{
	m_alignment = alignement;
	updateAlignment();
	emit alignmentChanged(alignement);
}

Qt::Alignment ElementTextItemGroup::alignment() const
{
	return m_alignment;
}

/**
 * @brief ElementTextItemGroup::setAlignment
 * Update the alignement of the items in this group, according
 * to the current alignement.
 * @param alignement
 */
void ElementTextItemGroup::updateAlignment()
{
	prepareGeometryChange();
	
	QList <QGraphicsItem *> texts = childItems();
	if (texts.size() > 1)
	{
		prepareGeometryChange();
		std::sort(texts.begin(), texts.end(), sorting);
		
		qreal y_offset = 0;
		
		if(m_alignment == Qt::AlignLeft)
		{
			QPointF ref = texts.first()->pos();
				
			for(QGraphicsItem *item : texts)
			{
				item->setPos(ref.x(), ref.y()+y_offset);
				y_offset+=item->boundingRect().height() + m_vertical_adjustment;
			}
		}
		else if(m_alignment == Qt::AlignVCenter)
		{
			QPointF ref(texts.first()->pos().x() + texts.first()->boundingRect().width()/2,
						texts.first()->pos().y());
			
			for(QGraphicsItem *item : texts)
			{
				item->setPos(ref.x() - item->boundingRect().width()/2,
							 ref.y() + y_offset);
				y_offset+=item->boundingRect().height() + m_vertical_adjustment;
			}	
		}
		else if (m_alignment == Qt::AlignRight)
		{
			QPointF ref(texts.first()->pos().x() + texts.first()->boundingRect().width(),
						texts.first()->pos().y());
			
			for(QGraphicsItem *item : texts)
			{
				item->setPos(ref.x() - item->boundingRect().width(),
							 ref.y() + y_offset);
				y_offset+=item->boundingRect().height() + m_vertical_adjustment;
			}
		}
		
		setTransformOriginPoint(boundingRect().topLeft());
	}
}

/**
 * @brief ElementTextItemGroup::setVerticalAdjustment
 * Set the value of the vertical adjustment to @v.
 * The vertical adjutment is use to adjust the space between the texts of this group.
 * @param v
 */
void ElementTextItemGroup::setVerticalAdjustment(int v)
{
	if(m_vertical_adjustment != v)
	{
		prepareGeometryChange();
		m_vertical_adjustment = v;
		updateAlignment();
		emit verticalAdjustmentChanged(v);
	}
}

/**
 * @brief ElementTextItemGroup::setName
 * @param name Set the name of this group
 */
void ElementTextItemGroup::setName(QString name)
{
	m_name = name;
	emit nameChanged(m_name);
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
 * @brief ElementTextItemGroup::parentElement
 * @return The parent element of this group or nullptr
 */
Element *ElementTextItemGroup::parentElement() const
{
	if(parentItem() && parentItem()->type() == Element::Type)
		return static_cast<Element *>(parentItem());
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
	dom_element.setAttribute("alignment", me.valueToKey(m_alignment));
	
	dom_element.setAttribute("rotation", this->rotation());
	dom_element.setAttribute("vertical_adjustment", m_vertical_adjustment);
	
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
	m_alignment = Qt::Alignment(me.keyToValue(dom_element.attribute("alignment").toStdString().data()));
	
	setRotation(dom_element.attribute("rotation", QString::number(0)).toDouble());
	m_vertical_adjustment = dom_element.attribute("vertical_adjustment").toInt();
	
	if(parentElement())
	{
		for(QDomElement text : QET::findInDomElement(dom_element, "texts", "text"))
		{
			DynamicElementTextItem *deti = nullptr;
			QUuid uuid(text.attribute("uuid"));
			
			for(DynamicElementTextItem *txt : parentElement()->dynamicTextItems())
				if(txt->uuid() == uuid)
					deti = txt;
			
			if (deti)
				parentElement()->addTextToGroup(deti, this);
		}
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

void ElementTextItemGroup::setRotation(qreal angle)
{
	QGraphicsItemGroup::setRotation(angle);
	emit rotationChanged(angle);
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
		{
			m_mouse_to_origin_movement = old_pos - event->buttonDownScenePos(Qt::LeftButton);
			if(parentElement())
				parentElement()->setHighlighted(true);
		}
		
		QPointF expected_pos = event->scenePos() + m_mouse_to_origin_movement;
		event->modifiers() == Qt::ControlModifier ? setPos(expected_pos) : setPos(Diagram::snapToGrid(expected_pos));
		
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
	{
		diagram()->endMoveElementTexts();
		if(parentElement())
			parentElement()->setHighlighted(false);
	}
	
	if(!(event->modifiers() & Qt::ControlModifier))
		QGraphicsItemGroup::mouseReleaseEvent(event);
}

/**
 * @brief ElementTextItemGroup::keyPressEvent
 * @param event
 */
void ElementTextItemGroup::keyPressEvent(QKeyEvent *event)
{	
 	if(event->modifiers() == Qt::ControlModifier)
	{
		if(event->key() == Qt::Key_Left && m_alignment	!= Qt::AlignLeft)
		{
			if(diagram())
				diagram()->undoStack().push(new AlignmentTextsGroupCommand(this, Qt::AlignLeft));
			else
				setAlignment(Qt::AlignLeft);
		}
		else if (event->key() == Qt::Key_Up && m_alignment	!= Qt::AlignVCenter)
		{
			if(diagram())
				diagram()->undoStack().push(new AlignmentTextsGroupCommand(this, Qt::AlignVCenter));
			else
				setAlignment(Qt::AlignVCenter);
		}
		else if (event->key() == Qt::Key_Right && m_alignment	!= Qt::AlignRight)
		{
			if(diagram())
				diagram()->undoStack().push(new AlignmentTextsGroupCommand(this, Qt::AlignRight));
			else
				setAlignment(Qt::AlignRight);
		}
	}
	event->ignore();
}

