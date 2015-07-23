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
#ifndef PART_ARC_H
#define PART_ARC_H

#include "abstractpartellipse.h"
#include "QetGraphicsItemModeler/qetgraphicshandlerutility.h"

class QPropertyUndoCommand;

/**
 * @brief The PartArc class
 * This class represents an elliptical arc primitive which may be used to
 * compose the drawing of an electrical element within the element editor.
 */
class PartArc : public AbstractPartEllipse
{
		Q_OBJECT

	public:
		PartArc(QETElementEditor *editor, QGraphicsItem *parent = 0);
		virtual ~PartArc();
	
	private:
		PartArc(const PartArc &);
		// methods
	public:
		enum { Type = UserType + 1101 };
			/**
			 * Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a PartArc.
			 * @return the QGraphicsItem type
			 */
		virtual int type() const { return Type; }
		virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0);

			//Name and XML
		virtual QString name()    const { return(QObject::tr("arc", "element part name")); }
		virtual QString xmlName() const { return(QString("arc")); }
		virtual const QDomElement toXml   (QDomDocument &) const;
		virtual void              fromXml (const QDomElement &);

		virtual QRectF boundingRect()  const;
		virtual QPainterPath shape() const;
		virtual QPainterPath shadowShape() const;

	protected:
		virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
		virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

	private:
		QetGraphicsHandlerUtility m_handler;
		int m_handler_index;
		QPropertyUndoCommand *m_undo_command;
};
#endif
