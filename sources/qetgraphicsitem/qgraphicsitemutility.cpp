/*
	Copyright 2006-2021 The QElectroTech Team
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
#include "qgraphicsitemutility.h"

#include "../diagram.h"
#include "element.h"

#include <QDebug>
#include <QGraphicsItem>

/**
	@brief centerToParentBottom
	Center the item at the bottom of is parent.
	@param item item to center
	@return true if centered else false (item have not parent)
*/
bool centerToParentBottom(QGraphicsItem *item) {
	if (! item->parentItem()) {
		qDebug() << "Qet::centerToParentBottom : item have not parent";
		return false;
	}

	QPointF p = item -> parentItem() -> boundingRect().center();
	p.ry() +=   item -> parentItem() -> boundingRect().height()/2;
	p.rx() -=  (item -> boundingRect().width()/2 + item->boundingRect().left()); //< we add boundingrect.left because this value can be négative

	item -> setPos(p);
	return true;
}

/**
	@brief centerToBottomDiagram
	Set item pos to the bottom of diagram and centered vertically to element_to_follow, and add offset.
	@param item_to_center
	@param element_to_follow
	@param offset
	@return true if element is centered else false (element_to_follow have not diagram)
*/
#include "elementtextitemgroup.h"
#include "crossrefitem.h"
bool centerToBottomDiagram (QGraphicsItem *item_to_center, Element *element_to_follow, qreal offset) {
	if (! element_to_follow -> diagram()) {
		qDebug() << "qgraphicsitemutility centerAtBottomDiagram : Element_to_follow have not diagram";
		return false;
	}
	
	QRectF  border = element_to_follow -> diagram() -> border_and_titleblock.insideBorderRect();
	QPointF point  = element_to_follow -> sceneBoundingRect().center();
	
	point.setY(border.bottom() - item_to_center -> boundingRect().height() - offset );
	point.rx() -= (item_to_center -> boundingRect().width()/2);
	
		//Apply the difference between the pos() of item and its bounding rect
	QPointF tl = item_to_center->boundingRect().topLeft();
	point.rx() -= tl.x();
	point.ry() -= tl.y();
	
	item_to_center -> setPos(0,0);	  //Due to a weird behavior or bug, before setting the new position and rotation,
	item_to_center -> setRotation(0); //we must set the position and rotation to 0.
	
	item_to_center->setPos(item_to_center->mapFromScene(point));
	
	qreal rot = item_to_center->rotation();
	QGraphicsItem *parent = item_to_center->parentItem();
	while (parent) {
		rot += parent->rotation();
		parent = parent->parentItem();
	}
	if(rot != 0)
		item_to_center->setRotation(item_to_center->rotation() - rot);

	return true;
}
