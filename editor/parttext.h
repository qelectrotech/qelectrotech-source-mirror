/*
	Copyright 2006-2007 Xavier Guerrin
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
#ifndef PART_TEXT_H
#define PART_TEXT_H
#include <QtGui>
#include "customelementpart.h"
class TextEditor;
/**
	Cette classe represente un texte pouvant etre utilise pour composer le
	dessin d'un element dans l'editeur d'element.
*/
class PartText : public QGraphicsTextItem, public CustomElementPart {
	// constructeurs, destructeur
	public:
	PartText(QETElementEditor *, QGraphicsItem * = 0, ElementScene * = 0);
	virtual ~PartText();
	
	private:
	PartText(const PartText &);
	
	// attributs
	TextEditor *infos;
	
	// methodes
	public:
	enum { Type = UserType + 1107 };
	virtual int type() const { return Type; }
	virtual QString name() const { return(QObject::tr("texte")); }
	void fromXml(const QDomElement &);
	const QDomElement toXml(QDomDocument &) const;
	QWidget *elementInformations();
	QPointF pos() const;
	void setPos(const QPointF &);
	void setPos(qreal, qreal);
	virtual void setProperty(const QString &, const QVariant &);
	virtual QVariant property(const QString &);
	
	protected:
	virtual void focusOutEvent(QFocusEvent *);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *);
	virtual QVariant itemChange(GraphicsItemChange, const QVariant &);
	QRectF boundingRect() const;
	
	private:
	QPointF margin() const;
};
#endif
