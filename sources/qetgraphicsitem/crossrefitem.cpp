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
#include "elementtextitem.h"

//define the height of the header.
#define header 5
//define the minimal height of the cross (without header)
#define cross_min_heigth 33

/**
 * @brief CrossRefItem::CrossRefItem
 * Default constructor
 * @param elmt element to display the cross ref and also parent item.
 */
CrossRefItem::CrossRefItem(Element *elmt) :
	QGraphicsObject(elmt),
	m_element (elmt)
{
	m_properties = elmt->diagram()->defaultXRefProperties(elmt->kindInformations()["type"].toString());
	connect(elmt, SIGNAL(elementInfoChange(DiagramContext)), this, SLOT(updateLabel()));
	connect(elmt->diagram()->project(), SIGNAL(projectDiagramsOrderChanged(QETProject*,int,int)), this, SLOT(updateLabel()));
	connect(elmt->diagram(), SIGNAL(XRefPropertiesChanged()), this, SLOT(updateProperties()));

	//set specific behavior related to the parent item.
	if(m_properties.snapTo() == XRefProperties::Bottom) {
		connect(elmt, SIGNAL(positionChange(QPointF)), this, SLOT(autoPos()));
		connect(elmt, SIGNAL(rotationChanged()),	   this, SLOT(autoPos()));
	} else {
		setTextParent();
	}
	updateLabel();
}

/**
 * @brief CrossRefItem::~CrossRefItem
 * Default destructor
 */
CrossRefItem::~CrossRefItem() {
	if(m_properties.snapTo() == XRefProperties::Bottom) {
		disconnect(m_element, SIGNAL(positionChange(QPointF)), this, SLOT(autoPos()));
		disconnect(m_element, SIGNAL(rotationChanged()),	   this, SLOT(autoPos()));
	}
	disconnect(m_element, SIGNAL(elementInfoChange(DiagramContext)), this, SLOT(updateLabel()));
	disconnect(m_element->diagram()->project(), SIGNAL(projectDiagramsOrderChanged(QETProject*,int,int)), this, SLOT(updateLabel()));
	disconnect(m_element->diagram(), SIGNAL(XRefPropertiesChanged()), this, SLOT(updateProperties()));
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
		if      (elmt->kindInformations()["type"].toString() == "power")        txt.prepend(m_properties.prefix("power"));
		else if (elmt->kindInformations()["type"].toString().contains("delay")) txt.prepend(m_properties.prefix("delay"));
		else if (elmt->kindInformations()["state"].toString() == "SW")          txt.prepend(m_properties.prefix("switch"));
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

		//NO and NC are displayed in single place in the cross
		if (state == "NO" || state == "NC") {
			if (state == "NO") tmp_str = &no_str;
			else if (state == "NC") tmp_str = &nc_str;

			if (!tmp_str->isEmpty()) *tmp_str += "\n";
			*tmp_str += elementPositionText(elmt, add_prefix);
		}

		//SW are displayed in NC and NO column in the cross
		else if (state == "SW") {
			for (int i = 0; i < 2; i++) {
				tmp_str = i==0? &no_str : &nc_str;
				if (!tmp_str->isEmpty()) *tmp_str += "\n";
				*tmp_str += elementPositionText(elmt, add_prefix);
			}
		}
	}
}

/**
 * @brief CrossRefItem::updateProperties
 * update the curent properties
 */
void CrossRefItem::updateProperties() {
	XRefProperties xrp = m_element->diagram()->defaultXRefProperties(m_element->kindInformations()["type"].toString());

	if (m_properties != xrp) {
		if (m_properties.snapTo() != xrp.snapTo()) {
			if (xrp.snapTo() == XRefProperties::Bottom) {
				setParentItem(m_element);
				connect(m_element, SIGNAL(positionChange(QPointF)), this, SLOT(autoPos()));
				connect(m_element, SIGNAL(rotationChanged()),	   this, SLOT(autoPos()));
			} else {
				setTextParent();
				disconnect(m_element, SIGNAL(positionChange(QPointF)), this, SLOT(autoPos()));
				disconnect(m_element, SIGNAL(rotationChanged()),	   this, SLOT(autoPos()));
			}
		}
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
	//We calcul the position according to the @snapTo of the xrefproperties
	if (m_properties.snapTo() == XRefProperties::Bottom) {
		QRectF border = m_element->diagram()->border();
		QPointF point = m_element->sceneBoundingRect().center();

		point.setY(border.height() - m_element->diagram()->border_and_titleblock.titleBlockHeight() - boundingRect().height());
		point.rx() -= (m_bounding_rect.width()/2 + m_bounding_rect.left()); //< we add boundingrect.left because this value can be négative

		setPos(0,0);	//Due to a weird behavior or bug, before set the new position and rotation,
		setRotation(0); //we must to set the position and rotation at 0.
		setPos(mapFromScene(point));
		if (rotation() != - m_element->rotation()) {
			setRotation(0);
			setRotation(- m_element->rotation());
		}
	}
	else {
		QPointF p = parentItem()->boundingRect().center();
		p.ry() += parentItem()->boundingRect().height()/2;
		p.rx() -= (m_bounding_rect.width()/2 + m_bounding_rect.left()); //< we add boundingrect.left because this value can be négative
		setPos(p);
	}
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
	m_drawing.play(painter);
}

/**
 * @brief CrossRefItem::mouseDoubleClickEvent
 * @param event
 */
void CrossRefItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	event->accept();
	m_element->editProperty();
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

			QString state = info["state"].toString();
				 if (state == "NO") option = NO;
			else if (state == "NC") option = NC;
			else if (state == "SW") option = SW;

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
 * Draw one contact, the type of contact to draw is define in @flags.
 * @param painter, painter to use
 * @param flags, define how to draw the contact (see enul CONTACTS)
 * @param str, the text to display for this contact (the position of the contact).
 */
void CrossRefItem::drawContact(QPainter &painter, int flags, QString str) {
	int offset = m_drawed_contacts*10;

	//Draw NO or NC contact
	if (flags &NOC) {
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
		if(flags &Delay) {
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

	//Draw a switch contact
	else if (flags &SW) {
		//draw the NO side
		painter.drawLine(0, offset+6, 8, offset+6);
		//Draw the NC side
		QPointF p1[3] = {
			QPointF(0, offset+16),
			QPointF(8, offset+16),
			QPointF(8, offset+12)
		};
		painter.drawPolyline(p1, 3);
		//Draw the common side
		QPointF p2[3] = {
			QPointF(7, offset+14),
			QPointF(16, offset+11),
			QPointF(24, offset+11),
		};
		painter.drawPolyline(p2, 3);
		//Draw position text
		painter.drawText(20, offset+5, 30, 10, Qt::AlignRight | Qt::AlignVCenter, str);
		//a switch contact take place of two normal contact
		m_drawed_contacts += 2;
	}
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

/**
 * @brief CrossRefItem::setTextParent
 * Set the text field tagged "label" of m_element
 * parent of this item
 */
void CrossRefItem::setTextParent() {
	ElementTextItem *eti = m_element->taggedText("label");
	if (eti) setParentItem(eti);
	else qDebug() << "CrossRefItem,no texte tagged 'label' found to set has parent";
}

