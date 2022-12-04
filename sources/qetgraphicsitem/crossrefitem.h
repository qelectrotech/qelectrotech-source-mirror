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
#ifndef CROSSREFITEM_H
#define CROSSREFITEM_H

#include "../properties/xrefproperties.h"

#include <QGraphicsObject>
#include <QMultiMap>
#include <QPicture>

class Element;
class DynamicElementTextItem;
class ElementTextItemGroup;

/**
	@brief The CrossRefItem class
	This clas provide an item, for show the cross reference,
	like the contacts linked to a coil.
	The item setpos automatically when parent move.
	All slave displayed in cross ref will be updated
	when folio position change in the project.
	It's the responsibility of the master element
	to inform displayed slave are moved,
	by calling the slot updateLabel
	By default master element is the parent graphics item of this Xref,
	but if the Xref must be snap to the label of master,
	the label become the parent of this Xref.
	This behavior can be changed at anytime by calling setProperties.
*/
class CrossRefItem : public QGraphicsObject
{
	Q_OBJECT

		//Methods
	public:
		explicit CrossRefItem(Element *elmt);
		explicit CrossRefItem(
			Element *elmt, DynamicElementTextItem *text);
		explicit CrossRefItem(
			Element *elmt, ElementTextItemGroup *group);
		~CrossRefItem() override;
	private:
		void init();
		void setUpConnection();
	
	public:
		enum { Type = UserType + 1009 };
		int type() const override { return Type; }

		/**
			@brief The CONTACTS enum
		*/
		enum CONTACTS {
			NO		= 1,
			NC		= 2,
			NOC		= 3,
			SW		= 4,
			Power		= 8,
			DelayOn		= 16,
			DelayOff	= 32,
			DelayOnOff	= 64,
			Delay		= 112,
			Other	   = 128
		};

		QRectF boundingRect() const override;
		QPainterPath shape() const override;
		QString elementPositionText(
				const Element *elmt,
				const bool &add_prefix = false) const;

	public slots:
		void updateProperties();
		void updateLabel();
		void autoPos();

	protected:
		bool sceneEvent(QEvent *event) override;
		void paint(QPainter *painter,
			   const QStyleOptionGraphicsItem *option,
			   QWidget *widget) override;
		void mouseDoubleClickEvent (
				QGraphicsSceneMouseEvent * event ) override;
		void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
		void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
		void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

	private:
		void linkedChanged();
		void buildHeaderContact();
		void setUpCrossBoundingRect(QPainter &painter);
		void drawAsCross(QPainter &painter);
		void drawAsContacts(QPainter &painter);
		QRectF drawContact(QPainter &painter, int flags, Element *elmt);
		void fillCrossRef(QPainter &painter);
		void AddExtraInfo(QPainter &painter, const QString&);
		QList<Element *> NOElements() const;
		QList<Element *> NCElements() const;

		//Attributes
	private:
		Element *m_element; //element to display the cross reference
		QRectF m_bounding_rect;
		QPicture m_drawing, m_hdr_no_ctc, m_hdr_nc_ctc;
		QPainterPath m_shape_path;
		XRefProperties m_properties;
		int m_drawed_contacts;
		QMultiMap <Element *, QRectF> m_hovered_contacts_map;
		Element *m_hovered_contact = nullptr;
		DynamicElementTextItem *m_text = nullptr;
		ElementTextItemGroup *m_group = nullptr;
		QList <QMetaObject::Connection> m_slave_connection;
		QList <QMetaObject::Connection> m_update_connection;
};

#endif // CROSSREFITEM_H

