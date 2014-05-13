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

//define the height of the header.
#define header 5
//define the minimal height of the cross (without header)
#define cross_min_heigth 33

/**
 * @brief CrossRefItem::CrossRefItem
 * Default constructor
 * @param elmt element to dispaly the cross ref
 * @param parent parent QetGraphicsItem
 */
CrossRefItem::CrossRefItem(Element *elmt, QGraphicsItem *parent) :
	QGraphicsObject(parent),
	m_element (elmt)
{
	m_properties = elmt->diagram()->defaultXRefProperties();
	setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
	connect(elmt, SIGNAL(positionChange(QPointF)), this, SLOT(autoPos()));
	connect(elmt, SIGNAL(elementInfoChange(DiagramContext)), this, SLOT(updateLabel()));
	connect(elmt->diagram()->project(), SIGNAL(projectDiagramsOrderChanged(QETProject*,int,int)), this, SLOT(updateLabel()));
	connect(elmt->diagram(), SIGNAL(XRefPropertiesChanged(XRefProperties)), this, SLOT(setProperties(XRefProperties)));
	updateLabel();
}

/**
 * @brief CrossRefItem::~CrossRefItem
 * Default destructor
 */
CrossRefItem::~CrossRefItem() {
	disconnect(m_element, SIGNAL(positionChange(QPointF)), this, SLOT(autoPos()));
	disconnect(m_element, SIGNAL(elementInfoChange(DiagramContext)), this, SLOT(updateLabel()));
	disconnect(m_element->diagram()->project(), SIGNAL(projectDiagramsOrderChanged(QETProject*,int,int)), this, SLOT(updateLabel()));
	disconnect(m_element->diagram(), SIGNAL(XRefPropertiesChanged(XRefProperties)), this, SLOT(setProperties(XRefProperties)));
}

/**
 * @brief CrossRefItem::boundingRect
 * @return the bounding rect of this item
 */
QRectF CrossRefItem::boundingRect() const {
	return m_bounding_rect;
}

/**
 * @brief CrossRefItem::shape
 * @return the shape of this item
 */
QPainterPath CrossRefItem::shape() const{
	return m_shape_path;
}

/**
 * @brief CrossRefItem::elementPositionText
 * @param elmt
 * @return the string corresponding to the position of @elmt in the diagram.
 * if @add_prefix is true, prefix (for power and delay contact) is added to the poistion text.
 */
QString CrossRefItem::elementPositionText(const Element *elmt, const bool &add_prefix) const{
	QString txt;
	txt += QString::number(elmt->diagram()->folioIndex() + 1);
	txt += "-";
	txt += elmt->diagram()->convertPosition(elmt -> scenePos()).toString();
	if (add_prefix) {
		if (elmt->kindInformations()["type"].toString() == "power") txt.prepend(m_properties.prefix("power"));
		else if (elmt->kindInformations()["type"].toString().contains("delay")) txt.prepend(m_properties.prefix("delay"));
	}
	return txt;
}

/**
 * @brief CrossRefItem::allElementsPositionText
 * Return the text of all elements linked to @m_element, in several QString
 * according to the type of linked elements. Each text of elements are separate by "\n"
 * @param no_str the string of NO contacts
 * @param nc_str the string of NC contacts
 * @param add_prefix must add prefix to text (true) or not (false);
 */
void CrossRefItem::allElementsPositionText(QString &no_str, QString &nc_str, const bool &add_prefix) const {
	QString *tmp_str;
	foreach (Element *elmt, m_element->linkedElements()) {
		QString state = elmt->kindInformations()["state"].toString();

		if (state == "NO") tmp_str = &no_str;
		else if (state == "NC") tmp_str = &nc_str;

		if (!tmp_str->isEmpty()) *tmp_str += "\n";
		*tmp_str += elementPositionText(elmt, add_prefix);
	}
}

void CrossRefItem::setProperties(const XRefProperties &xrp) {
	if (m_properties != xrp) {
		m_properties = xrp;
		updateLabel();
	}
}

/**
 * @brief CrossRefItem::updateLabel
 * Update the content of the item
 */
