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
#include "treecoloranimation.h"

/**
	Constructor
	@param items List of items whose background color will be animated.
	@param parent Parent QObject
*/
TreeColorAnimation::TreeColorAnimation(const QList<QTreeWidgetItem *> &items, QObject *parent) : 
	QVariantAnimation(parent),
	items_(items)
{
}

/**
	Destructor
*/
TreeColorAnimation::~TreeColorAnimation() {
}

/**
	@return the list of items whose background color will be animated.
*/
QList<QTreeWidgetItem *> TreeColorAnimation::items() const {
	return(items_);
}

/**
	Apply the provided color to animated items.
	@param color Color to be applied on animated items.
*/
void TreeColorAnimation::updateCurrentValue(const QVariant &color) {
	foreach (QTreeWidgetItem *item, items_) {
		item -> setBackgroundColor(0, color.value<QColor>());
	}
}
