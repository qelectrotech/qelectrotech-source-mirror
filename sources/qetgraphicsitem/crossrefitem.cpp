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
#include "crossrefitem.h"

#include "../autoNum/assignvariables.h"
#include "../diagram.h"
#include "../diagramposition.h"
#include "../qetapp.h"
#include "dynamicelementtextitem.h"
#include "element.h"
#include "elementtextitemgroup.h"
#include "qgraphicsitemutility.h"

//define the height of the header.
static int header = 5;
//define the minimal height of the cross (without header)
static int cross_min_height = 33;

/**
	@brief CrossRefItem::CrossRefItem
	@param elmt : element to display the cross ref
*/
CrossRefItem::CrossRefItem(Element *elmt) :
	QGraphicsObject(elmt),
	m_element(elmt)
{init();}

/**
	@brief CrossRefItem::CrossRefItem
	@param elmt : element to display the cross ref
	@param text : If the Xref must be displayed under a text, the text.
*/
CrossRefItem::CrossRefItem(Element *elmt, DynamicElementTextItem *text) :
	QGraphicsObject(text),
	m_element (elmt),
	m_text(text)
{init();}

/**
	@brief CrossRefItem::CrossRefItem
	@param elmt : element to display the cross ref
	@param group : If the Xref must be displayed under a group, the group.
*/
CrossRefItem::CrossRefItem(Element *elmt, ElementTextItemGroup *group) :
	QGraphicsObject(group),
	m_element(elmt),
	m_group(group)
{init();}

/**
	@brief CrossRefItem::~CrossRefItem
	Default destructor
*/
CrossRefItem::~CrossRefItem()
{}

/**
	@brief CrossRefItem::init
	init this Xref
*/
void CrossRefItem::init()
{
	if(!m_element->diagram())
	{
		qDebug() << "CrossRefItem constructor" << "element is not in a diagram";
		return;
	}
	
	QETProject *project = m_element->diagram()->project();
	connect(project, &QETProject::XRefPropertiesChanged, this, &CrossRefItem::updateProperties);
	
	m_properties = m_element->diagram()->project()->defaultXRefProperties(m_element->kindInformations()["type"].toString());
	setAcceptHoverEvents(true);
	
	setUpConnection();
	linkedChanged();
	updateLabel();
}

/**
	@brief CrossRefItem::setUpConnection
	Set up several connection to keep up to date the Xref
*/
void CrossRefItem::setUpConnection()
{
	for(const QMetaObject::Connection& c : m_update_connection)
		disconnect(c);
	
	m_update_connection.clear();
	QETProject *project = m_element->diagram()->project();
	bool set=false;
		
	if(m_properties.snapTo() == XRefProperties::Label && (m_text || m_group)) //Snap to label and parent is a text or a group
		set=true;
	else if(m_properties.snapTo() == XRefProperties::Bottom && !m_text && !m_group) //Snap to bottom of element and parent is the element itself
	{
		m_update_connection << connect(m_element, SIGNAL(yChanged()),
					       this, SLOT(autoPos()));
		m_update_connection << connect(m_element, SIGNAL(rotationChanged()),
					       this, SLOT(autoPos()));
		set=true;
	}

	if(set)
	{
		m_update_connection
				<< connect(project,
					       &QETProject::projectDiagramsOrderChanged,
					       this, &CrossRefItem::updateLabel);
		m_update_connection << connect(project,
					       &QETProject::diagramRemoved,
					       this, &CrossRefItem::updateLabel);
		m_update_connection << connect(m_element,
					       &Element::linkedElementChanged,
					       this, &CrossRefItem::linkedChanged);

		auto diagram_ = dynamic_cast<Diagram *>(this->scene());
		auto formula_ = m_properties.masterLabel();
		if (diagram_ &&
			formula_.contains("%F"))
		{
			m_update_connection << connect(diagram_ , &Diagram::diagramInformationChanged,
										   this, &CrossRefItem::updateLabel);
		}
		linkedChanged();
		updateLabel();
	}
}

