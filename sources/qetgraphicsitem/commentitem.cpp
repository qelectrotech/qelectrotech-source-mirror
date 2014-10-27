/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "commentitem.h"
#include "element.h"
#include "qetapp.h"
#include "diagram.h"
#include <QPainter>

/**
 * @brief CommentItem::CommentItem
 * @param elmt : the element to display comment,
 * element is also the parent item of this item
 */
CommentItem::CommentItem(Element *elmt) :
	QGraphicsObject(elmt),
	m_element (elmt)
{
	updateLabel();
	connect(elmt, SIGNAL(yChanged()),                        this, SLOT (autoPos()));
	connect(elmt, SIGNAL(rotationChanged()),                 this, SLOT (autoPos()));
	connect(elmt, SIGNAL(elementInfoChange(DiagramContext)), this, SLOT (updateLabel()));
}

/**
 * @brief CommentItem::boundingRect
 * @return the bounding rect of this item
 */
QRectF CommentItem::boundingRect() const {
	return m_bounding_rect;
}

/**
 * @brief CommentItem::autoPos
 * Adjust the position of this item.
 */
void CommentItem::autoPos() {
	if(!m_element -> diagram()) return;

	QRectF  border = m_element -> diagram() -> border();
	QPointF point  = m_element -> sceneBoundingRect().center();

	point.setY(border.height() - m_element -> diagram() -> border_and_titleblock.titleBlockHeight() - boundingRect().height());
	point.rx() -= (m_bounding_rect.width()/2 + m_bounding_rect.left()); //< we add boundingrect.left because this value can be négative

	setPos(0,0);	//Due to a weird behavior or bug, before set the new position and rotation,
	setRotation(0); //we must to set the position and rotation at 0.
	setPos(mapFromScene(point));
	if (rotation() != - m_element -> rotation()) {
		setRotation(0);
		setRotation(- m_element -> rotation());
	}
}

/**
 * @brief CommentItem::paint
 * Paint this item
 * @param painter
 * @param option
 * @param widget
 */
void CommentItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option); Q_UNUSED(widget);
	m_picture.play(painter);
}

/**
 * @brief CommentItem::updateLabel
 * update the content of this item
 * (draw this item in a QPicture)
 */
void CommentItem::updateLabel() {
	QString comment = m_element -> elementInformations()["comment"].toString();
	bool    show    = m_element -> elementInformations().keyMustShow("comment");

	if (comment == m_comment && show == m_show) return;

	m_comment = comment;
	m_show    = show;

	QPen pen(Qt::black);
		 pen.setWidthF (0.5);

	QPainter painter(&m_picture);
			 painter.setPen  (pen);
			 painter.setFont (QETApp::diagramTextsFont(6));

	QRectF drawing_rect(QPointF(0,0), QSizeF(100, 100));
	QRectF text_bounding;

	painter.drawText(drawing_rect, Qt::TextWordWrap | Qt::AlignHCenter, m_comment, &text_bounding);

	text_bounding.adjust(-1,0,1,0); //adjust only for better visual
	painter.drawRoundedRect(text_bounding, 2, 2);

	m_bounding_rect = text_bounding;

	autoPos();
}
