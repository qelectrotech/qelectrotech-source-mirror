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
#ifndef DVEVENTADDSHAPE_H
#define DVEVENTADDSHAPE_H

#include "dveventinterface.h"
#include "qetshapeitem.h"

class QMouseEvent;

class DVEventAddShape : public DVEventInterface
{
	public:
		DVEventAddShape(DiagramView *dv, QetShapeItem::ShapeType shape_type);
		virtual ~DVEventAddShape ();
		virtual bool mousePressEvent       (QMouseEvent *event);
		virtual bool mouseMoveEvent        (QMouseEvent *event);
		virtual bool mouseReleaseEvent     (QMouseEvent *event);
		virtual bool mouseDoubleClickEvent (QMouseEvent *event);

	private:
		void updateHelpCross (const QPoint &p);

	protected:
		QetShapeItem::ShapeType  m_shape_type;
		QetShapeItem            *m_shape_item;
		QGraphicsLineItem       *m_help_horiz, *m_help_verti;
};

#endif // DVEVENTADDSHAPE_H