/**
	@brief CrossRefItem::boundingRect
	@return the bounding rect of this item
*/
QRectF CrossRefItem::boundingRect() const
{
	return m_bounding_rect;
}

/**
	@brief CrossRefItem::shape
	@return the shape of this item
*/
QPainterPath CrossRefItem::shape() const{
	return m_shape_path;
}

/**
	@brief CrossRefItem::elementPositionText
	@param elmt
	@param add_prefix
	@return the string corresponding to the position of elmt in the diagram.
	if add_prefix is true,
	prefix (for power and delay contact) is added to the position text.
*/
QString CrossRefItem::elementPositionText(
		const Element *elmt, const bool &add_prefix) const
{
	autonum::sequentialNumbers seq;
	QString txt = autonum::AssignVariables::formulaToLabel(
				m_properties.masterLabel(),
				seq, elmt->diagram(),
				elmt);

	if (add_prefix)
	{
		if      (elmt->kindInformations()["type"].toString() == "power")
			txt.prepend(m_properties.prefix("power"));
		else if (elmt->kindInformations()["type"].toString().contains("delay"))
			txt.prepend(m_properties.prefix("delay"));
		else if (elmt->kindInformations()["state"].toString() == "SW")
			txt.prepend(m_properties.prefix("switch"));
	}
	return txt;
}

/**
	@brief CrossRefItem::updateProperties
	update the current properties
*/
void CrossRefItem::updateProperties()
{
	XRefProperties xrp = m_element->diagram()->project()->defaultXRefProperties(m_element->kindInformations()["type"].toString());
	
	if (m_properties != xrp)
	{
		m_properties = xrp;
		hide();
		if(m_properties.snapTo() == XRefProperties::Label && (m_text || m_group)) //Snap to label and parent is text or group
			show();
		else if((m_properties.snapTo() == XRefProperties::Bottom && !m_text && !m_group)) //Snap to bottom of element is the parent
			show();
		
		setUpConnection();
		updateLabel();
	}
}

/**
	@brief CrossRefItem::updateLabel
	Update the content of the item
*/
void CrossRefItem::updateLabel()
{
		//init the shape and bounding rect
	m_shape_path    = QPainterPath();
	prepareGeometryChange();
	m_bounding_rect = QRectF();

		//init the painter
	QPainter qp;
	qp.begin(&m_drawing);
	QPen pen_;
	pen_.setWidthF(0.5);
	qp.setPen(pen_);
	qp.setFont(QETApp::diagramTextsFont(5));

		//Draw cross or contact, only if master element is linked.
	if (! m_element->linkedElements().isEmpty())
	{
		XRefProperties::DisplayHas dh = m_properties.displayHas();

		if (dh == XRefProperties::Cross)
			drawAsCross(qp);
		else if (dh == XRefProperties::Contacts)
			drawAsContacts(qp);
	}
	qp.end();

	autoPos();
	update();
}

/**
	@brief CrossRefItem::autoPos
	Calculate and set position automatically.
*/
void CrossRefItem::autoPos()
{
	//We calcul the position according to the snapTo of the xrefproperties
	if (m_properties.snapTo() == XRefProperties::Bottom)
		centerToBottomDiagram(this,
				      m_element,
				      m_properties.offset() <= 40
				      ? 5
				      : m_properties.offset());
	else
		centerToParentBottom(this);
}

/**
	@brief CrossRefItem::sceneEvent
	@param event
	@return
*/
bool CrossRefItem::sceneEvent(QEvent *event)
{
	//By default when a QGraphicsItem is a child of a QGraphicsItemGroup
	//all events are forwarded to group.
	//We override it, when this Xref is in a group
	if(m_group)
	{
		switch (event->type())
		{
			case QEvent::GraphicsSceneHoverEnter:
				hoverEnterEvent(static_cast<QGraphicsSceneHoverEvent *>(event));
				break;
			case QEvent::GraphicsSceneHoverMove:
				hoverMoveEvent(static_cast<QGraphicsSceneHoverEvent *>(event));
				break;
			case QEvent::GraphicsSceneHoverLeave:
				hoverLeaveEvent(static_cast<QGraphicsSceneHoverEvent *>(event));
				break;
			case QEvent::GraphicsSceneMouseDoubleClick:
				mouseDoubleClickEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
				break;
			default:break;
		}
		return true;
	}
	
	return QGraphicsObject::sceneEvent(event);
}

