/*
        Copyright 2006-2019 QElectroTech Team
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
#ifndef QETGRAPHICSHEADERITEM_H
#define QETGRAPHICSHEADERITEM_H

#include "qetapp.h"

#include <QGraphicsObject>
#include <QFont>
#include <QMargins>

class QAbstractItemModel;

class QetGraphicsHeaderItem : public QGraphicsObject
{
    public:
        QetGraphicsHeaderItem(QGraphicsItem *parent = nullptr);

		enum { Type = UserType + 1301 };
		int type() const override { return Type; }

        void setModel(QAbstractItemModel *model);
        QAbstractItemModel *model() const;
        void reset();

        virtual QRectF boundingRect() const override;
        virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
		QRect rect() const;
		void resizeSection(int logicalIndex, int size);
		int sectionSize(int logical_index) const;

	private:
		void setUpMinimumSectionsSize();
		void setUpBoundingRect();

    private:
        QRectF m_bounding_rect;
        QAbstractItemModel *m_model = nullptr;
        QFont m_font = QETApp::diagramTextsFont();
		QMargins m_margin;
		QVector<int> m_sections_minimum_width,
					 m_current_sections_width;
		int m_section_height=1;
		QRect m_minimum_rect,
			  m_current_rect;
};

#endif // QETGRAPHICSHEADERITEM_H
