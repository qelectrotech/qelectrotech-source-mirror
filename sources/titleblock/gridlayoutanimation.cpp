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
#include "gridlayoutanimation.h"

/**
	Constructor
	@param grid Grid to be animated
	@param parent Parent QObject
*/
GridLayoutAnimation::GridLayoutAnimation(QGraphicsGridLayout *grid, QObject *parent) :
	QVariantAnimation(parent),
	grid_(grid)
{
}

/**
	Destructor
*/
GridLayoutAnimation::~GridLayoutAnimation() {
}

/**
	@return the animated grid
*/
QGraphicsGridLayout *GridLayoutAnimation::grid() {
	return(grid_);
}

/**
	@param grid The grid to be animated
*/
void GridLayoutAnimation::setGrid(QGraphicsGridLayout *grid) {
	grid_ = grid;
}

/**
	@return the index of the row/column to be animated
*/
int GridLayoutAnimation::index() const {
	return(index_);
}

/**
	@param index the index of the row/column to be animated
*/
void GridLayoutAnimation::setIndex(int index) {
	index_ = index;
}

/**
	@return true if this object acts on a row, false if it acts on a column.
*/
bool GridLayoutAnimation::actsOnRows() const {
	return(row_);
}

/**
	@param acts_on_row true for this object to act on a row, false for it to
	act on a column.
*/
void GridLayoutAnimation::setActsOnRows(bool acts_on_row) {
	row_ = acts_on_row;
}

/**
	Implementation of QVariantAnimation::updateCurrentValue().
*/
void GridLayoutAnimation::updateCurrentValue(const QVariant &value) {
	if (!grid_) return;
	if (row_) {
		grid_ -> setRowFixedHeight(index_, value.toReal());
	} else {
		grid_ -> setColumnFixedWidth(index_, value.toReal());
	}
}