/**
	@brief CrossRefItem::paint
	Paint this item
	@param painter
	@param option
	@param widget
*/
void CrossRefItem::paint(
		QPainter *painter,
		const QStyleOptionGraphicsItem *option,
		QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)
	m_drawing.play(painter);
}

/**
	@brief CrossRefItem::mouseDoubleClickEvent
	@param event
*/
void CrossRefItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	event->accept();
	if (m_hovered_contact && m_hovered_contact->scene())
	{
			//Show and select the linked slave element
		if (scene() != m_hovered_contact->scene())
		{
			m_hovered_contact->diagram()->showMe();
		}
		m_hovered_contact->setSelected(true);

			//Zoom to the linked slave element
		foreach(QGraphicsView *view,
				m_hovered_contact->diagram()->views())
		{
			QRectF fit = m_hovered_contact->sceneBoundingRect();
			fit.adjust(-200, -200, 200, 200);
			view->fitInView(fit, Qt::KeepAspectRatioByExpanding);
		}
	}
}

/**
	@brief CrossRefItem::hoverEnterEvent
	@param event
*/
void CrossRefItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	m_hovered_contact = nullptr;
		QGraphicsObject::hoverEnterEvent(event);
}

/**
	@brief CrossRefItem::hoverMoveEvent
	@param event
*/
void CrossRefItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
	QPointF pos = event->pos();

	if (m_hovered_contact)
	{
		foreach(QRectF rect, m_hovered_contacts_map.values(m_hovered_contact))
		{
			//Mouse hover the same rect than previous hover event
			if (rect.contains(pos))
			{
				QGraphicsObject::hoverMoveEvent(event);
				return;
			}
		}

		//At this point, mouse don't hover previous rect
		m_hovered_contact = nullptr;

		foreach (Element *elmt, m_hovered_contacts_map.keys())
		{
			foreach(QRectF rect, m_hovered_contacts_map.values(elmt))
			{
					//Mouse hover a contact
				if (rect.contains(pos))
				{
					m_hovered_contact = elmt;
				}
			}
		}

		updateLabel();
		QGraphicsObject::hoverMoveEvent(event);
		return;
	}
	else
	{
		foreach (Element *elmt, m_hovered_contacts_map.keys())
		{
			foreach(QRectF rect, m_hovered_contacts_map.values(elmt))
			{
				//Mouse hover a contact
				if (rect.contains(pos))
				{
					m_hovered_contact = elmt;
					updateLabel();
					QGraphicsObject::hoverMoveEvent(event);
					return;
				}
			}
		}
	}
}

/**
	@brief CrossRefItem::hoverLeaveEvent
	@param event
*/
void CrossRefItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	m_hovered_contact = nullptr;
	updateLabel();
	QGraphicsObject::hoverLeaveEvent(event);
}

/**
	@brief CrossRefItem::linkedChanged
*/
void CrossRefItem::linkedChanged()
{
	for(const QMetaObject::Connection& c : m_slave_connection)
		disconnect(c);

	m_slave_connection.clear();

	if(!isVisible())
		return;
	
	for(Element *elmt : m_element->linkedElements())
	{
		m_slave_connection << connect(elmt,
					      &Element::xChanged,
					      this,
					      &CrossRefItem::updateLabel);
		m_slave_connection << connect(elmt,
					      &Element::yChanged,
					      this,
					      &CrossRefItem::updateLabel);
	}

	updateLabel();
}

