/*
	Copyright 2006-2016 The QElectroTech Team
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
	else {
		XRefProperties xrp = m_element->diagram()->defaultXRefProperties(m_element->kindInformations()["type"].toString());
		centerToBottomDiagram(this, m_element, xrp.offset());
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
void CommentItem::updateLabel()
{
	QString comment  = m_element -> elementInformations()["comment"]. toString();

	QString location = m_element -> elementInformations()["location"].toString();

	prepareGeometryChange();
	m_bounding_rect = QRectF();

	QPainter painter_;
	painter_.begin(&m_picture);

	if (comment == m_comment && !m_text_parent && location == m_location)
		return;

	if (comment != m_comment)
	{
		m_comment = comment;
		addInfo(painter_, "comment");
	}
	else if (comment == m_comment)
		addInfo(painter_, "comment");

	if (location != m_location)
	{
		m_location = location;
		addInfo(painter_, "location");
	}
	else if (location == m_location){
		addInfo(painter_, "location");
	}
	painter_.end();

	autoPos();
}

/**
 * @brief CommentItem::addInfo
 * @param painter
 * @param type e.g. comment, location
 * Draw Info to item text.
 * (draw this item in a QPicture)
 */
void CommentItem::addInfo(QPainter &painter, QString type)
{
	QString text = autonum::AssignVariables::formulaToLabel(m_element -> elementInformations()[type].toString(), m_element->rSequenceStruct(), m_element->diagram(), m_element);
	bool must_show  = m_element -> elementInformations().keyMustShow(type);

	if (!text.isEmpty() && must_show)
	{
		painter.save();
		painter.setFont (QETApp::diagramTextsFont(6));

		QPen pen(Qt::black);
		pen.setWidthF (0.5);

		painter.setPen  (pen);

		QRectF r, text_bounding;
		qreal center = boundingRect().center().x();

		r = QRectF(QPointF(center - 35, boundingRect().bottom()),
				   QPointF(center + 35, boundingRect().bottom() + 1));

		text_bounding = painter.boundingRect(r, Qt::TextWordWrap | Qt::AlignHCenter, text);
		painter.drawText(text_bounding, Qt::TextWordWrap | Qt::AlignHCenter, text);

		text_bounding.adjust(-1,0,1,0); //adjust only for better visual

		m_shape_path.addRect(text_bounding);
		prepareGeometryChange();
		m_bounding_rect = m_bounding_rect.united(text_bounding);
		if (type == "comment") painter.drawRoundedRect(text_bounding, 2, 2);
		painter.restore();
	}
}