void CrossRefItem::updateLabel() {
	//init the shape
	m_shape_path= QPainterPath();
	//init the painter
	QPainter qp;
	qp.begin(&m_drawing);
	QPen pen_;
	pen_.setWidthF(0.2);
	qp.setPen(pen_);
	qp.setFont(QETApp::diagramTextsFont(5));

	XRefProperties::DisplayHas dh = m_properties.displayHas();
	if (dh == XRefProperties::Cross) {
		drawHasCross(qp);
	}
	else if (dh == XRefProperties::Contacts) {
		drawHasContacts(qp);
	}

	AddExtraInfo(qp);
	qp.end();

	autoPos();
	update();
	checkMustShow();
}

/**
 * @brief CrossRefItem::autoPos
 * Calculate and set position automaticaly.
 */
void CrossRefItem::autoPos() {
	if (isSelected() && m_element->isSelected()) return;
	QRectF border= m_element->diagram()->border();
	QPointF point;

	//if this item have parent calcule the position by using mapped point.
	if(parentItem()) {
		point = m_element->boundingRect().center();
		QPointF ypoint_ = mapToParent(mapFromScene(0, border.height() - m_element->diagram()->border_and_titleblock.titleBlockHeight() - boundingRect().height()));
		point.setY(ypoint_.y());
	}
	else {
		point = m_element->sceneBoundingRect().center();
		point.setY(border.height() - m_element->diagram()->border_and_titleblock.titleBlockHeight() - boundingRect().height());
	}

	qreal offset = m_bounding_rect.topLeft().x() < 0 ? m_bounding_rect.topLeft().x() : 0;
	point.setX(point.x() - m_bounding_rect.width()/2 - offset);
	setPos(point);
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
		painter -> drawPath(m_shape_path);
		painter -> restore();
	}
	m_drawing.play(painter);
}

/**
 * @brief CrossRefItem::mouseMoveEvent
 * handle mouse move event
 * @param e event
 */
void CrossRefItem::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	m_element->setHighlighted(true);
	QGraphicsObject::mouseMoveEvent(e);
}

/**
 * @brief CrossRefItem::mouseReleaseEvent
 * handle mouse release event
 * @param e event
 */
void CrossRefItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	m_element->setHighlighted(false);
	QGraphicsObject::mouseReleaseEvent(e);
}

/**
 * @brief CrossRefItem::buildHeaderContact
 * Draw the QPicture of m_hdr_no_ctc and m_hdr_nc_ctc
 */
void CrossRefItem::buildHeaderContact() {
	if (!m_hdr_no_ctc.isNull() && !m_hdr_nc_ctc.isNull()) return;

	//init the painter
	QPainter qp;
	QPen pen_;
	pen_.setWidthF(0.2);

	//draw the NO contact
	if (m_hdr_no_ctc.isNull()) {
		qp.begin(&m_hdr_no_ctc);
		qp.setPen(pen_);
		qp.drawLine(0, 3, 5, 3);
		QPointF p1[3] = {
			QPointF(5, 0),
			QPointF(10, 3),
			QPointF(15, 3),
		};
		qp.drawPolyline(p1,3);
		qp.end();
	}

	//draw the NC contact
	if (m_hdr_nc_ctc.isNull()) {
		qp.begin(&m_hdr_nc_ctc);
		qp.setPen(pen_);
		QPointF p2[3] = {
			QPointF(0, 3),
			QPointF(5, 3),
			QPointF(5, 0)
		};
		qp.drawPolyline(p2,3);
		QPointF p3[3] = {
			QPointF(4, 0),
			QPointF(10, 3),
			QPointF(15, 3),
		};
		qp.drawPolyline(p3,3);
		qp.end();
	}
}

/**
 * @brief CrossRefItem::setUpCrossBoundingRect
 * Get the numbers of slaves elements linked to this parent element,
 * for calculate the size of the cross bounding rect.
 * The cross ref item is drawing according to the size of the cross bounding rect.
 */
