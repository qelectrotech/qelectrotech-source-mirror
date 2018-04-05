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
#ifndef CONDUCTOR_TEXT_ITEM_H
#define CONDUCTOR_TEXT_ITEM_H
#include "diagramtextitem.h"
class Conductor;
/**
	This class represents a text item attached to a parent conductor.
	It may be moved and edited by users.
	It may also be rotated to any angle.
	Its movements are however limited to a particular distance around its
	parent conductor.
*/
class ConductorTextItem : public DiagramTextItem
{
	Q_OBJECT
	
        // constructors, destructor
	public:
        ConductorTextItem(Conductor * = nullptr);
        ConductorTextItem(const QString &, Conductor * = nullptr);
        ~ConductorTextItem() override;
	private:
        ConductorTextItem(const ConductorTextItem &);

	public:
        enum { Type = UserType + 1006 };
        Conductor *parentConductor() const;
        void fromXml(const QDomElement &) override;
        int type() const override { return Type; }
        virtual bool wasMovedByUser() const;
        virtual bool wasRotateByUser() const;
        virtual void forceMovedByUser(bool);
        virtual void forceRotateByUser(bool);
        virtual void setPos(const QPointF &pos);
        virtual void setPos(qreal x, qreal y);
	
	protected:
        void mousePressEvent   (QGraphicsSceneMouseEvent *event) override;
        void mouseMoveEvent    (QGraphicsSceneMouseEvent *event) override;
        void mouseReleaseEvent (QGraphicsSceneMouseEvent *event) override;
        void hoverEnterEvent(QGraphicsSceneHoverEvent *) override;
        void hoverLeaveEvent(QGraphicsSceneHoverEvent *) override;
        void hoverMoveEvent(QGraphicsSceneHoverEvent *) override;

        // attributes
	private:
        Conductor *parent_conductor_;
        bool moved_by_user_;
        bool rotate_by_user_;
        QPointF before_mov_pos_;
};
#endif
