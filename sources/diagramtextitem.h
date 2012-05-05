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
#ifndef DIAGRAM_TEXT_ITEM_H
#define DIAGRAM_TEXT_ITEM_H
#include <QtGui>
#include "diagram.h"
/**
	Cette classe represente un champ de texte editable sur le schema.
	Par defaut, les DiagramTextItem sont Selectable et Movable.
	@see QGraphicsItem::GraphicsItemFlags
*/
class DiagramTextItem : public QGraphicsTextItem {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	DiagramTextItem(QGraphicsItem * = 0, Diagram * = 0);
	DiagramTextItem(const QString &, QGraphicsItem * = 0, Diagram * = 0);
	virtual ~DiagramTextItem();
	
	// attributs
	public:
	enum { Type = UserType + 1004 };
	
	// methodes
	public:
	/**
		Cette methode permet d'utiliser qgraphicsitem_cast sur cet objet
		@return le type de QGraphicsItem
	*/
	virtual int type() const { return Type; }
	Diagram *diagram() const;
	virtual void fromXml(const QDomElement &) = 0;
	virtual QDomElement toXml(QDomDocument &) const = 0;
	virtual void setPos(const QPointF &);
	virtual void setPos(qreal, qreal);
	virtual QPointF pos() const;
	qreal rotationAngle() const;
	void setRotationAngle(const qreal &);
	void rotateBy(const qreal &);
	QPointF mapMovementToScene(const QPointF &) const;
	QPointF mapMovementFromScene(const QPointF &) const;
	QPointF mapMovementToParent(const QPointF &) const;
	QPointF mapMovementFromParent(const QPointF &) const;
	
	protected:
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
	virtual void focusInEvent(QFocusEvent *);
	virtual void focusOutEvent(QFocusEvent *);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *);
	virtual void applyRotation(const qreal &);
	
	// signaux
	signals:
	/// signal emis lorsque le champ de texte perd le focus
	void lostFocus();
	/// signal emis lorsque le champ de texte a ete modifie
	void diagramTextChanged(DiagramTextItem *, const QString &, const QString &);

	// slots
	public slots:
	void setNonFocusable();
	
	// attributs prives
	private:
	/// Texte precedent
	QString previous_text_;
	/// angle de rotation du champ de texte
	qreal rotation_angle_;
};
#endif