void CrossRefItem::setUpCrossBoundingRect(QPainter &painter) {
	//this is the default size of cross ref item
	QRectF default_bounding(0, 0, 40, header + cross_min_heigth);

	//No need to calcul if nothing is linked
	if (!m_element->isFree()) {

		QString no_str, nc_str;
		allElementsPositionText(no_str, nc_str, true);

		//Adjust the size of default_bounding if needed.
		//We calcule the size by using a single text
		//because in the method fillCrossRef, the text is draw like this (aka single text)

		//Adjust according to the NO
		QRectF bounding = painter.boundingRect(QRectF (), Qt::AlignCenter, no_str);
		if (bounding.height() > default_bounding.height() - header)
			default_bounding.setHeight(bounding.height() + header); //adjust the height
		if (bounding.width() > default_bounding.width()/2)
			default_bounding.setWidth(bounding.width()*2);			//adjust the width

		//Adjust according to the NC
		bounding = painter.boundingRect(QRectF (), Qt::AlignCenter, nc_str);
		if (bounding.height() > default_bounding.height() - header)
			default_bounding.setHeight(bounding.height() + header); //adjust the heigth
		if (bounding.width() > default_bounding.width()/2)
			default_bounding.setWidth(bounding.width()*2);			//adjust the width
	}

	m_shape_path.addRect(default_bounding);
	m_bounding_rect = default_bounding;
}

/**
 * @brief CrossRefItem::drawHasCross
 * Draw this crossref with a cross
 * @param painter, painter to use
 */
void CrossRefItem::drawHasCross(QPainter &painter) {
	//calcul the size of the cross
	setUpCrossBoundingRect(painter);

	//draw the cross
	QRectF br = boundingRect();
	painter.drawLine(br.width()/2, 0, br.width()/2, br.height());	//vertical line
	painter.drawLine(0, header, br.width(), header);	//horizontal line

	//Add the symbolic contacts
	buildHeaderContact();
	QPointF p((m_bounding_rect.width()/4) - (m_hdr_no_ctc.width()/2), 0);
	painter.drawPicture (p, m_hdr_no_ctc);
	p.setX((m_bounding_rect.width() * 3/4) - (m_hdr_nc_ctc.width()/2));
	painter.drawPicture (p, m_hdr_nc_ctc);

	//and fill it
	fillCrossRef(painter);
}

/**
 * @brief CrossRefItem::drawHasContacts
 * Draw this crossref with symbolic contacts
 * @param painter painter to use
 */
void CrossRefItem::drawHasContacts(QPainter &painter) {
	m_drawed_contacts = 0;

	//Draw each linked contact
	foreach (Element *elmt,  m_element->linkedElements()) {
		DiagramContext info = elmt->kindInformations();

		for (int i=0; i<info["number"].toInt(); i++) {
			int option = 0;

			info["state"].toString() == "NO"? option = NO : option = NC;

			QString type = info["type"].toString();
			if (type == "power") option += Power;
			else if (type == "delayOn") option += DelayOn;
			else if (type == "delayOff") option += DelayOff;

			drawContact(painter, option, elementPositionText(elmt));
		}
	}

	QRectF br(0, 0, 50, m_drawed_contacts*10+4);
	m_bounding_rect = br;
	m_shape_path.addRect(br);
}

/**
 * @brief CrossRefItem::drawContact
 * draw one contacte, the type of contact to draw is define in ctc.
 * @param painter painter to use
 * @param ctc option for draw the contact, see enum CONTACTS
 */
