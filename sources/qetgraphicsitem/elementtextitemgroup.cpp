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
#include "crossrefitem.h"
#include "qetapp.h"
#include "masterelement.h"
#include "qgraphicsitemutility.h"

#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <utility>

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
	m_parent_element(parent)
{
	setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
	connect(parent, &Element::linkedElementChanged, this, &ElementTextItemGroup::updateXref);
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
			//Befor add text to this group we must to set the text at the same rotation of this group		
		if((item->rotation() != rotation()) && !m_block_alignment_update)
			item->setRotation(rotation());
		
		QGraphicsItemGroup::addToGroup(item);
		updateAlignment();
		
		DynamicElementTextItem *deti = qgraphicsitem_cast<DynamicElementTextItem *>(item);
		connect(deti, &DynamicElementTextItem::fontSizeChanged,      this, &ElementTextItemGroup::updateAlignment);
		connect(deti, &DynamicElementTextItem::textChanged,          this, &ElementTextItemGroup::updateAlignment);
		connect(deti, &DynamicElementTextItem::textFromChanged,      this, &ElementTextItemGroup::updateAlignment);
		connect(deti, &DynamicElementTextItem::infoNameChanged,      this, &ElementTextItemGroup::updateAlignment);
		connect(deti, &DynamicElementTextItem::compositeTextChanged, this, &ElementTextItemGroup::updateAlignment);
		connect(deti, &DynamicElementTextItem::plainTextChanged,     this, &ElementTextItemGroup::updateAlignment);
		connect(deti, &DynamicElementTextItem::textWidthChanged,     this, &ElementTextItemGroup::updateAlignment);
		
		connect(deti, &DynamicElementTextItem::textFromChanged, this, &ElementTextItemGroup::updateXref);
		connect(deti, &DynamicElementTextItem::infoNameChanged, this, &ElementTextItemGroup::updateXref);
		
		updateXref();
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
		disconnect(deti, &DynamicElementTextItem::plainTextChanged,     this, &ElementTextItemGroup::updateAlignment);
		disconnect(deti, &DynamicElementTextItem::textWidthChanged,     this, &ElementTextItemGroup::updateAlignment);
		
		disconnect(deti, &DynamicElementTextItem::textFromChanged, this, &ElementTextItemGroup::updateXref);
		disconnect(deti, &DynamicElementTextItem::infoNameChanged, this, &ElementTextItemGroup::updateXref);
		
		updateXref();
	}
}

/**
 * @brief ElementTextItemGroup::blockAlignmentUpdate
 * If true, the texts in this group are never aligned, moved, rotated etc...
 * the texts stay as it was, until blockAlignmentUpdate is set to false.
 * @param block
 */
void ElementTextItemGroup::blockAlignmentUpdate(bool block)
{
	m_block_alignment_update = block;
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
	if(m_block_alignment_update)
		return;
	
	prepareGeometryChange();
	
	QList <DynamicElementTextItem *> texts = this->texts();
	
	qreal rotation_ = rotation();
	
		//Set the rotation of this group to 0° relative to the scene
	qreal rot = rotation();
	QGraphicsItem *parent = parentItem();
	while (parent) {
		rot += parent->rotation();
		parent = parent->parentItem();
	}
	if(rot != 0)
		setRotation(rotation() - rot);
	
	
	if(texts.size() == 1)
	{
		prepareGeometryChange();
		
		QGraphicsItem *first = texts.first();
		setPos(mapFromScene(first->mapToScene(pos())));
		first->setPos(0,0);
	}
	else if (texts.size() > 1)
	{
		qreal width = 0;
		for(QGraphicsItem *item : texts)
			if(item->boundingRect().width() > width)
				width = item->boundingRect().width();
		
		prepareGeometryChange();
		std::sort(texts.begin(), texts.end(), sorting);
		
		qreal y_offset = 0;
		
		if(m_alignment == Qt::AlignLeft)
		{
			QPointF ref = texts.first()->pos();
				
			for(QGraphicsItem *item : texts)
			{
				item->setPos(0, ref.y()+y_offset);
				y_offset+=item->boundingRect().height() + m_vertical_adjustment;
			}
		}
		else if(m_alignment == Qt::AlignVCenter)
		{
			QPointF ref(width/2,0);
			
			for(QGraphicsItem *item : texts)
			{
				item->setPos(ref.x() - item->boundingRect().width()/2,
							 ref.y() + y_offset);
				y_offset+=item->boundingRect().height() + m_vertical_adjustment;
			}	
		}
		else if (m_alignment == Qt::AlignRight)
		{
			QPointF ref(width,0);
			
			for(QGraphicsItem *item : texts)
			{
				item->setPos(ref.x() - item->boundingRect().width(),
							 ref.y() + y_offset);
				y_offset+=item->boundingRect().height() + m_vertical_adjustment;
			}
		}
	}
	
		//Restor the rotation
	setRotation(rotation_);
	
	if(m_Xref_item)
		m_Xref_item->autoPos();
	if(m_slave_Xref_item)
		adjustSlaveXrefPos();
	if(m_hold_to_bottom_of_page)
		autoPos();
}

