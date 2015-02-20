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
#ifndef ELEMENTS_COLLECTION_H
#define ELEMENTS_COLLECTION_H
#include <QtCore>
#include <QIcon>
#include "elementscollectionitem.h"
class QETProject;
class ElementsCategory;
class ElementsCollectionCache;
class ElementDefinition;
class MoveElementsHandler;
/**
	This abstract class represents an elements collection. For instance, it may
	represent the collection provided along with QElectroTech, users custom
	collections or collections embedded within QET project files.
*/
class ElementsCollection : public ElementsCollectionItem {
	Q_OBJECT
	public:
	// constructors, destructor
	ElementsCollection(ElementsCollectionItem * = 0);
	virtual ~ElementsCollection();
	
	private:
	ElementsCollection(const ElementsCollection &);
	
	// Implementations of pure virtual methodes from parent classes
	public:
	virtual QString title() const;
	virtual void setTitle(const QString &);
	virtual QIcon icon() const;
	virtual void setIcon(const QIcon &);
	virtual bool isCollection() const;
	virtual bool isRootCategory() const;
	virtual bool isCategory()  const;
	virtual bool isElement() const;
	virtual ElementsCollection *toCollection();
	virtual ElementsCategory *toCategory();
	virtual ElementsCategory *toPureCategory();
	virtual ElementDefinition *toElement();
	virtual ElementsCollectionItem *copy(ElementsCategory *, MoveElementsHandler *, bool = true);
	virtual ElementsCollectionItem *move(ElementsCategory *, MoveElementsHandler *);
	virtual bool removeContent();
	virtual bool remove();
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
	virtual QString pathName() const;
	virtual QString virtualPath();
	virtual QString fullVirtualPath();
	virtual ElementsLocation location();
	virtual QList<ElementsCategory *> categories();
	virtual ElementsCategory *category(const QString &);
	virtual ElementsCategory *createCategory(const QString &);
	virtual QList<ElementDefinition *> elements();
	virtual ElementDefinition *element(const QString &);
	virtual ElementDefinition *createElement(const QString &);
	virtual bool isEmpty();
	virtual int count();
	
	// Methods specific to the ElementsCollection class
	public:
	/**
		@return the root category of this collection
	*/
	virtual ElementsCategory *rootCategory() = 0;
	virtual ElementsCollectionItem *item(const QString &, bool = true);
	virtual bool isCacheable() const = 0;
	virtual ElementsCollectionCache *cache() const;
	virtual void setCache(ElementsCollectionCache *);
	
	signals:
	void elementsCollectionChanged(ElementsCollection *);
	
	// attributes
	protected:
	/// Title to be used when referring to this collection
	QString title_;
	/// Icon to be displayed when representing this collection
	QIcon icon_;
	/// "Protocol" used to access this collection
	QString protocol_;
	/// Project this collection belongs to, if any
	QETProject *project_;
	/// Optional cache used to improve performance
	ElementsCollectionCache *cache_;
};
#endif
