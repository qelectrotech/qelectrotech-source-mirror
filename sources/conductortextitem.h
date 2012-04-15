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
#ifndef CONDUCTOR_TEXT_ITEM_H
#define CONDUCTOR_TEXT_ITEM_H
#include "diagramtextitem.h"
class Conductor;
/**
	Cette classe represente un champ de texte rattache a un conducteur.
	Il est editable et deplacable par l'utilisateur.
	Il peut egalement etre oriente a un angle quelconque.
	Ses deplacements sont toutefois limites a une certaine distance autour de
	son conducteur parent.
*/
class ConductorTextItem : public DiagramTextItem {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	ConductorTextItem(Conductor * = 0, Diagram * = 0);
	ConductorTextItem(const QString &, Conductor * = 0, Diagram * = 0);
	virtual ~ConductorTextItem();
	private:
	ConductorTextItem(const ConductorTextItem &);
	
	// attributs
	public:
	enum { Type = UserType + 1006 };
	Conductor *parentConductor() const;
	virtual void fromXml(const QDomElement &);
	virtual QDomElement toXml(QDomDocument &) const;
	
	// methodes
	public:
	virtual int type() const { return Type; }
	virtual bool wasMovedByUser() const;
	virtual void forceMovedByUser(bool);
	
	protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
	
	// attributs
	private:
	Conductor *parent_conductor_;
	bool moved_by_user_;
	QPointF before_mov_pos_;
	bool first_move_;
	QPointF mouse_to_origin_movement_;
};
#endif
