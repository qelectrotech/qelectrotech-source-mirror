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
#ifndef QETSHAPEITEM_H
#define QETSHAPEITEM_H

#include "qetgraphicsitem.h"

class QDomElement;
class QDomDocument;

/**
 * @brief The QetShapeItem class
 * this class is used to draw a basic shape (line, rectangle, ellipse)
 * into a diagram, that can be saved to .qet file.
 */
class QetShapeItem : public QetGraphicsItem
{
	Q_OBJECT

	public:
	Q_ENUMS(ShapeType)
	enum ShapeType {Line	  =0,
					Rectangle =1,
					Ellipse	  =2,
					Polyline  =3 };

	enum { Type = UserType + 1008 };

	QetShapeItem(QPointF, QPointF = QPointF(0,0), ShapeType = Line, QGraphicsItem *parent = 0);
	virtual ~QetShapeItem();

	/**
		Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a QetShapeItem
		@return the QGraphicsItem type
	*/
	virtual int type() const { return Type; }

	///METHODS
	void setStyle(Qt::PenStyle);

	virtual bool	    fromXml (const QDomElement &);
	virtual QDomElement toXml	(QDomDocument &document) const;
	virtual bool		toDXF	(const QString &filepath);

	virtual void editProperty();
	virtual QString name() const;

	void setP2 (QPointF P2);

		//Methods available for polygon shape
	int  pointsCount   () const;
	void setNextPoint (QPointF P);
	void removePoints (int number = 1);

	QRectF boundingRect() const;
	QPainterPath shape()  const;

	protected:
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	virtual void hoverEnterEvent (QGraphicsSceneHoverEvent *event);
	virtual void hoverLeaveEvent (QGraphicsSceneHoverEvent *event);

	private:
	void changeGraphicsItem (const ShapeType &newtype);

	private slots:
	void previewScale(int factor);

	///ATTRIBUTES
	private:
	ShapeType    m_shapeType;
	Qt::PenStyle m_shapeStyle;
	QPointF		 m_P1, m_P2;
	QPolygonF	 m_polygon;
	bool         m_hovered;
};
#endif // QETSHAPEITEM_H
