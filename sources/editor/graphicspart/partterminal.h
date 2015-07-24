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
#ifndef PART_TERMINAL_H
#define PART_TERMINAL_H

#include "customelementgraphicpart.h"

/**
	This class represents a terminal which may be used to compose the drawing of
	an electrical element within the element editor.
*/
class PartTerminal : public CustomElementGraphicPart
{
		Q_OBJECT

		Q_PROPERTY(Qet::Orientation orientation READ orientation WRITE setOrientation)

	public:
		// constructors, destructor
		PartTerminal(QETElementEditor *editor, QGraphicsItem *parent = 0);
		virtual ~PartTerminal();
	private:
		PartTerminal(const PartTerminal &);

	signals:
		void orientationChanged();
	
		// attributes
	private:
		Qet::Orientation m_orientation;
		QPointF second_point;
	
		// methods
	public:
		enum { Type = UserType + 1106 };
			/**
			 * Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a PartTerminal.
			 * @return the QGraphicsItem type
			 */
		virtual int type() const { return Type; }
		virtual QString name() const { return(QObject::tr("borne", "element part name")); }
		virtual QString xmlName() const { return(QString("terminal")); }
		virtual void fromXml(const QDomElement &);
		virtual const QDomElement toXml(QDomDocument &) const;
		virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

		virtual QPainterPath shape() const;
		virtual QPainterPath shadowShape() const {return shape();}
		virtual QRectF boundingRect() const;
		virtual bool isUseless() const;
		virtual QRectF sceneGeometricRect() const;
		virtual void startUserTransformation(const QRectF &);
		virtual void handleUserTransformation(const QRectF &, const QRectF &);

		Qet::Orientation orientation() const {return m_orientation;}
		void setOrientation(Qet::Orientation ori);
	
	private:
		void updateSecondPoint();
	
	private:
		QPointF saved_position_;
};
#endif
