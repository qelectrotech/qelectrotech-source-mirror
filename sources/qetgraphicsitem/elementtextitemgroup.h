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
#ifndef ELEMENTTEXTITEMGROUP_H
#define ELEMENTTEXTITEMGROUP_H

#include <QGraphicsItemGroup>
#include <QObject>
#include <QDomElement>

class Element;
class DynamicElementTextItem;
class Diagram;

/**
 * @brief The ElementTextItemGroup class
 * This class represent a group of element text
 * Texts in the group can be aligned left / center /right
 */
class ElementTextItemGroup : public QObject, public  QGraphicsItemGroup
{
	Q_OBJECT
	
	Q_PROPERTY(QPointF pos READ pos WRITE setPos)
	Q_PROPERTY(qreal rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
	Q_PROPERTY(int verticalAdjustment READ verticalAdjustment WRITE setVerticalAdjustment NOTIFY verticalAdjustmentChanged)
	Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged)
	
	public:
	signals:
		void rotationChanged(qreal);
		void verticalAdjustmentChanged(int);
		void alignmentChanged(Qt::Alignment);
	
	public:
		ElementTextItemGroup(const QString &name, Element *parent);
		~ElementTextItemGroup() override;
		void addToGroup(QGraphicsItem *item);
		void removeFromGroup(QGraphicsItem *item);
		
		void setAlignment(Qt::Alignment alignement);
		Qt::Alignment alignment() const;
		void updateAlignment();
		int verticalAdjustment() const {return m_vertical_adjustment;}
		void setVerticalAdjustment(int v);
		void setName(QString name);
		QString name() const {return m_name;}
		QList<DynamicElementTextItem *> texts() const;
		Diagram *diagram() const;
		Element *parentElement() const;
		
		QDomElement toXml(QDomDocument &dom_document) const;
		void fromXml(QDomElement &dom_element);
		static QString xmlTaggName() {return QString("text_group");}
		
		void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
		QRectF boundingRect() const override;
		void setRotation(qreal angle);
		
	protected:
		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
		void keyPressEvent(QKeyEvent *event) override;

	private:
		Qt::Alignment m_alignment = Qt::AlignJustify;
		QString m_name;
		bool m_first_move = true;
		QPointF m_mouse_to_origin_movement;
		int m_vertical_adjustment = 0;
};

#endif // ELEMENTTEXTITEMGROUP_H