/**
	@brief CrossRefItem::buildHeaderContact
	Draw the QPicture of m_hdr_no_ctc and m_hdr_nc_ctc
*/
void CrossRefItem::buildHeaderContact()
{
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
	@brief CrossRefItem::setUpCrossBoundingRect
	Get the numbers of slaves elements linked to this parent element,
	for calculate the size of the cross bounding rect.
	The cross ref item is drawing according to the size of
	the cross bounding rect.
	@param painter
*/
void CrossRefItem::setUpCrossBoundingRect(QPainter &painter)
{
	//No need to calcul if nothing is linked
	if (m_element->isFree()) return;

	QStringList no_str, nc_str;

	for (auto elmt : NOElements()) {
		no_str.append(elementPositionText(elmt, true));
	}
	for (auto elmt : NCElements()) {
		nc_str.append(elementPositionText(elmt, true));
	}

	//There is no string to display, we return now
	if (no_str.isEmpty() && nc_str.isEmpty()) return;

	//this is the default size of cross ref item
	QRectF default_bounding(0, 0, 40, header + cross_min_height);

	//Bounding rect of the NO text
	QRectF no_bounding;
	for (auto str : no_str)
	{
		QRectF bounding = painter.boundingRect(QRectF (), Qt::AlignCenter, str);
		no_bounding = no_bounding.united(bounding);
		no_bounding.setHeight(no_bounding.height() + bounding.height());
	}
	//Adjust according to the NO
	if (no_bounding.height() > default_bounding.height() - header)
		default_bounding.setHeight(no_bounding.height() + header); //adjust the height
	if (no_bounding.width() > default_bounding.width()/2)
		default_bounding.setWidth(no_bounding.width()*2);//adjust the width

	//Bounding rect of the NC text
	QRectF nc_bounding;
	for (auto str : nc_str)
	{
		QRectF bounding = painter.boundingRect(QRectF (), Qt::AlignCenter, str);
		nc_bounding = nc_bounding.united(bounding);
		nc_bounding.setHeight(nc_bounding.height() + bounding.height());
	}
	//Adjust according to the NC
	if (nc_bounding.height() > default_bounding.height() - header)
		default_bounding.setHeight(nc_bounding.height() + header); //adjust the height
	if (nc_bounding.width() > default_bounding.width()/2)
		default_bounding.setWidth(nc_bounding.width()*2);//adjust the width

	//Minor adjustement for better visual
	default_bounding.adjust(0, 0, 4, 0);
	m_shape_path.addRect(default_bounding);
	prepareGeometryChange();
	m_bounding_rect = default_bounding;
}

/**
	@brief CrossRefItem::drawAsCross
	Draw this crossref with a cross
	@param painter: painter to use
*/
void CrossRefItem::drawAsCross(QPainter &painter)
{
	//calcul the size of the cross
	setUpCrossBoundingRect(painter);
	m_hovered_contacts_map.clear();

	//Bounding rect is empty that mean there's no contact to draw
	if (boundingRect().isEmpty()) return;

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
	@brief CrossRefItem::drawAsContacts
	Draw this crossref with symbolic contacts
	@param painter painter to use
*/
void CrossRefItem::drawAsContacts(QPainter &painter)
{
	if (m_element -> isFree())
		return;

	m_drawed_contacts = 0;
	m_hovered_contacts_map.clear();
	QRectF bounding_rect;

	//Draw each linked contact
	foreach (Element *elmt,  m_element->linkedElements())
	{
		DiagramContext info = elmt->kindInformations();

		for (int i=0; i<info["number"].toInt(); i++)
		{
			int option = 0;

			QString state = info["state"].toString();
				 if (state == "NO") option = NO;
			else if (state == "NC") option = NC;
			else if (state == "SW") option = SW;
			else if (state == "Other") option = Other;

			QString type = info["type"].toString();
				 if (type == "power")    option += Power;
			else if (type == "delayOn")  option += DelayOn;
			else if (type == "delayOff") option += DelayOff;
			else if (type == "delayOnOff") option += DelayOnOff;

			QRectF br = drawContact(painter, option, elmt);
			bounding_rect = bounding_rect.united(br);
		}
	}

	bounding_rect.adjust(-30, -4, 4, 4);
	prepareGeometryChange();
	m_bounding_rect = bounding_rect;
	m_shape_path.addRect(bounding_rect);
}

/**
	@brief CrossRefItem::drawContact
	Draw one contact, the type of contact to draw is define in flags.
	@param painter : painter to use
	@param flags : define how to draw the contact (see enul CONTACTS)
	@param elmt : the element to display text (the position of the contact)
	@return The bounding rect of the draw (contact + text)
*/
QRectF CrossRefItem::drawContact(QPainter &painter, int flags, Element *elmt)
{
	QString str = elementPositionText(elmt);
	int offset = m_drawed_contacts*10;
	QRectF bounding_rect = QRectF(0, offset, 24, 10);
	
	QPen pen = painter.pen();
	m_hovered_contact == elmt ? pen.setColor(Qt::blue) :pen.setColor(Qt::black);
	painter.setPen(pen);

	//Draw NO or NC contact
	if (flags &NOC)
	{
		bounding_rect = QRectF(0, offset, 24, 10);

		//draw the basic line
		painter.drawLine(0, offset+6, 8, offset+6);
		painter.drawLine(16, offset+6, 24, offset+6);

		///take example of this code for display the terminal text
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
					painter.drawLine(12, offset+8, 12, offset+11);
					QRectF r(9.5, offset+9, 5, 3);
					painter.drawArc(r, 180*16, 180*16);
				}
				if (flags &NC) {
					painter.drawLine(QPointF(12.5, offset+5), QPointF(12.5, offset+8));
					QRectF r(10, offset+6, 5, 3);
					painter.drawArc(r, 180*16, 180*16);
				}
			}
			// for delay off contact
			else if ( flags &DelayOff){
				if (flags &NO) {
					painter.drawLine(12, offset+8, 12, offset+9.5);
					QRectF r(9.5, offset+9.5, 5, 3);
					painter.drawArc(r, 0, 180*16);
				}
				if (flags &NC) {
					painter.drawLine(QPointF(12.5, offset+5), QPointF(12.5, offset+7));
					QRectF r(10, offset+7.5, 5, 3);
					painter.drawArc(r, 0, 180*16);
				}

			}
			else {
					// for delay on contact
				if (flags &NO) {
					painter.drawLine(12, offset+8, 12, offset+11);
					QRectF r(9.5, offset+11.7, 5, 3);
					painter.drawArc(r, 0, 180*16);
					QRectF rl(9.5, offset+9, 5, 3);
					painter.drawArc(rl, 180*16, 180*16);
				}
				if (flags &NC) {
					painter.drawLine(QPointF(12.5, offset+5), QPointF(12.5, offset+8));
					QRectF r(9.5, offset+10.7, 5, 3);
					painter.drawArc(r, 0, 180*16);
					QRectF rl(9.5, offset+8, 5, 3);
					painter.drawArc(rl, 180*16, 180*16);
				}
			}
		}

		QRectF text_rect = painter.boundingRect(QRectF(30, offset, 5, 10), Qt::AlignLeft | Qt::AlignVCenter, str);
		painter.drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, str);
		bounding_rect = bounding_rect.united(text_rect);

		if (m_hovered_contacts_map.contains(elmt))
		{
			m_hovered_contacts_map.insert(elmt, bounding_rect);
		}
		else
		{
			m_hovered_contacts_map.insert(elmt, bounding_rect);
		}

		++m_drawed_contacts;
	}

		//Draw a switch contact
	
	
	
	else if (flags &SW)
	{
		bounding_rect = QRectF(0, offset, 24, 20);
		bounding_rect.adjust(-20, -4, 4, 4);

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

			//Draw the half ellipse off delay
		if (flags &Delay)
		{
			painter.drawLine(12, offset+13, 12, offset+16);
			if (flags &DelayOn) {
				QRectF r(9.5, offset+14, 5, 3);
				painter.drawArc(r, 180*16, 180*16);
			}
			else if (flags &DelayOff) {
				QRectF r(9.5, offset+16.5, 5, 3);
				painter.drawArc(r, 0, 180*16);
			}
			else if (flags &DelayOnOff) {
				QRectF r(9.5, offset+14, 5, 3);
				painter.drawArc(r, 180*16, 180*16);
				QRectF rr(9.5, offset+17, 5, 3);
				painter.drawArc(rr, 0, 180*16);
			}
			
		}

			//Draw position text
		QRectF text_rect = painter.boundingRect(
					QRectF(30, offset+4, 5, 10),
					Qt::AlignLeft | Qt::AlignVCenter,
					str);
		painter.drawText(text_rect,
				 Qt::AlignLeft | Qt::AlignVCenter,
				 str);
		bounding_rect = bounding_rect.united(text_rect);

		if (m_hovered_contacts_map.contains(elmt)) {
			m_hovered_contacts_map.insert(elmt, bounding_rect);
		}
		else {
			m_hovered_contacts_map.insert(elmt, bounding_rect);
		}

			//a switch contact take place of two normal contact
		m_drawed_contacts += 2;
		
	}
	
		//Draw Other symbol "ↈ"
	else if(flags &Other)
	{
		bounding_rect = QRectF(0, offset, 24, 20);
		bounding_rect.adjust(-20, -4, 4, 4);
	
			//Draw the first arc symbol
		QRectF r(8, offset+4, 5, 3);
				painter.drawArc(r, 10*16, 270*16);
		
			//Draw the second arc symbol
		QRectF r2(11.2, offset+4, 5, 3);
				painter.drawArc(r2, 160*16, 300*16);

			//Draw position text
		QRectF text_rect = painter.boundingRect(
					QRectF(30, offset, 5, 10), 
					Qt::AlignLeft | Qt::AlignVCenter, 
					str);
		painter.drawText(text_rect,
					Qt::AlignLeft | Qt::AlignVCenter, 
					str);
		bounding_rect = bounding_rect.united(text_rect);

		if (m_hovered_contacts_map.contains(elmt)) {
			m_hovered_contacts_map.insert(elmt, bounding_rect);
		}
		else {
			m_hovered_contacts_map.insert(elmt, bounding_rect);
		}
		++m_drawed_contacts;
	}
		return bounding_rect;
}


