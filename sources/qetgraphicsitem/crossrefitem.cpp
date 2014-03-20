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
#include "crossrefitem.h"
#include "element.h"
#include "qetapp.h"
#include "diagramposition.h"

/**
 * @brief CrossRefItem::CrossRefItem
 * Default constructor
 * @param elmt element to dispaly the cross ref
 * @param parent parent QetGraphicsItem
 */
CrossRefItem::CrossRefItem(Element *elmt, QetGraphicsItem *parent) :
	QetGraphicsItem(parent),
	element_ (elmt)
{
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable);
	connect(elmt, SIGNAL(positionChange(QPointF)), this, SLOT(autoPos()));
	connect(diagram()->project(), SIGNAL(projectDiagramsOrderChanged(QETProject*,int,int)), this, SLOT(updateLabel()));
	updateLabel();
}

/**
 * @brief CrossRefItem::~CrossRefItem
 * Default destructor
 */
CrossRefItem::~CrossRefItem() {
	disconnect(element_, SIGNAL(positionChange(QPointF)), this, SLOT(autoPos()));
	disconnect(diagram()->project(), SIGNAL(projectDiagramsOrderChanged(QETProject*,int,int)), this, SLOT(updateLabel()));
}

/**
 * @brief CrossRefItem::updateLabel
 * Update the content of the item
 */
void CrossRefItem::updateLabel() {
	//init the painter
	QPainter qp(&drawing_);
	QPen pen_;
	pen_.setWidthF(0.2);
	pen_.setCosmetic(true);
	qp.setPen(pen_);

	//calcul the size
	setUpBoundingRect();

	//draw the cross
	qp.drawLine(30, 0, 30, boundingRect().height()); //vertical line
	qp.drawLine(0,	5, boundingRect().width(), 5);  //horizontal line

	//draw the symbolic NO
	qp.drawLine(11, 2, 14, 2);
	static const QPointF p1[3] = {
		QPointF(14, 1),
		QPointF(16, 2),
		QPointF(18, 2),
	};
	qp.drawPolyline(p1,3);

	//draw the symbolic NC
	static const QPointF p2[3] = {
		QPointF(36, 2),
		QPointF(39, 2),
		QPointF(39, 0),
	};
	qp.drawPolyline(p2,3);
	static const QPointF p3[3] = {
		QPointF(38, 0),
		QPointF(41, 2),
		QPointF(43, 2),
	};
	qp.drawPolyline(p3,3);

	///keep this code for possible next feature
	///choice to use symbolic or text.
	//draw the header
	/*qp.setFont(QETApp::diagramTextsFont(7));
	QRectF header_rect (0,0,30,10);
	qp.drawText(header_rect, Qt::AlignCenter, "NO");
	header_rect.setRect(30, 0, 30, 10);
	qp.drawText(header_rect, Qt::AlignCenter, "NC");*/

	//and fill it
	fillCrossRef(&qp);

	autoPos();
	update();
}

/**
 * @brief CrossRefItem::autoPos
 * Calculate and set position automaticaly.
 */
void CrossRefItem::autoPos() {
	if (isSelected() && element_->isSelected()) return;
	QRectF border= element_->diagram()->border();
	QPointF point;

	//if this item have parent calcule
	//te position by using mapped point.
	if(parentItem()) {
		point = element_->boundingRect().center();
		QPointF ypoint_ = mapToParent(mapFromScene(0, border.height() - element_->diagram()->border_and_titleblock.titleBlockHeight() - boundingRect().height()));
		point.setY(ypoint_.y());
	}
	else {
		point = element_->sceneBoundingRect().center();
		point.setY(border.height() - element_->diagram()->border_and_titleblock.titleBlockHeight() - boundingRect().height());
	}

	point.setX(point.x() - boundingRect().width()/2);
	setPos(point);
}

/**
 * @brief CrossRefItem::boundingRect
 * @return the bounding rect of this item
 */
QRectF CrossRefItem::boundingRect() const {
	return bounding_rect_;
}

