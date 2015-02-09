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
#ifndef PART_ELLIPSE_H
#define PART_ELLIPSE_H

#include "abstractpartellipse.h"

/**
 * @brief The PartEllipse class
 * This class represents an ellipse primitive which may be used to compose the
 * drawing of an electrical element within the element editor.
 */
class PartEllipse : public  AbstractPartEllipse
{
		Q_OBJECT

		// constructors, destructor
	public:
		PartEllipse(QETElementEditor *editor, QGraphicsItem * parent = 0);
		virtual ~PartEllipse();
	
	private:
		PartEllipse(const PartEllipse &);
	
		// methods
	public:
		enum { Type = UserType + 1103 };
			/**
			 * Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a PartEllipse.
			 * @return the QGraphicsItem type
			 */
		virtual int type() const { return Type; }	
		virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0);

			//Name and XML
		virtual QString name()    const { return(QObject::tr("ellipse", "element part name")); }
		virtual QString xmlName() const { return(QString("ellipse")); }
		virtual const QDomElement toXml   (QDomDocument &) const;
		virtual void              fromXml (const QDomElement &);

		virtual QPainterPath shape() const;
};
#endif
