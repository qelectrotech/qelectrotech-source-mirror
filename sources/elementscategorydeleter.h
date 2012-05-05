/*
	Copyright 2006-2012 Xavier Guerrin
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
	Cette classe represente une couche d'abstraction pour supprimer
	une categorie d'elements et les elements qu'elle contient.
	Si la categorie racine d'une collection est fournie, elle sera
	videe apres un avertissement.
	Cette classe demande toujours confirmation a l'utilisateur par deux fois.
*/
class ElementsCategoryDeleter : public QWidget {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	ElementsCategoryDeleter(const ElementsLocation &, QWidget * = 0);
	virtual ~ElementsCategoryDeleter();
	private:
	ElementsCategoryDeleter(const ElementsCategory &);
	
	// methodes
	public slots:
	bool exec();
	
	// attributs
	private:
	ElementsCollectionItem *category;
};
#endif