/**
 * @brief CrossRefItem::paint
 * Paint this item
 * @param painter
 * @param option
 * @param widget
 */
void CrossRefItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option);
	Q_UNUSED(widget);

	//draw the selection rect
	if (isSelected()) {
		painter->save();
		QPen t(Qt::black);
		t.setStyle(Qt::DashLine);
		t.setCosmetic(true);
		painter -> setPen(t);
		painter -> setRenderHint(QPainter::Antialiasing, false);
		painter->drawRect(boundingRect());
		painter->restore();
	}
	drawing_.play(painter);
}

/**
 * @brief CrossRefItem::mouseMoveEvent
 * handle mouse move event
 * @param e event
 */
void CrossRefItem::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	element_->setHighlighted(true);
	QetGraphicsItem::mouseMoveEvent(e);
}

/**
 * @brief CrossRefItem::mouseReleaseEvent
 * handle mouse release event
 * @param e event
 */
void CrossRefItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	element_->setHighlighted(false);
	QetGraphicsItem::mouseReleaseEvent(e);
}

/**
 * @brief CrossRefItem::setUpBoundingRect
 * Get the numbers of slaves elements linked to this parent element,
 * for calculate the size of the bounding rect.
 * The cross ref item is drawing according to the size of the bounding rect.
 */
void CrossRefItem::setUpBoundingRect() {
	//this is the default size of cross ref item
	QRectF default_bounding(0, 0, 60, 50);

	//No need to calcul if nothing is linked
	if (!element_->isFree()) {
		QList <Element *> NO_list;
		QList <Element *> NC_list;

		//find each no and nc of connected element to element_
		foreach (Element *elmt, element_->linkedElements()) {
			QString state = elmt->kindInformations()["state"].toString();
			if (state == "NO")		NO_list << elmt;
			else if (state == "NC") NC_list << elmt;
		}

		int i =0;
		if (NO_list.count()>4 || NC_list.count()>4) {
			i = NO_list.count() > NC_list.count()?
						NO_list.count() : NC_list.count();

			//increase the height of bounding rect,
			//according to the number of slave item less 4.
			i-=4;
			default_bounding.setHeight(default_bounding.height() + (i*9));
		}
	}

	bounding_rect_ = default_bounding;
}

/**
 * @brief CrossRefItem::fillCrossRef
 * Fill the content of the cross ref
 * @param painter painter to use.
 */
void CrossRefItem::fillCrossRef(QPainter *painter) {
	if (element_->isFree()) return;

	QList <Element *> NO_list;
	QList <Element *> NC_list;

	//find each no and nc of connected element to element_
	foreach (Element *elmt, element_->linkedElements()) {
		QString state = elmt->kindInformations()["state"].toString();
		if (state == "NO")		NO_list << elmt;
		else if (state == "NC") NC_list << elmt;
	}

	painter -> setFont(QETApp::diagramTextsFont(4));
	painter -> setRenderHint(QPainter::Antialiasing, false);
	//fill the NO
	QString contact_str;
	foreach (Element *elmt, NO_list) {
		contact_str += QString::number(elmt->diagram()->folioIndex() + 1);
		contact_str += "-";
		contact_str += elmt->diagram()->convertPosition(elmt -> scenePos()).toString();
		contact_str += "\n";
	}
	QRectF rect_(0, 10, 26, (boundingRect().height()-10));
	painter->drawText(rect_, Qt::AlignHCenter, contact_str);

	//fill the NC
	contact_str.clear();
	foreach (Element *elmt, NC_list) {
		contact_str += QString::number(elmt->diagram()->folioIndex() + 1);
		contact_str += "-";
		contact_str += elmt->diagram()->convertPosition(elmt -> scenePos()).toString();
		contact_str += "\n";
	}
	rect_.setRect(28, 10, 28, (boundingRect().height()-10));
	painter->drawText(rect_, Qt::AlignHCenter, contact_str);
}
