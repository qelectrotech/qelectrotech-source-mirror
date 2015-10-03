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
#ifndef ELEMENT_DEFINITION_H
#define ELEMENT_DEFINITION_H
#include <QtXml>
#include "elementscategory.h"
class ElementsCollection;
class MoveElementsHandler;
/**
	This abstract class represents a way to access the XML definition of an
	element, be it from a .elmt file or from a QET project file.
*/
class ElementDefinition : public ElementsCollectionItem {
	Q_OBJECT
	
	public:
	/**
		Constructor
	*/
	ElementDefinition(ElementsCategory *category = 0, ElementsCollection *collection = 0) :
		ElementsCollectionItem(category),
		parent_category_(category),
		parent_collection_(collection)
	{};

	virtual ~ElementDefinition() {};
	
	/**
		@return the XML definition of a particular element
	*/
	virtual QDomElement xml() = 0;
	virtual QUuid uuid();
	
	/**
		Specify the XML definition of a particular element
		@param xml_element New XML definition
		@return true if everything went well, false otherwise
	*/
	virtual bool setXml(const QDomElement &xml_element) = 0;
	
	/**
		@return true if the definition is not available
	*/
	virtual bool isNull() const = 0;
	
	virtual ElementsCategory *parentCategory();
	virtual QList<ElementsCategory *> parentCategories();
	virtual bool hasParentCategory();
	
	/**
		@return whether the element is attached to an elements collection
		An elemet which belongs to a collection always has a virtual path.
	*/
	virtual bool hasParentCollection();
	virtual bool isChildOf(ElementsCollectionItem *);
	
	/**
		@return the elements collections this element belongs to
	*/
	virtual ElementsCollection *parentCollection();
	
	virtual QETProject *project();
	virtual void setProject(QETProject *);
	/**
		@return the "protocol" used by the parent collection
	*/
	virtual QString protocol();
	/**
		Has no effect.
	*/
	virtual void setProtocol(const QString &);
	
	/**
		@return the full virtual path for this element (i.e. "protocol" + path)
	*/
	virtual QString fullVirtualPath();
	
	/**
		@return the location of this element, as an ElementsLocation object.
		@see ElementsLocation
	*/
	virtual ElementsLocation location();
	
	virtual QList<ElementsCategory *> categories();
	virtual ElementsCategory *category(const QString &);
	virtual ElementsCategory *createCategory(const QString &);
	
	virtual QList<ElementDefinition *> elements();
	virtual ElementDefinition *element(const QString &);
	virtual ElementDefinition *createElement(const QString &);
	virtual bool isEmpty();
	virtual int count();
	virtual ElementsCollectionItem *copy(ElementsCategory *, MoveElementsHandler *, bool = true);
	virtual ElementsCollectionItem *move(ElementsCategory *, MoveElementsHandler *);
	
	virtual bool isCollection()   const { return(false); } ///< @return always false
	virtual bool isRootCategory() const { return(false); } ///< @return always false
	virtual bool isCategory()     const { return(false); } ///< @return always false
	virtual bool isElement()      const { return(true ); } ///< @return always  true
	virtual ElementsCollection *toCollection();
	virtual ElementsCategory *toCategory();
	virtual ElementsCategory *toPureCategory();
	virtual ElementDefinition *toElement();
	virtual bool equals(ElementDefinition &);
	virtual bool removeContent();
	virtual QDateTime modificationTime() const = 0;
	void copy(MoveElementsDescription *);
	void move(MoveElementsDescription *);
	
	// attributes
	private:
	ElementsCategory *parent_category_;
	ElementsCollection *parent_collection_;
	QUuid m_uuid;
};
#endif
