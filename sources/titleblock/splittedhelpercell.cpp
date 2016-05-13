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
#include "splittedhelpercell.h"

/**
	Constructor
	@param parent Parent QGraphicsItem
*/
SplittedHelperCell::SplittedHelperCell(QGraphicsItem *parent) :
	HelperCell(parent),
	split_background_color(background_color),
	split_foreground_color(foreground_color),
	split_size(0)
{
}

/**
	Destructor
*/
SplittedHelperCell::~SplittedHelperCell() {
}

/**
	Handles the splitted helper cell visual rendering
	@param painter QPainter to be used for the rendering
	@param option Rendering options
	@param widget QWidget being painted, if any
*/
void SplittedHelperCell::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * widget) {
	if (!split_size) {
		HelperCell::paint(painter, option, widget);
		return;
	}
	QRectF first_drawing_rectangle(QPointF(0, 0), geometry().adjusted(0, 0, -split_size, 0).size());
	QRectF second_drawing_rectangle(first_drawing_rectangle.topRight(), QSize(split_size, first_drawing_rectangle.height()));
	
	painter -> setPen(Qt::black);
	painter -> setBrush(background_color);
	painter -> drawRect(first_drawing_rectangle);
	painter -> setBrush(split_background_color);
	painter -> drawRect(second_drawing_rectangle);
	
	painter -> setPen(foreground_color);
	painter -> drawText(first_drawing_rectangle, Qt::AlignHCenter | Qt::AlignVCenter, label);
	painter -> setPen(split_foreground_color);
	painter -> drawText(second_drawing_rectangle, Qt::AlignHCenter | Qt::AlignVCenter, split_label);
}
