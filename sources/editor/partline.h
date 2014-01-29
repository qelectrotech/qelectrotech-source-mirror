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
#ifndef PART_LINE_H
#define PART_LINE_H
#include <QtGui>
#include "customelementgraphicpart.h"
#include "qet.h"
/**
	This class represents a line primitive which may be used to compose the
	drawing of an electrical element within the element editor. Lines may have
	specific visual ends (e.g. arrows) through the setFirstEndType and
	setSecondEndType methods. Their size can be defined using the
	setFirstEndLength and setSecondEndLength methods. Please note ends are not
	drawn if the required length for their drawing is longer than the line itself.
	In case there is room for a single end only, the first one get priority.
*/
class PartLine : public QGraphicsLineItem, public CustomElementGraphicPart {
	// constructors, destructor
	public:
	PartLine(QETElementEditor *, QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartLine();
	
	private:
	PartLine(const PartLine &);
	
	// attributes
	private:
	QET::EndType first_end;
	qreal first_length;
	QET::EndType second_end;
	qreal second_length;
	QList<QPointF> saved_points_;
	
	// methods
	public:
	enum { Type = UserType + 1104 };
	
	/**
		Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a
		PartLine.
		@return the QGraphicsItem type
	*/
	virtual int type() const { return Type; }
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0);
	virtual QString name() const { return(QObject::tr("ligne", "element part name")); }
	virtual QString xmlName() const { return(QString("line")); }
	virtual const QDomElement toXml(QDomDocument &) const;
	virtual void fromXml(const QDomElement &);
	virtual QPointF sceneP1() const;
	virtual QPointF sceneP2() const;
	virtual QPainterPath shape() const;
	virtual QRectF boundingRect() const;
	virtual void setProperty(const QString &, const QVariant &);
	virtual QVariant property(const QString &);
	virtual bool isUseless() const;
	virtual QRectF sceneGeometricRect() const;
	virtual void startUserTransformation(const QRectF &);
	virtual void handleUserTransformation(const QRectF &, const QRectF &);
	virtual void setFirstEndType(const QET::EndType &);
	virtual QET::EndType firstEndType() const;
	virtual void setSecondEndType(const QET::EndType &);
	virtual QET::EndType secondEndType() const;
	virtual void setFirstEndLength(const qreal &);
	virtual qreal firstEndLength() const;
	virtual void setSecondEndLength(const qreal &);
	virtual qreal secondEndLength() const;
	static uint requiredLengthForEndType(const QET::EndType &);
	static QList<QPointF> fourEndPoints(const QPointF &, const QPointF &, const qreal &);
	protected:
	QVariant itemChange(GraphicsItemChange, const QVariant &);
	
	private:
	QList<QPointF> fourShapePoints() const;
	QRectF firstEndCircleRect() const;
	QRectF secondEndCircleRect() const;
	void debugPaint(QPainter *);
};
#endif
