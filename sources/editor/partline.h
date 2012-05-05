/*
	Copyright 2006-2012 Xavier Guerrin
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
	Cette classe represente une ligne pouvant etre utilisee pour composer le
	dessin d'un element dans l'editeur d'element.
	Une ligne est composee de deux points. Elle peut comporter des extremites
	speciales definissables grace aux methodes setFirstEndType et
	setSecondEndType. La taille des extremites est definissable via les
	methodes setFirstEndLength et setSecondEndLength.
	A noter que les extremites ne sont pas dessinees si la longueur requise
	pour leur dessin n'est pas contenue dans la ligne. S'il n'y a de la place
	que pour une seule extremite, c'est la premiere qui est privilegiee.
*/
class PartLine : public QGraphicsLineItem, public CustomElementGraphicPart {
	// constructeurs, destructeur
	public:
	PartLine(QETElementEditor *, QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartLine();
	
	private:
	PartLine(const PartLine &);
	
	// attributs
	private:
	QET::EndType first_end;
	qreal first_length;
	QET::EndType second_end;
	qreal second_length;
	
	// methodes
	public:
	enum { Type = UserType + 1104 };
	
	/**
		permet de caster un QGraphicsItem en PartLine avec qgraphicsitem_cast
		@return le type de QGraphicsItem
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