void CrossRefItem::drawContact(QPainter &painter, int flags, QString str) {
	int offset = m_drawed_contacts*10;

	//draw the basic line
	painter.drawLine(0, offset+6, 8, offset+6);
	painter.drawLine(16, offset+6, 24, offset+6);

	///take exemple of this code for display the terminal text
	/*QFont font = QETApp::diagramTextsFont(4);
	font.setBold(true);
	painter.setFont(font);
	QRectF bt(0, offset, 24, 10);
	int txt = 10 + m_drawed_contacts;
	painter.drawText(bt, Qt::AlignLeft|Qt::AlignTop, QString::number(txt));
	painter.drawText(bt, Qt::AlignRight|Qt::AlignTop, QString::number(txt));
	painter.setFont(QETApp::diagramTextsFont(5));*/

	//draw open contact
	if (flags &NO) {
		painter.drawLine(8, offset+9, 16, offset+6);
	}
	//draw close contact
	if (flags &NC) {
		QPointF p1[3] = {
			QPointF(8, offset+6),
			QPointF(9, offset+6),
			QPointF(9, offset+2.5)
		};
		painter.drawPolyline(p1,3);
		painter.drawLine(8, offset+3, 16, offset+6);
	}

	//draw half circle for power contact
	if (flags &Power) {
		QRectF arc(4, offset+4, 4, 4);
		if (flags &NO)
			painter.drawArc(arc, 180*16, 180*16);
		else
			painter.drawArc(arc, 0, 180*16);
	}

	// draw half circle for delay contact
	if(flags &DelayOn || flags &DelayOff) {
		// for delay on contact
		if (flags &DelayOn) {
			if (flags &NO) {
				painter.drawLine(12, offset+4, 12, offset+6);
				QRectF r(9.5, offset+1, 5, 3);
				painter.drawArc(r, 180*16, 180*16);
			}
			if (flags &NC) {
				painter.drawLine(QPointF(13.5, offset+2), QPointF(13.5, offset+3.5));
				QRectF r(11, offset-1, 5, 3);
				painter.drawArc(r, 180*16, 180*16);
			}
		}
		// for delay off contact
		else {
			if (flags &NO) {
				painter.drawLine(12, offset+3, 12, offset+6);
				QRectF r(9.5, offset+2, 5, 3);
				painter.drawArc(r, 0, 180*16);
			}
			if (flags &NC) {
				painter.drawLine(QPointF(13.5, offset+1), QPointF(13.5, offset+3.5));
				QRectF r(11, offset, 5, 3);
				painter.drawArc(r, 0, 180*16);
			}
		}
	}

	painter.drawText(20, offset, 30, 10, Qt::AlignRight | Qt::AlignVCenter, str);
	++m_drawed_contacts;
}

/**
 * @brief CrossRefItem::fillCrossRef
 * Fill the content of the cross ref
 * @param painter painter to use.
 */
void CrossRefItem::fillCrossRef(QPainter &painter) {
	if (m_element->isFree()) return;

	QString no_str, nc_str;
	allElementsPositionText(no_str, nc_str, true);

	qreal middle_cross = m_bounding_rect.width()/2;

	QRectF rect_(0, header, middle_cross, boundingRect().height()-header);
	painter.drawText(rect_, Qt::AlignTop | Qt::AlignLeft, no_str);

	rect_.moveTopLeft(QPointF (middle_cross, header));
	painter.drawText(rect_, Qt::AlignTop | Qt::AlignRight, nc_str);


}

/**
 * @brief CrossRefItem::AddExtraInfo
 * Add the comment info of the parent item if needed.
 * @param painter painter to use for draw the text
 */
void CrossRefItem::AddExtraInfo(QPainter &painter) {
	QString comment = m_element-> elementInformations()["comment"].toString();
	bool must_show = m_element-> elementInformations().keyMustShow("comment");
	if (!comment.isEmpty() && must_show) {
		painter.save();
		painter.setFont(QETApp::diagramTextsFont(6));

		QRectF r, text_bounding;
		qreal center = boundingRect().center().x();
		r = QRectF(QPointF(center - 50, boundingRect().bottom()),
				   QPointF(center + 50, boundingRect().bottom() + 50));
		painter.drawText(r, Qt::TextWordWrap | Qt::AlignHCenter, comment, &text_bounding);

		text_bounding.adjust(-1,0,1,0); //adjust only for better visual

		m_shape_path.addRect(text_bounding);
		m_bounding_rect = m_bounding_rect.united(text_bounding);
		painter.drawRoundedRect(text_bounding, 2, 2);
		painter.restore();
	}
}

/**
 * @brief CrossRefItem::checkMustShow
 * Check the propertie of this Xref for know if we
 * must to be show or not
 */
void CrossRefItem::checkMustShow() {
	//We always show Xref when is displayed has contact
	if (m_properties.displayHas() == XRefProperties::Contacts) {
		this->show();
		return;
	}

	//if Xref is display has cross and we must to don't show power contact, check it
	else if (m_properties.displayHas() == XRefProperties::Cross && !m_properties.showPowerContact()) {
		bool power = false;
		foreach (Element *elmt, m_element->linkedElements()) {
			// contact checked isn't power, show this xref and return;
			if (elmt->kindInformations()["type"].toString() != "power") {
				this->show();
				return;
			} else {
				power = true;
			}
		}
		if (power) {
			this->hide();
			return;
		}
	}

	//By default, show this Xref
	else {
		this->show();
		return;
	}
}

