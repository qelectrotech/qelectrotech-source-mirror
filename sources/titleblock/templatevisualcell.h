/*
    Copyright 2006-2016 The QElectroTech Team
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
#ifndef TITLEBLOCK_SLASH_QET_TEMPLATE_VISUAL_CELL_H
#define TITLEBLOCK_SLASH_QET_TEMPLATE_VISUAL_CELL_H
#include <QtWidgets>
#include "qet.h"
class TitleBlockTemplate;
#include "titleblockcell.h"

/**
	This class implements a preview widget for cells that compose a title
	block template.
*/
class TitleBlockTemplateVisualCell : public QGraphicsLayoutItem, public QGraphicsItem {
	// constructor, destructor
	public:
	TitleBlockTemplateVisualCell(QGraphicsItem * parent = 0);
	virtual ~TitleBlockTemplateVisualCell();
	private:
	TitleBlockTemplateVisualCell(const TitleBlockTemplateVisualCell &);
	
	// methods
	public:
	virtual void setGeometry(const QRectF &);
	virtual QSizeF sizeHint(Qt::SizeHint, const QSizeF & = QSizeF()) const;
	virtual QRectF boundingRect() const;
	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0);
	
	public slots:
	void setTemplateCell(TitleBlockTemplate *, TitleBlockCell *);
	TitleBlockTemplate *titleBlockTemplate() const;
	TitleBlockCell *cell() const;
	QSet<TitleBlockCell *> cells() const;
	
	// attributes
	private:
	TitleBlockTemplate *template_; ///< Title block template of the previewed cell
	TitleBlockCell *cell_;         ///< Previewed cell
};
#endif