/**
 * @brief ElementTextItemGroup::setVerticalAdjustment
 * Set the value of the vertical adjustment to @v.
 * The vertical adjutment is use to adjust the space between the texts of this group.
 * @param v
 */
void ElementTextItemGroup::setVerticalAdjustment(int v)
{
	prepareGeometryChange();
	m_vertical_adjustment = v;
	updateAlignment();
	emit verticalAdjustmentChanged(v);
}

/**
 * @brief ElementTextItemGroup::setName
 * @param name Set the name of this group
 */
void ElementTextItemGroup::setName(QString name)
{
	m_name = std::move(name);
	emit nameChanged(m_name);
}

void ElementTextItemGroup::setHoldToBottomPage(bool hold)
{
	if(m_hold_to_bottom_of_page == hold)
		return;
	
	m_hold_to_bottom_of_page = hold;
	if(m_hold_to_bottom_of_page)
	{
		setFlag(QGraphicsItem::ItemIsSelectable, false);
		setFlag(QGraphicsItem::ItemIsMovable, false);
		connect(m_parent_element, &Element::yChanged, this, &ElementTextItemGroup::autoPos);
		connect(m_parent_element, &Element::rotationChanged, this, &ElementTextItemGroup::autoPos);
		if(m_parent_element->linkType() == Element::Master)
		{
				//We use timer to let the time of the parent element xref to be updated, befor update the position of this group
				//because the position of this group is related to the size of the parent element Xref
			m_linked_changed_timer = connect(m_parent_element, &Element::linkedElementChanged,
											 [this]() {QTimer::singleShot(200, this, &ElementTextItemGroup::autoPos);});
			if(m_parent_element->diagram())
				m_XrefChanged_timer = connect(m_parent_element->diagram()->project(), &QETProject::XRefPropertiesChanged,
											  [this]()	{QTimer::singleShot(200, this, &ElementTextItemGroup::autoPos);});
		}
		autoPos();
	}
	else
	{
		setFlag(QGraphicsItem::ItemIsSelectable, true);
		setFlag(QGraphicsItem::ItemIsMovable, true);
		disconnect(m_parent_element, &Element::yChanged, this, &ElementTextItemGroup::autoPos);
		disconnect(m_parent_element, &Element::rotationChanged, this, &ElementTextItemGroup::autoPos);
		if(m_parent_element->linkType() == Element::Master)
		{
			disconnect(m_linked_changed_timer);
			if(m_XrefChanged_timer)
				disconnect(m_XrefChanged_timer);
		}
	}
	
	emit holdToBottomPageChanged(hold);
}

void ElementTextItemGroup::setFrame(const bool frame)
{
	m_frame = frame;
	update();
	emit frameChanged(m_frame);
}

