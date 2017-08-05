/*
	Copyright 2006-2017 The QElectroTech Team
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

class QPropertyUndoCommand;

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
		PartEllipse(QETElementEditor *editor, QGraphicsItem * parent = nullptr);
		~PartEllipse() override;
	
	private:
		PartEllipse(const PartEllipse &);
	
		// methods
	public:
		enum { Type = UserType + 1103 };
			/**
			 * Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a PartEllipse.
			 * @return the QGraphicsItem type
			 */
		int type() const override { return Type; }	
		void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = nullptr) override;

			//Name and XML
		QString name()    const override { return(QObject::tr("ellipse", "element part name")); }
		QString xmlName() const override { return(QString("ellipse")); }
		const QDomElement toXml   (QDomDocument &) const override;
		void              fromXml (const QDomElement &) override;
		QPainterPath shape() const override;
		QPainterPath shadowShape() const override;
		void setRect(const QRectF &rect) override {AbstractPartEllipse::setRect(rect); adjusteHandlerPos();}

	protected:
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
		QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
		bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;

	private:
		void switchResizeMode();
		void adjusteHandlerPos();
		void handlerMousePressEvent   (QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event);
		void handlerMouseMoveEvent    (QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event);
		void handlerMouseReleaseEvent (QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event);
		void sceneSelectionChanged ();
		
		void addHandler();
		void removeHandler();

	private:
		QPropertyUndoCommand *m_undo_command;
		int m_resize_mode = 1,
			m_vector_index = -1;
};
#endif
