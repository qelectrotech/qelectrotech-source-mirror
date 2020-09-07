/*
	Copyright 2006-2020 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef QETGRAPHICSHEADERITEM_H
#define QETGRAPHICSHEADERITEM_H

#include "qetapp.h"

#include <QGraphicsObject>
#include <QFont>
#include <QMargins>

class QAbstractItemModel;

/**
	@brief The QetGraphicsHeaderItem class
	The header have a few parameters to edit her visual aspect.
	Margins, to edit the margin between the cell and the text.
	Text font.
	Text alignment in the cell
	These three parameters are not settable directly with the header but trough the model to be displayed by the header.
	Header search these parameters only in the section 0 for cell of header.
	By consequence, set data in other section is useless also these parameter can't be set individually for each cell.
	The margins is stored in the model in index Qt::UserRole+1 and for value a QString. See QETUtils::marginsFromString and  QETUtils::marginsToString
*/
class QetGraphicsHeaderItem : public QGraphicsObject
{
	Q_OBJECT

	public:
		QetGraphicsHeaderItem(QGraphicsItem *parent = nullptr);

		enum { Type = UserType + 1301 };
		int type() const override { return Type; }

		void setModel(QAbstractItemModel *model = nullptr);
		QAbstractItemModel *model() const;

		virtual QRectF boundingRect() const override;
		virtual void paint(
				QPainter *painter,
				const QStyleOptionGraphicsItem *option,
				QWidget *widget) override;
		QRect rect() const;
		void resizeSection(int logicalIndex, int size);
		int sectionSize(int logical_index) const;
		QVector<int> minimumSectionWidth()  const
		{return m_sections_minimum_width;}
		int minimumWidth() const
		{return m_minimum_width;}

		QDomElement toXml (QDomDocument &document) const;
		void fromXml(const QDomElement &element);
		static QString xmlTagName() {return QString("graphics_header");}
		virtual bool toDXF (const QString &filepath);

	signals:
		void sectionResized(int logicalIndex, int size);
		void heightResized();

	private:
		void setUpMinimumSectionsSize();
		void setUpBoundingRect();
		void headerDataChanged(
				Qt::Orientations orientation,
				int first,
				int last);
		void adjustSize();
		void modelReseted();

		QRect m_current_rect;
		QRectF m_bounding_rect;
		QAbstractItemModel *m_model = nullptr;

		QVector<int>
		m_sections_minimum_width,
		m_current_sections_width;

		int
		m_section_height=1,
		m_minimum_section_height=1,
		m_minimum_width=1;
};

#endif // QETGRAPHICSHEADERITEM_H
