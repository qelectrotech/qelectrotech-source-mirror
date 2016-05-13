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
#ifndef CROSSREFITEM_H
#define CROSSREFITEM_H

#include <QGraphicsObject>
#include"properties/xrefproperties.h"
#include <QPicture>

class Element;

/**
 * @brief The CrossRefItem class
 * This clas provide an item, for show the cross reference, like the contacts linked to a coil.
 * The item setpos automaticaly when parent move.
 * All slave displayed in cross ref will be updated when folio position change in the project.
 * It's the responsability of the master element to informe displayed slave are moved,
 * by calling the slot @updateLabel
 * By default master element is the parent graphics item of this Xref,
 * but if the Xref must be snap to the label of master, the label become the parent of this Xref.
 * This behavior can be changed at anytime by calling setProperties.
 */
class CrossRefItem : public QGraphicsObject
{
	Q_OBJECT

	//Methods
	public:
	explicit CrossRefItem(Element *elmt);
	~CrossRefItem();

	enum { Type = UserType + 1009 };
	virtual int type() const { return Type; }

	enum CONTACTS {
		NO       = 1,
		NC       = 2,
		NOC      = 3,
		SW       = 4,
		Power    = 8,
		DelayOn  = 16,
		DelayOff = 32,
		Delay    = 48
	};

	QRectF boundingRect			() const;
	virtual QPainterPath shape	() const;
	QString elementPositionText (const Element *elmt, const bool &add_prefix = false) const;
	void allElementsPositionText (QString &no_str, QString &nc_str,const bool &add_prefix = false) const;

	signals:

	public slots:
	void updateProperties ();
	void updateLabel   ();
	void autoPos	   ();

	protected:
	virtual void paint (QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	virtual void mouseDoubleClickEvent (QGraphicsSceneMouseEvent * event );

	private:
	void buildHeaderContact		();
	void setUpCrossBoundingRect (QPainter &painter);
	void drawHasCross			(QPainter &painter);
	void drawHasContacts		(QPainter &painter);
	void drawContact			(QPainter &painter, int flags, QString str = QString());
	void fillCrossRef			(QPainter &painter);
	void AddExtraInfo			(QPainter &painter);
	void setTextParent			();

	//Attributes
	private:
	Element		  *m_element; //element to display the cross reference
	QRectF		   m_bounding_rect;
	QPicture	   m_drawing, m_hdr_no_ctc, m_hdr_nc_ctc;
	QPainterPath   m_shape_path;
	XRefProperties m_properties;
	int			   m_drawed_contacts;
};

#endif // CROSSREFITEM_H

