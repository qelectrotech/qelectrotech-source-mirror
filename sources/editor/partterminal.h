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
#ifndef PART_TERMINAL_H
#define PART_TERMINAL_H
#include "customelementpart.h"
#include "qet.h"
#include <QtGui>
/**
	This class represents a terminal which may be used to compose the drawing of
	an electrical element within the element editor.
*/
class PartTerminal : public CustomElementPart, public QGraphicsItem {
	public:
	// constructors, destructor
	PartTerminal(QETElementEditor *, QGraphicsItem * = 0, QGraphicsScene * = 0);
	virtual ~PartTerminal();
	private:
	PartTerminal(const PartTerminal &);
	
	// attributes
	private:
	QET::Orientation _orientation;
	QPointF second_point;
	QString number_, name_;
	bool nameHidden_;
	
	// methods
	public:
	enum { Type = UserType + 1106 };
	/**
		Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into a
		PartTerminal.
		@return the QGraphicsItem type
	*/
	virtual int type() const { return Type; }
	virtual QString name() const { return(QObject::tr("borne", "element part name")); }
	virtual QString xmlName() const { return(QString("terminal")); }
	virtual void fromXml(const QDomElement &);
	virtual const QDomElement toXml(QDomDocument &) const;
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
	virtual QRectF boundingRect() const;
	QET::Orientation orientation() const;
	void setOrientation(QET::Orientation);
	QString number() const;
	void setNumber(const QString &);
	QString nameOfTerminal() const;
	void setName(const QString &);
	bool nameIsHidden() const;
	void setNameHidden(const bool &);
	
	virtual void setProperty(const QString &, const QVariant &);
	virtual QVariant property(const QString &);
	virtual bool isUseless() const;
	virtual QRectF sceneGeometricRect() const;
	virtual void startUserTransformation(const QRectF &);
	virtual void handleUserTransformation(const QRectF &, const QRectF &);
	
	protected:
	QVariant itemChange(GraphicsItemChange, const QVariant &);
	
	private:
	void updateSecondPoint();
	
	private:
	QPointF saved_position_;
};
#endif