bool ElementTextItemGroup::frame() const {
	return m_frame;
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
	
	dom_element.setAttribute("x", QString::number(pos().x()));
	dom_element.setAttribute("y", QString::number(pos().y()));

	QMetaEnum me = QMetaEnum::fromType<Qt::Alignment>();
	dom_element.setAttribute("alignment", me.valueToKey(m_alignment));
	
	dom_element.setAttribute("rotation", this->rotation());
	dom_element.setAttribute("vertical_adjustment", m_vertical_adjustment);
	dom_element.setAttribute("frame", m_frame? "true" : "false");
	
	dom_element.setAttribute("hold_to_bottom_page", m_hold_to_bottom_of_page == true ? "true" : "false");
	
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
	
	setName(dom_element.attribute("name", "no name"));
	QMetaEnum me = QMetaEnum::fromType<Qt::Alignment>();
	setAlignment(Qt::Alignment(me.keyToValue(dom_element.attribute("alignment").toStdString().data())));
	
	setPos(dom_element.attribute("x", QString::number(0)).toDouble(),
		   dom_element.attribute("y", QString::number(0)).toDouble());
	
	setRotation(dom_element.attribute("rotation", QString::number(0)).toDouble());
	setVerticalAdjustment(dom_element.attribute("vertical_adjustment").toInt());
	setFrame(dom_element.attribute("frame", "false") == "true"? true : false);
	
	QString hold = dom_element.attribute("hold_to_bottom_page", "false");
	setHoldToBottomPage(hold == "true" ? true : false);
	
	if(parentElement())
	{
		m_block_alignment_update = true;
		for(const QDomElement& text : QET::findInDomElement(dom_element, "texts", "text"))
		{
			DynamicElementTextItem *deti = nullptr;
			QUuid uuid(text.attribute("uuid"));
			
			for(DynamicElementTextItem *txt : parentElement()->dynamicTextItems())
				if(txt->uuid() == uuid)
					deti = txt;
			
			if (deti)
				parentElement()->addTextToGroup(deti, this);
		}
		m_block_alignment_update = false;
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
	if(m_frame)
	{		
		int font_size = 1;
		QRectF rect;
		for(DynamicElementTextItem *deti : this->texts())
		{
			font_size = std::max(font_size, deti->fontSize());
			rect = rect.united(mapFromItem(deti, deti->frameRect()).boundingRect());
		}
		
			//Adjust the thickness according to the font size
		qreal w=0.3;
		if (font_size >= 5)
		{
			w = (qreal)font_size*0.1;
			if(w > 2.5)
				w = 2.5;
		}
		
		painter->save();
		QPen pen;
		pen.setWidthF(w);
		painter->setPen(pen);
		painter->setRenderHint(QPainter::Antialiasing);
		
			//Adjust the rounding of the rectangle according to the size of the font
		qreal ro = (qreal)font_size/3;
		painter->drawRoundedRect(rect, ro, ro);
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
	for(QGraphicsItem *qgi : texts())
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

void ElementTextItemGroup::setPos(const QPointF &pos)
{
	QPointF old_pos = this->pos();
	QGraphicsItemGroup::setPos(pos);
	if (old_pos.x() != this->pos().x())
		emit xChanged();
	if (old_pos.y() != this->pos().y())
		emit yChanged();
}

void ElementTextItemGroup::setPos(qreal x, qreal y)
{
	QPointF old_pos = this->pos();
	QGraphicsItemGroup::setPos(x,y);
	if (old_pos.x() != this->pos().x())
		emit xChanged();
	if (old_pos.y() != this->pos().y())
		emit yChanged();
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
	if((event->buttons() & Qt::LeftButton) && (flags() & ItemIsMovable))
	{
		if(diagram() && m_first_move)
			diagram()->elementTextsMover().beginMovement(diagram(), this);
		
		if(m_first_move)
		{
			m_initial_position = pos();
			if(parentElement())
				parentElement()->setHighlighted(true);
		}
		
		QPointF current_parent_pos;
		QPointF button_down_parent_pos;
		current_parent_pos = mapToParent(mapFromScene(event->scenePos()));
		button_down_parent_pos = mapToParent(mapFromScene(event->buttonDownScenePos(Qt::LeftButton)));
		
		QPointF new_pos = m_initial_position + current_parent_pos - button_down_parent_pos;
		event->modifiers() == Qt::ControlModifier ? setPos(new_pos) : setPos(Diagram::snapToGrid(new_pos));
		
		if(diagram())
			diagram()->elementTextsMover().continueMovement(event);
	} else {
		event->ignore();
	}
	
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
		diagram()->elementTextsMover().endMovement();
		if(parentElement())
			parentElement()->setHighlighted(false);
	}
	
	if(!(event->modifiers() & Qt::ControlModifier))
		QGraphicsItemGroup::mouseReleaseEvent(event);
}

void ElementTextItemGroup::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if(m_slave_Xref_item)
	{
		if(m_slave_Xref_item->boundingRect().contains(mapToItem(m_slave_Xref_item, event->pos())))
		{
			if(parentElement()->linkType() == Element::Slave && !parentElement()->linkedElements().isEmpty())
			{
				m_slave_Xref_item->setDefaultTextColor(Qt::black);
				Element *elmt = parentElement()->linkedElements().first();
				
					//Unselect and ungrab mouse to prevent unwanted
					//move when linked element is in the same scene of this.
				setSelected(false);
				ungrabMouse();
				
				if(scene() != elmt->scene())
					elmt->diagram()->showMe();
				elmt->setSelected(true);
				
					//Zoom to the element
				for(QGraphicsView *view : elmt->scene()->views())
				{
					QRectF fit = elmt->sceneBoundingRect();
					fit.adjust(-200, -200, 200, 200);
					view->fitInView(fit, Qt::KeepAspectRatioByExpanding);
				}
			}
		}
	}
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

void ElementTextItemGroup::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
		//The pos of the event is not in this item coordinate,
		//but in child item hovered by the mouse, so we use the scene pos.
	if(m_slave_Xref_item &&
	   m_slave_Xref_item->boundingRect().contains(m_slave_Xref_item->mapFromScene(event->scenePos())))
		m_slave_Xref_item->setDefaultTextColor(Qt::blue);
	
	QGraphicsItemGroup::hoverEnterEvent(event);
}

void ElementTextItemGroup::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	if(m_slave_Xref_item)
		m_slave_Xref_item->setDefaultTextColor(Qt::black);
	
	QGraphicsItemGroup::hoverLeaveEvent(event);
}

