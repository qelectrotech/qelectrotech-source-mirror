/*
    Copyright 2006-2013 The QElectroTech Team
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
#ifndef TITLEBLOCK_SLASH_SPLITTED_HELPER_CELL_H
#define TITLEBLOCK_SLASH_SPLITTED_HELPER_CELL_H
#include "helpercell.h"

/**
	This class is a variant of HelperCell having the ability to display two
	labels, with a split line between them.
*/
class SplittedHelperCell : public HelperCell {
	Q_OBJECT
	public:
	SplittedHelperCell(QGraphicsItem * = 0);
	virtual ~SplittedHelperCell();
	private:
	SplittedHelperCell(const SplittedHelperCell &);
	
	// methods
	public:
	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget * = 0);
	
	// attributes
	QColor split_background_color; ///< Background color on the split side
	QColor split_foreground_color; ///< Text color on the split side
	QString split_label;           ///< Text displayed on the split side
	int split_size;                ///< Length of the split side
};

#endif
