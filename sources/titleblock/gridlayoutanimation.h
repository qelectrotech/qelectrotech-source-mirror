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
#ifndef TITLEBLOCK_SLASH_GRID_LAYOUT_ANIMATION_H
#define TITLEBLOCK_SLASH_GRID_LAYOUT_ANIMATION_H
#include <QtWidgets>

/**
	This class allows animating a dimension change for a QGraphicsGridLayout
	row or column.
*/
class GridLayoutAnimation : public QVariantAnimation {
	// Constructors, destructor
	public:
	GridLayoutAnimation(QGraphicsGridLayout * = 0, QObject * = 0);
	virtual ~GridLayoutAnimation();
	
	// methods
	public:
	QGraphicsGridLayout *grid();
	void setGrid(QGraphicsGridLayout *);
	int index() const;
	void setIndex(int);
	bool actsOnRows() const;
	void setActsOnRows(bool);
	
	protected:
	void updateCurrentValue(const QVariant &);
	
	// attributes
	private:
	QGraphicsGridLayout *grid_; ///< Grid this class will animate
	bool row_;                  ///< Whether we should animate a row or a column
	int index_;                 ///< Index of the row/column to be animated
};
#endif