void ElementTextItemGroup::updateXref()
{
	if(m_parent_element->diagram())
	{
		QETProject *project = m_parent_element->diagram()->project();
		
		if(m_parent_element->linkType() == Element::Master &&
		   !m_parent_element->linkedElements().isEmpty())
		{
			
			XRefProperties xrp = project->defaultXRefProperties(m_parent_element->kindInformations()["type"].toString());
			
			if(xrp.snapTo() == XRefProperties::Label)
			{
					//At least one text owned by this group must be set with
					//textFrom -> element info and element info name -> label
					//for display a xref
				for(DynamicElementTextItem *deti : texts())
				{
					if(deti->textFrom() == DynamicElementTextItem::ElementInfo &&
					   deti->infoName() == "label")
					{
						if(!m_Xref_item)
							m_Xref_item = new CrossRefItem(m_parent_element, this);
						m_Xref_item->autoPos();
						return;
					}
				}
			}
		}
		else if(m_parent_element->linkType() == Element::Slave &&
				!m_parent_element->linkedElements().isEmpty())
		{
			Element *master_elmt = m_parent_element->linkedElements().first();
			for(DynamicElementTextItem *deti : texts())
			{
				if((deti->textFrom() == DynamicElementTextItem::ElementInfo && deti->infoName() == "label") ||
				   (deti->textFrom() == DynamicElementTextItem::CompositeText && deti->compositeText().contains("%{label")))
				{
					XRefProperties xrp = project->defaultXRefProperties(master_elmt->kindInformations()["type"].toString());
					QString xref_label = xrp.slaveLabel();
					xref_label = autonum::AssignVariables::formulaToLabel(xref_label, master_elmt->rSequenceStruct(), master_elmt->diagram(), master_elmt);
					
					if(!m_slave_Xref_item)
					{
						m_slave_Xref_item = new QGraphicsTextItem(xref_label, this);
						m_slave_Xref_item->setFont(QETApp::diagramTextsFont(5));
						
						m_update_slave_Xref_connection << connect(master_elmt, &Element::xChanged,                       this, &ElementTextItemGroup::updateXref);
						m_update_slave_Xref_connection << connect(master_elmt, &Element::yChanged,                       this, &ElementTextItemGroup::updateXref);
						m_update_slave_Xref_connection << connect(master_elmt, &Element::elementInfoChange,              this, &ElementTextItemGroup::updateXref);
						m_update_slave_Xref_connection << connect(project,     &QETProject::projectDiagramsOrderChanged, this, &ElementTextItemGroup::updateXref);
						m_update_slave_Xref_connection << connect(project,     &QETProject::diagramRemoved,              this, &ElementTextItemGroup::updateXref);
						m_update_slave_Xref_connection << connect(project,     &QETProject::XRefPropertiesChanged,       this, &ElementTextItemGroup::updateXref);
					}
					else
						m_slave_Xref_item->setPlainText(xref_label);
					
					adjustSlaveXrefPos();
					return;
				}
			}
		
		}
	}

	
		//There is no reason to have a xref, we delete it if exist
	if(m_Xref_item)
	{
		delete m_Xref_item;
		m_Xref_item = nullptr;	
	}
	if(m_slave_Xref_item)
	{
		delete m_slave_Xref_item;
		m_slave_Xref_item = nullptr;
		m_update_slave_Xref_connection.clear();
	}
}

void ElementTextItemGroup::adjustSlaveXrefPos()
{
	QRectF r = boundingRect();
	QPointF pos(r.center().x() - m_slave_Xref_item->boundingRect().width()/2,
				r.bottom());
	m_slave_Xref_item->setPos(pos);
}

void ElementTextItemGroup::autoPos()
{
	int offset = 5;
	
	if(m_parent_element->linkType() == Element::Master)
	{
		if(!diagram())
			return;
		
		MasterElement *master = static_cast<MasterElement *>(m_parent_element);
		XRefProperties xrp = diagram()->project()->defaultXRefProperties(master->kindInformations()["type"].toString());
		if(xrp.snapTo() == XRefProperties::Bottom)
		{
			QRectF rectXref = master->XrefBoundingRect();
			offset = xrp.offset() <= 40 ? 5 : xrp.offset();
			
			offset += (int)rectXref.height();
		}
	}
	qreal r = rotation();
	centerToBottomDiagram(this, m_parent_element, offset);
		//centerToBottomDiagram change the rotation of this group if needed,
		//but setRotation is not a virtual function of QGraphicsItem, and the function centerToBottomDiagram
		//work with a QGraphicsItem. So we emit the signal if rotation changed
	if(rotation() != r)
		emit rotationChanged(rotation());
}

