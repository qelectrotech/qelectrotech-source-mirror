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
#ifndef QETGRAPHICSHANDLERITEM_H
#define QETGRAPHICSHANDLERITEM_H

#include <QGraphicsItem>

/**
 * @brief The QetGraphicsHandlerItem class
 * This graphics item represents a point, destined to be used as an handler,
 * for modifie the geometrie of a another graphics item (like shapes).
 * The graphics item to be modified, must call "installSceneEventFilter" of this item with itself for argument,.
 * The ghraphics item to be modified, need to reimplement "sceneEventFilter" for create the modification behavior.
 */
class QetGraphicsHandlerItem : public QGraphicsItem
{
    public:
        QetGraphicsHandlerItem(qreal size = 15);
        QRectF boundingRect() const override;
		
		enum { Type = UserType + 1200};
		int type() const override {return Type;}
		
		void setColor(QColor color);

    protected:
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    private:
        qreal m_size,
              m_previous_zoom_factor = 1;
		QColor m_color;
		
	public:
		static QVector<QetGraphicsHandlerItem *> handlerForPoint(const QVector<QPointF> &points, int size = 15);
};

#endif // QETGRAPHICSHANDLERITEM_H
