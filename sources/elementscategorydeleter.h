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
#ifndef ELEMENTS_CATEGORY_DELETER_H
#define ELEMENTS_CATEGORY_DELETER_H
#include "fileelementscategory.h"
#include "elementslocation.h"
#include <QtGui>
/**
	This class provide an abstract way to delete an elements category along with
	its child elements and subcategories. It always requires a double confirmation
	from users before actually proceeding to the deletion.
	If the deletion of a whole elements collection is required, this class will
	require an extra confirmation.
*/
class ElementsCategoryDeleter : public QWidget {
	Q_OBJECT
	// constructors, destructor
	public:
	ElementsCategoryDeleter(const ElementsLocation &, QWidget * = 0);
	virtual ~ElementsCategoryDeleter();
	private:
	ElementsCategoryDeleter(const ElementsCategory &);
	
	// methods
	public slots:
	bool exec();
	
	// attributes
	private:
	ElementsCollectionItem *category;
};
#endif
