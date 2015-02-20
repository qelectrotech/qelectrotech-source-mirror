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
#ifndef ELEMENT_DELETER_H
#define ELEMENT_DELETER_H
#include "elementscategory.h"
#include "elementslocation.h"
#include <QtGui>
/**
	This class provides an abstract way to delete an element from its parent
	collection. Especially, it requires a confirmation from users.
*/
class ElementDeleter : public QWidget {
	Q_OBJECT
	// constructors, destructor
	public:
	ElementDeleter(const ElementsLocation &, QWidget * = 0);
	virtual ~ElementDeleter();
	private:
	ElementDeleter(const ElementsCategory &);
	
	// methods
	public slots:
	bool exec();
	
	// attributes
	private:
	ElementsCollectionItem *element;
};
#endif
