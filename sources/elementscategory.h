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
#ifndef ELEMENTS_CATEGORY_H
#define ELEMENTS_CATEGORY_H
#include "elementscollectionitem.h"
#include "nameslist.h"
#include "elementslocation.h"
class ElementDefinition;
class ElementsCollection;
class MoveElementsHandler;
class MoveElementsDescription;
/**
	This abstract class represents an elements category, i.e. a kind of
	subfolder within elements collections.
*/
class ElementsCategory : public ElementsCollectionItem {
	Q_OBJECT
	
	// constructors, destructor
	public:
	ElementsCategory(ElementsCategory * = 0, ElementsCollection * = 0);
	virtual ~ElementsCategory();
	
	private:
	ElementsCategory(const ElementsCategory &);
	
	// Implementations of pure virtual methods from parent classes
	public:
	virtual QETProject *project();
	virtual void setProject(QETProject *);
	virtual QString protocol();
	virtual void setProtocol(const QString &);
	virtual ElementsCategory *parentCategory();
	virtual QList<ElementsCategory *> parentCategories();
	virtual bool hasParentCategory();
	virtual ElementsCollection *parentCollection();
	virtual bool hasParentCollection();
	virtual bool isChildOf(ElementsCollectionItem *);
	virtual QString fullVirtualPath();
	virtual ElementsLocation location();
	virtual bool isRootCategory() const;
	virtual bool isCollection() const;
	virtual bool isCategory() const;
	virtual bool isElement() const;
	virtual ElementsCollection *toCollection();
	virtual ElementsCategory *toCategory();
	virtual ElementsCategory *toPureCategory();
	virtual ElementDefinition *toElement();
	virtual ElementsCollectionItem *copy(ElementsCategory *, MoveElementsHandler *, bool = true);
	virtual ElementsCollectionItem *move(ElementsCategory *, MoveElementsHandler *);
	virtual void deleteUnusedElements(MoveElementsHandler *handler);
	virtual void deleteEmptyCategories(MoveElementsHandler *handler);
	virtual bool isEmpty();
	virtual int count();
	
	// Methods specific to the ElementsCategory class
	public:
	virtual QString name() const;
	virtual NamesList categoryNames() const;
	virtual void clearNames();
	virtual void addName(const QString &, const QString &);
	virtual void setNames(const NamesList &);
	void copy(MoveElementsDescription *);
	void move(MoveElementsDescription *);
	
	// attributes
	protected:
	/// parent collection
	ElementsCollection *parent_collection_;
	/// parent category
	ElementsCategory   *parent_category_;
	/// names list for this category
	NamesList           category_names;
};
#endif