/**
	@brief CrossRefItem::fillCrossRef
	Fill the content of the cross ref
	@param painter painter to use.
*/
void CrossRefItem::fillCrossRef(QPainter &painter)
{
	if (m_element->isFree()) return;

	qreal middle_cross = m_bounding_rect.width()/2;

		//Fill NO
	QPointF no_top_left(0, header);
	foreach(Element *elmt, NOElements())
	{
		QPen pen = painter.pen();
		m_hovered_contact == elmt ? pen.setColor(Qt::blue) :pen.setColor(Qt::black);
		painter.setPen(pen);

		QString str = elementPositionText(elmt, true);
		QRectF bounding = painter.boundingRect(
					QRectF(no_top_left,
					       QSize(middle_cross, 1)),
					Qt::AlignLeft,
					str);
		painter.drawText(bounding, Qt::AlignLeft, str);

		if (m_hovered_contacts_map.contains(elmt))
		{
			m_hovered_contacts_map.insert(elmt, bounding);
		}
		else
		{
			m_hovered_contacts_map.insert(elmt, bounding);
		}

		no_top_left.ry() += bounding.height();
	}

		//Fill NC
	QPointF nc_top_left(middle_cross, header);
	foreach(Element *elmt, NCElements())
	{
		QPen pen = painter.pen();
		m_hovered_contact == elmt ? pen.setColor(Qt::blue)
					  :pen.setColor(Qt::black);
		painter.setPen(pen);

		QString str = elementPositionText(elmt, true);
		QRectF bounding = painter.boundingRect(
					QRectF(nc_top_left,
					       QSize(middle_cross, 1)),
					Qt::AlignRight,
					str);
		painter.drawText(bounding, Qt::AlignRight, str);

		if (m_hovered_contacts_map.contains(elmt))
		{
			m_hovered_contacts_map.insert(elmt, bounding);
		}
		else
		{
			m_hovered_contacts_map.insert(elmt, bounding);
		}

		nc_top_left.ry() += bounding.height();
	}
}

