/*
    Copyright 2006-2015 The QElectroTech Team
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
#ifndef TITLEBLOCK_SLASH_HELPER_CELL_H
#define TITLEBLOCK_SLASH_HELPER_CELL_H
#include <QtGui>
#include "qet.h"

/**
	This class implements a helper widget for cells that indicate the length of
	columns and rows.
*/
class HelperCell : public QGraphicsObject, public QGraphicsLayoutItem {
	Q_OBJECT
	Q_INTERFACES(QGraphicsLayoutItem)
	
	// constructor, destructor
	public:
	HelperCell(QGraphicsItem * = 0);
	virtual ~HelperCell();
	private:
	HelperCell(const HelperCell &);
	
	// attributes
	public:
	QColor background_color;     ///< Background color when rendering this cell
	QColor foreground_color;     ///< Text color when rendering this cell
	QString label;               ///< Label displayed in this cell
	Qt::Orientation orientation; ///< Orientation of this cell
	int index;                   ///< Index of this cell
	
	// methods
	public:
	virtual void setGeometry(const QRectF &);
	virtual QSizeF sizeHint(Qt::SizeHint, const QSizeF & = QSizeF()) const;
	virtual QRectF boundingRect() const;
	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0);
	virtual void setType(QET::TitleBlockColumnLength);
	virtual void setActions(const QList<QAction *> &);
	virtual QList<QAction *> actions() const;
	virtual void setLabel(const QString &text, bool = true);
	
	protected:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent *);
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *);
	
	signals:
	void contextMenuTriggered(HelperCell *);
	void doubleClicked(HelperCell *);
	
	private:
	QList<QAction *> actions_; ///< List of actions displayed by the context menu
};
#endif
