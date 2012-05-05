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
#ifndef ELEMENT_DEFINITION_H
#define ELEMENT_DEFINITION_H
#include <QtXml>
#include "elementscategory.h"
class ElementsCollection;
class MoveElementsHandler;
/**
	Cette classe abstraite represente une definition XML d'element,
	c'est-a-dire qu'elle definit l'interface a implementer pour acceder a la
	definition XML d'un element, que celle-ci proviennt d'un fichier *.elmt ou
	d'un fichier projet QET.
*/
class ElementDefinition : public ElementsCollectionItem {
	Q_OBJECT
	
	public:
	/**
		Constructeur
	*/
	ElementDefinition(ElementsCategory *category = 0, ElementsCollection *collection = 0) : ElementsCollectionItem(category), parent_category_(category), parent_collection_(collection) {};
	
	/**
		Destructeur
	*/
	virtual ~ElementDefinition() {};
	
	/**
		@return la definition XML de l'element
	*/
	virtual QDomElement xml() = 0;
	
	/**
		Change la definition XML de l'element
		@param xml_element Nouvelle definition XML de l'element
		@return true si l'operation s'est bien passee, false sinon
	*/
	virtual bool setXml(const QDomElement &xml_element) = 0;
	
	/**
		@return true si la definition n'est pas disponible
	*/
	virtual bool isNull() const = 0;
	
	virtual ElementsCategory *parentCategory();
	virtual QList<ElementsCategory *> parentCategories();
	virtual bool hasParentCategory();
	
	/**
		@return true si l'element est rattache a une collection d'elements
		Un element appartenant a une collection a forcement un chemin virtuel.
	*/
	virtual bool hasParentCollection();
	virtual bool isChildOf(ElementsCollectionItem *);
	
	/**
		@return la collection d'element a laquelle appartient cet element
	*/
	virtual ElementsCollection *parentCollection();
	
	virtual QETProject *project();
	virtual void setProject(QETProject *);
	/**
		@return le protocole utilise par la collection a laquelle appartient cet element
	*/
	virtual QString protocol();
	/**
		Ne fait rien.
	*/
	virtual void setProtocol(const QString &);
	
	/**
		@return le chemin virtuel complet de cet element (protocole + chemin)
	*/
	virtual QString fullVirtualPath();
	
	/**
		@return l'emplacement de cet element
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
	
	virtual bool isCollection()   const { return(false); } ///< @return toujours false
	virtual bool isRootCategory() const { return(false); } ///< @return toujours false
	virtual bool isCategory()     const { return(false); } ///< @return toujours false
	virtual bool isElement()      const { return(true ); } ///< @return toujours true
	virtual ElementsCollection *toCollection();
	virtual ElementsCategory *toCategory();
	virtual ElementsCategory *toPureCategory();
	virtual ElementDefinition *toElement();
	virtual bool equals(ElementDefinition &);
	virtual bool removeContent();
	virtual QDateTime modificationTime() const = 0;
	void copy(MoveElementsDescription *);
	void move(MoveElementsDescription *);
	
	// attributs
	private:
	ElementsCategory *parent_category_;
	ElementsCollection *parent_collection_;
};
#endif
