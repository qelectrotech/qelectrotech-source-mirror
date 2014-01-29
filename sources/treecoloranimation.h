/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef TREE_COLOR_ANIMATION_H
#define TREE_COLOR_ANIMATION_H
#include <QtGui>

/**
	This class allows animating a background color change for a
	set of QTreeWidgetItem.
*/
class TreeColorAnimation : public QVariantAnimation {
	// Constructors, destructor
	public:
	TreeColorAnimation(const QList<QTreeWidgetItem *> &items, QObject * = 0);
	virtual ~TreeColorAnimation();
	
	// methods
	public:
	QList<QTreeWidgetItem *> items() const;
	
	protected:
	void updateCurrentValue(const QVariant &);
	
	// attributes
	private:
	QList<QTreeWidgetItem *> items_; ///< Items this object will animate
};
#endif
