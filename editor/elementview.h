/*
	Copyright 2006-2007 Xavier Guerrin
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
#ifndef ELEMENT_VIEW_H
#define ELEMENT_VIEW_H
#include <QGraphicsView>
#include "elementscene.h"
/**
	Cette classe represente un widget permettant de visualiser une
	ElementScene, c'est-a-dire la classe d'edition des elements.
*/
class ElementView : public QGraphicsView {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	ElementView(ElementScene *, QWidget * = 0);
	virtual ~ElementView();
	
	private:
	ElementView(const ElementView &);
	
	// methodes
	public:
	ElementScene *scene() const;
	void setScene(ElementScene *);
	protected:
	bool event(QEvent *);
	
	//attributs
	private:
	ElementScene *scene_;
};
#endif