/**
	@brief CrossRefItem::AddExtraInfo
	Add the comment info of the parent item if needed.
	@param painter painter to use for draw the text
	@param type type of Info do be draw e.g. comment, location.
*/
void CrossRefItem::AddExtraInfo(QPainter &painter, const QString& type)
{
	QString text = autonum::AssignVariables::formulaToLabel(
				m_element->elementInformations()[type].toString(),
				m_element->rSequenceStruct(),
				m_element->diagram(),
				m_element);
	bool must_show  = m_element -> elementInformations().keyMustShow(type);

	if (!text.isEmpty() && must_show)
	{
		painter.save();
		painter.setFont(QETApp::diagramTextsFont(6));

		QRectF r, text_bounding;
		qreal center = boundingRect().center().x();
		qreal width = boundingRect().width() > 70
				? boundingRect().width()/2
				: 35;

		r = QRectF(QPointF(center - width,
				   boundingRect().bottom()),
				   QPointF(center + width,
					   boundingRect().bottom() + 1));

		text_bounding = painter.boundingRect(
					r,
					Qt::TextWordWrap | Qt::AlignHCenter,
					text);
		painter.drawText(text_bounding,
				 Qt::TextWordWrap | Qt::AlignHCenter,
				 text);

		text_bounding.adjust(-1,0,1,0); //adjust only for better visual

		m_shape_path.addRect(text_bounding);
		prepareGeometryChange();
		m_bounding_rect = m_bounding_rect.united(text_bounding);
		if (type == "comment") painter.drawRoundedRect(text_bounding,
							       2,
							       2);
		painter.restore();
	}
}

