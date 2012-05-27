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
#ifndef ELEMENT_TEXT_ITEM_H
#define ELEMENT_TEXT_ITEM_H
#include "diagramtextitem.h"
#include <QtXml>
class Diagram;
class Element;
/**
	Cette classe represente un champ de texte rattache a un element.
	Il est editable et deplacable (relativement a son element parent) par
	l'utilisateur.
	Il peut egalement etre oriente a un angle quelconque.
	Il est possible pour ce champ de texte de rester dans le sens de la lecture
	malgre les rotations de son element parent.
*/
class ElementTextItem : public DiagramTextItem {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	ElementTextItem(Element * = 0, Diagram * = 0);
	ElementTextItem(const QString &, Element * = 0, Diagram * = 0);
	virtual ~ElementTextItem();
	
	// attributs
	public:
	enum { Type = UserType + 1003 };
	
	private:
	Element *parent_element_;
	bool follow_parent_rotations;
	QPointF original_position;
	qreal original_rotation_angle_;
	bool first_move_;
	
	// methodes
	public:
	virtual int type() const { return Type; }
	Element *parentElement() const;
	/// @return le rectangle delimitant le champ de texte
	virtual QRectF boundingRect() const { return(QGraphicsTextItem::boundingRect().adjusted(0.0, -1.1, 0.0, 0.0)); }
	bool followParentRotations() const;
	void setFollowParentRotations(bool);
	void fromXml(const QDomElement &);
	QDomElement toXml(QDomDocument &) const;
	void setPos(const QPointF &);
	void setPos(qreal, qreal);
	virtual QPointF pos() const;
	void setOriginalPos(const QPointF &);
	QPointF originalPos() const;
	void setOriginalRotationAngle(const qreal &);
	qreal originalRotationAngle() const;
	virtual void setFont(const QFont &);
	
	public slots:
	void adjustItemPosition(int = 0);
	
	protected:
	virtual void applyRotation(const qreal &);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
};

/**
	Permet de savoir si le champ de texte suit les rotations de son parent.
	@return true si le champ de texte suit les rotations de son parent, false
	sinon
*/
inline bool ElementTextItem::followParentRotations() const {
	return(follow_parent_rotations);
}

/**
	Permet de specifier si le champ de texte suit les rotations de son parent.
	@param frp true si le champ de texte doit suivre  les rotations de son
	parent, false pour qu'ils ne les suivent pas
*/
inline void ElementTextItem::setFollowParentRotations(bool frp) {
	follow_parent_rotations = frp;
}

#endif
