/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "elementtextitem.h"
#include "qgraphicsitemutility.h"
#include <QPainter>

/**
 * @brief CommentItem::CommentItem
 * @param elmt : the element to display comment,
 * element is also the parent item of this item
 */
CommentItem::CommentItem(Element *elmt) :
	QGraphicsObject (elmt),
	m_element       (elmt),
	m_text_parent   (false)
{
	if (! setTextParent() ) {
		connect(elmt, SIGNAL(yChanged()),                        this, SLOT (autoPos()));
		connect(elmt, SIGNAL(rotationChanged()),                 this, SLOT (autoPos()));
	}
	connect(elmt, SIGNAL(elementInfoChange(DiagramContext,DiagramContext)), this, SLOT (updateLabel()));

	updateLabel();
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
	if (m_text_parent)
		centerToParentBottom(this);
	else
		centerToBottomDiagram(this, m_element);
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
 * @brief CommentItem::mouseDoubleClickEvent
 * @param event
 */
void CommentItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	event     -> accept();
	m_element -> editProperty();
}

/**
 * @brief CommentItem::setTextParent
 * Set text tagged "label" of element has parent.
 * @return true if text has been set has parent.
 * else return false, the element is the parent of this comment item
 */
bool CommentItem::setTextParent() {
	if (ElementTextItem *eti = m_element->taggedText("label")) {
		setParentItem(eti);
		m_text_parent = true;
		return true;
	}

	qDebug() << "Comment item: can't found text tagged 'label' from actual parent element to set has parent, "
				 "comment will be displayed at bottom of diagram";
	return false;
}

/**
 * @brief CommentItem::updateLabel
 * update the content of this item
 * (draw this item in a QPicture)
 */
void CommentItem::updateLabel() {
	QString comment = m_element -> elementInformations()["comment"].toString();

	if (comment == m_comment && !m_text_parent) return;

	if (comment != m_comment) {

		m_comment = comment;

		QPen pen(Qt::black);
			 pen.setWidthF (0.5);

		QPainter painter(&m_picture);
				 painter.setPen  (pen);
				 painter.setFont (QETApp::diagramTextsFont(6));

		QRectF drawing_rect(QPointF(0,0), QSizeF(70, 100));
		QRectF text_bounding;

		painter.drawText(drawing_rect, Qt::TextWordWrap | Qt::AlignHCenter, m_comment, &text_bounding);

		text_bounding.adjust(-1,0,1,0); //adjust only for better visual
		painter.drawRoundedRect(text_bounding, 2, 2);

		prepareGeometryChange();
		m_bounding_rect = text_bounding;
	}

	autoPos();
}