/**
	@brief CrossRefItem::NOElements
	@return The linked elements of m_element which are open or switch contact.
	If linked element is a power contact,
	xref property is set to not show power contact
	and this cross item must be drawn as a cross,
	the element is not appended in the list.
*/
QList<Element *> CrossRefItem::NOElements() const
{
	QList<Element *> no_list;

	foreach (Element *elmt, m_element->linkedElements())
	{
		//We continue if element is a power contact and xref property
		//is set to not show power contact
		if (m_properties.displayHas() == XRefProperties::Cross &&
			!m_properties.showPowerContact() &&
			elmt -> kindInformations()["type"].toString() == "power")
			continue;

		QString state = elmt->kindInformations()["state"].toString();

		if (state == "NO" || state == "SW")
		{
			no_list.append(elmt);
		}
	}

	return no_list;
}

/**
	@brief CrossRefItem::NCElements
	@return The linked elements of m_element which are close
	or switch contact
	If linked element is a power contact,
	xref property is set to not show power contact
	and this cross item must be drawn as a cross,
	the element is not appended in the list.
*/
QList<Element *> CrossRefItem::NCElements() const
{
	QList<Element *> nc_list;

	foreach (Element *elmt, m_element->linkedElements())
	{
		//We continue if element is a power contact and xref property
		//is set to not show power contact
		if (m_properties.displayHas() == XRefProperties::Cross &&
			!m_properties.showPowerContact() &&
			elmt -> kindInformations()["type"].toString() == "power")
			continue;

		QString state = elmt->kindInformations()["state"].toString();

		if (state == "NC" || state == "SW")
		{
			nc_list.append(elmt);
		}
	}

	return nc_list;
}
