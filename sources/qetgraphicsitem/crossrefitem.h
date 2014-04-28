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
#ifndef CROSSREFITEM_H
#define CROSSREFITEM_H

#include "qetgraphicsitem/qetgraphicsitem.h"
#include"properties/xrefproperties.h"
class element;

/**
 * @brief The CrossRefItem class
 * This clas provide an item, for show the cross reference, like the contacts linked to a coil.
 * The item setpos automaticaly when parent move.
 * All slave displayed in cross ref will be updated when folio position change in the project.
 * It's the responsability of the parent to informe displayed slave are moved,
 * by calling the slot @updateLabel
 */
class CrossRefItem : public QGraphicsObject
{
	Q_OBJECT

	//Methods
	public:
	explicit CrossRefItem(Element *elmt, QGraphicsItem *parent = 0);
	~CrossRefItem();

	enum { Type = UserType + 1009 };
	virtual int type() const { return Type; }

	enum CONTACTS {
		NO = 1,
		NC = 2,
		Power = 4,
		DelayOn = 8,
		DelayOff = 16
	};

	QRectF boundingRect() const;
	virtual QPainterPath shape() const;

	signals:

	public slots:
	void updateLabel();
	void autoPos();

	protected:
	virtual void paint			   (QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	virtual void mouseMoveEvent	   (QGraphicsSceneMouseEvent *e);
	virtual void mouseReleaseEvent (QGraphicsSceneMouseEvent *e);

	private:
	void setUpCrossBoundingRect();
	void drawHasCross	 (QPainter &painter);
	void drawHasContacts (QPainter &painter);
	void drawContact	 (QPainter &painter, int flags, QString str = QString());
	void fillCrossRef	 (QPainter &painter);
	void AddExtraInfo	 (QPainter &painter);

	//Attributes
	private:
	Element		  *m_element; //element to display the cross reference
	QRectF		   m_bounding_rect;
	QPicture	   m_drawing;
	QPainterPath   m_shape_path;
	XRefProperties m_properties;
	int			   m_drawed_contacts;
};

#endif // CROSSREFITEM_H
