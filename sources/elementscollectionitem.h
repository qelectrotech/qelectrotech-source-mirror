/*
	Copyright 2006-2013 The QElectroTech Team
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
#ifndef ELEMENTS_COLLECTION_ITEM_H
#define ELEMENTS_COLLECTION_ITEM_H
#include <QtCore>
#include "elementslocation.h"
class ElementsCollection;
class ElementsCategory;
class ElementDefinition;
class MoveElementsHandler;
/**
	This interface is the base class for all classes representing a part of an
	elements collection.
*/
class ElementsCollectionItem : public QObject {
	Q_OBJECT
	
	// constructors, destructor
	public:
	ElementsCollectionItem(ElementsCollectionItem *parent = 0) : QObject(parent) {};
	virtual ~ElementsCollectionItem() {};
	
	private:
	ElementsCollectionItem(const ElementsCollectionItem &);
	
	// methods
	public:
	/// @return whether the item is an elements collection
	virtual bool isCollection() const = 0;
	/// @return whether the item is an elements category
	virtual bool isCategory() const = 0;
	/// @return whether the item is the root category of a collection
	virtual bool isRootCategory() const = 0;
	/// @return whether the item is an element
	virtual bool isElement() const = 0;
	
	/**
		@return a pointer to this item as a collection object, or 0 if this item is not a collection.
	*/
	virtual ElementsCollection *toCollection() = 0;
	/**
		@return a pointer to this item as a category object.
		If this item is a collection, return a valid pointer to its root category.
		If this item is a category, return a valid pointer.
		If this item is an element, return a valid pointer to its parent category.
	*/
	virtual ElementsCategory *toCategory() = 0;
	/**
		@return a pointer to this item as a category if and only if this object is a non-root elements category.
		If this item is a collection, return 0.
		If this item is a category, return a valid pointer.
		If this item is an element, return 0.
	*/
	virtual ElementsCategory *toPureCategory() = 0;
	/**
		@return a pointer to this item as an element object.
	*/
	virtual ElementDefinition *toElement() = 0;
	
	/**
		Copy this item to the specified target, using a specific strategy to handle
		corner cases (errors, conflicts, confirmations, ...), recursively or not.
	*/
	virtual ElementsCollectionItem *copy(ElementsCategory *, MoveElementsHandler *, bool = true) = 0;
	
	/**
		Move this item to the specified destination, using a specific strategy to
		handle corner cases (errors, conflicts, confirmations, ...).
	*/
	virtual ElementsCollectionItem *move(ElementsCategory *, MoveElementsHandler *) = 0;
	
	/// Reload this item
	virtual void reload() = 0;
	/// @return whether the item really exists
	virtual bool exists() = 0;
	/// @return whether the item is readable
	virtual bool isReadable() = 0;
	/// @return whether the item is writable
	virtual bool isWritable() = 0;
	/**
		Delete the item content (elements and subcategories) without deleting the item itself.
		@return true if the operation succeeded, false otherwise
	*/
	virtual bool removeContent() = 0;
	/**
		Delete the item content (elements and categories) before deleting the item itself.
		@return true if the operation succeeded, false otherwise
	*/
	virtual bool remove() = 0;
	/**
		Save the item.
		@return true if the operation succeeded, false otherwise
	*/
	virtual bool write() = 0;
	/**
		@return the project this item belongs to
	*/
	virtual QETProject *project() = 0;
	/**
		Set the project this item belongs to.
	*/
	virtual void setProject(QETProject *) = 0;
	/**
		@return the protocol used to access the collection this item belongs to.
	*/
	virtual QString protocol() = 0;
	/**
		Set the protocol used to access the collection this item belongs to.
	*/
	virtual void setProtocol(const QString &) = 0;
	/**
		@return the parent category of this item, or 0 if it does not have any
	*/
	virtual ElementsCategory *parentCategory() = 0;
	/**
		@return the list of parent categories of this item
	*/
	virtual QList<ElementsCategory *> parentCategories() = 0;
	/**
		@return whether this item has a parent category
	*/
	virtual bool hasParentCategory() = 0;
	/**
		@return the parent collection of this item, or 0 if it does not have any
	*/
	virtual ElementsCollection *parentCollection() = 0;
	/**
		@return whether this item belongs to an elements collection
	*/
	virtual bool hasParentCollection() = 0;
	/**
		@param other_item other item
		@return whether \a other_item is a direct or indirect parent of this item.
	*/
	virtual bool isChildOf(ElementsCollectionItem *other_item) = 0;
	/**
		@return the name of this item within the elements tree.
	*/
	virtual QString pathName() const = 0;
	/**
		@return the virtual path to this item within the elements tree, protocol excluded.
	*/
	virtual QString virtualPath() = 0;
	/**
		@return the virtual path to this item within the elements tree, protocol included.
	*/
	virtual QString fullVirtualPath() = 0;
	/**
		@return the location of this item
	*/
	virtual ElementsLocation location() = 0;
	/**
		@return whether this item is stored somewhere on the filesystem.
	*/
	virtual bool hasFilePath() = 0;
	/**
		@return the path of this item within the filesystem
	*/
	virtual QString filePath() = 0;
	/**
		Set the path to this item within the filesystem.
	*/
	virtual void setFilePath(const QString &) = 0;
	/**
		@return the list of direct child elements category within this item
	*/
	virtual QList<ElementsCategory *> categories() = 0;
	/**
		@return the specified category, provided its virtual path
	*/
	virtual ElementsCategory *category(const QString &) = 0;
	/**
		@return a new category, created from the spcified virtual path
	*/
	virtual ElementsCategory *createCategory(const QString &) = 0;
	/**
		@return the list of child elements within this item
	*/
	virtual QList<ElementDefinition *> elements() = 0;
	/**
		@return an element, provided its virtual path
	*/
	virtual ElementDefinition *element(const QString &) = 0;
	/**
		@return a new element, created from the specified virtual path
	*/
	virtual ElementDefinition *createElement(const QString &) = 0;
	/**
		@return whether the item is empty
	*/
	virtual bool isEmpty() = 0;
	/**
		@return the count of categories and elements within this item
	*/
	virtual int count() = 0;
};
#endif
