/*
	Copyright 2006-2013 QElectroTech Team
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
#ifndef DIAGRAM_IMAGE_ITEM_H
#define DIAGRAM_IMAGE_ITEM_H
#include <QtGui>
#include "diagram.h"
/**
	This class represents a selectable, movable and editable image on a
	diagram.
	@see QGraphicsItem::GraphicsItemFlags
*/
class DiagramImageItem : public QObject, public QGraphicsPixmapItem {
	Q_OBJECT
	// constructors, destructor
	public:
	DiagramImageItem(Diagram * = 0);
	DiagramImageItem(const QPixmap &, Diagram * = 0);
	virtual ~DiagramImageItem();
	
	// attributes
	public:
	enum { Type = UserType + 1007 };
	
	// methods
	public:
	/**
		Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a
		DiagramImageItem
		@return the QGraphicsItem type
	*/
	virtual int type() const { return Type; }
	Diagram *diagram() const;
	
	virtual bool fromXml(const QDomElement &);
	virtual QDomElement toXml(QDomDocument &) const;
	
	virtual void setPos(const QPointF &);
	virtual void setPos(qreal, qreal);
	void setRotationAngle(const qreal &);
	void rotateBy(const qreal &);
	void edit();
	void setPixmap(const QPixmap &pixmap);
	
	protected:
	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
	void mousePressEvent(QGraphicsSceneMouseEvent *e);
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
	void applyRotation(const qreal &);
	
	signals:
	/// signal emitted after image was changed
	void diagramImageChanged(DiagramImageItem *, const QString &, const QString &);
	
	private slots:
	void PreviewScale(int);
	
	private:
	bool first_move_;
	QPointF mouse_to_origin_movement_;
};
#endif
