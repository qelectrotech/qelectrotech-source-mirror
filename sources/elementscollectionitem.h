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
#ifndef ELEMENTS_COLLECTION_ITEM_H
#define ELEMENTS_COLLECTION_ITEM_H
#include <QtCore>
#include "elementslocation.h"
class ElementsCollection;
class ElementsCategory;
class ElementDefinition;
class MoveElementsHandler;
/**
	Cette interface est la classe mere pour toutes les classes
	modelisant une partie d'une collection d'elements.
*/
class ElementsCollectionItem : public QObject {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	ElementsCollectionItem(ElementsCollectionItem *parent = 0) : QObject(parent) {};
	virtual ~ElementsCollectionItem() {};
	
	private:
	ElementsCollectionItem(const ElementsCollectionItem &);
	
	// methodes
	public:
	/// @return true si l'item est une collection d'elements
	virtual bool isCollection() const = 0;
	/// @return true si l'item est une categorie d'elements
	virtual bool isCategory() const = 0;
	/// @return true si l'item est la categorie racine de sa collection
	virtual bool isRootCategory() const = 0;
	/// @return true si l'item est un element
	virtual bool isElement() const = 0;
	
	/**
		@return un pointeur sur cet item en tant que collection, ou 0 si cet item n'est pas une collection
	*/
	virtual ElementsCollection *toCollection() = 0;
	/**
		@return un pointeur sur cet item en tant que categorie.
		Si cet objet est une collection, un pointeur valide vers sa categorie racine sera renvoye.
		Si cet objet est une categorie, un pointeur valide sera renvoye.
		Si cet objet est un element, un pointeur valide vers sa categorie parente sera renvoye.
	*/
	virtual ElementsCategory *toCategory() = 0;
	/**
		@return un pointeur sur cet item en tant que categorie si et seulement
		si cet objet est une categorie non racine.
		Si cet objet est une collection, 0 sera retourne
		Si cet objet est une categorie, un pointeur valide sera renvoye.
		Si cet objet est un element, 0 sera retourne.
	*/
	virtual ElementsCategory *toPureCategory() = 0;
	/**
		@return un pointeur sur cet item en tant qu'element
	*/
	virtual ElementDefinition *toElement() = 0;
	
	/**
		Copie l'item vers la destination indiquee en parametre ; le booleen
		doit etre a true pour une copie recursive, a false sinon.
	*/
	virtual ElementsCollectionItem *copy(ElementsCategory *, MoveElementsHandler *, bool = true) = 0;
	
	/**
		Deplace l'item vers la destination indiquee en parametre.
	*/
	virtual ElementsCollectionItem *move(ElementsCategory *, MoveElementsHandler *) = 0;
	
	/// Recharge l'item
	virtual void reload() = 0;
	/// @return true si l'item existe
	virtual bool exists() = 0;
	/// @return true si l'item est lisible
	virtual bool isReadable() = 0;
	/// @return true si l'item est accessible en ecriture
	virtual bool isWritable() = 0;
	/**
		supprime le contenu de l'item (categories et elements) sans supprimer
		l'item lui-meme
		@return true si l'operation a reussi, false sinon
	*/
	virtual bool removeContent() = 0;
	/**
		supprime le contenu de l'item (categories et elements) puis l'item
		lui-meme
		@return true si l'operation a reussi, false sinon
	*/
	virtual bool remove() = 0;
	/**
		Enregistre l'item
		@return true si l'operation a reussi, false sinon
	*/
	virtual bool write() = 0;
	/**
		@return le projet auquel appartient cet item
	*/
	virtual QETProject *project() = 0;
	/**
		Definit le projet auquel appartient cet item
	*/
	virtual void setProject(QETProject *) = 0;
	/**
		@return le protocole utilise pour acceder a la collection de cet item
	*/
	virtual QString protocol() = 0;
	/**
		Definit le protocole a utiliser pour acceder a la collection de cet item
	*/
	virtual void setProtocol(const QString &) = 0;
	/**
		@return la categorie parente de cet item, ou 0 si celui-ci n'en possede pas
	*/
	virtual ElementsCategory *parentCategory() = 0;
	/**
		@return la liste des categories parentes de cet item
	*/
	virtual QList<ElementsCategory *> parentCategories() = 0;
	/**
		@return true si cet item possede une categorie parente, false sinon
	*/
	virtual bool hasParentCategory() = 0;
	/**
		@return la collection parente de cet item, ou 0 si celui-ci n'en possede pas
	*/
	virtual ElementsCollection *parentCollection() = 0;
	/**
		@return true si cet item possede une collection parente, false sinon
	*/
	virtual bool hasParentCollection() = 0;
	/**
		@param other_item Autre item
		@return true si other_item est parent (direct ou indirect) de other_item, false sinon
	*/
	virtual bool isChildOf(ElementsCollectionItem *other_item) = 0;
	/**
		@return le nom de cet item dans l'arborescence
	*/
	virtual QString pathName() const = 0;
	/**
		@return le chemin virtuel vers cet item dans l'arborescence, sans le protocole
	*/
	virtual QString virtualPath() = 0;
	/**
		@return le chemin virtuel vers cet item dans l'arborescence, avec le protocole
	*/
	virtual QString fullVirtualPath() = 0;
	/**
		@return l'emplacement de cet item
	*/
	virtual ElementsLocation location() = 0;
	/**
		@return true si cet item est represente quelque part sur le systeme de fichiers
	*/
	virtual bool hasFilePath() = 0;
	/**
		@return le chemin de cet item sur le systeme de fichiers
	*/
	virtual QString filePath() = 0;
	/**
		Definit le chemin de cet item sur le systeme de fichiers
	*/
	virtual void setFilePath(const QString &) = 0;
	/**
		@return la liste des categories d'elements contenues dans cet item
	*/
	virtual QList<ElementsCategory *> categories() = 0;
	/**
		@return une categorie a partir de son chemin virtuel
	*/
	virtual ElementsCategory *category(const QString &) = 0;
	/**
		@return une nouvelle categorie creee a partir d'un chemin virtuel
	*/
	virtual ElementsCategory *createCategory(const QString &) = 0;
	/**
		@return la liste des elements contenus dans cet item
	*/
	virtual QList<ElementDefinition *> elements() = 0;
	/**
		@return un element a partir de son chemin virtuel
	*/
	virtual ElementDefinition *element(const QString &) = 0;
	/**
		@return un nouvel element cree a partir d'un chemin virtuel
	*/
	virtual ElementDefinition *createElement(const QString &) = 0;
	/**
		@return true if the item is empty, false otherwise
	*/
	virtual bool isEmpty() = 0;
	/**
		@return the count of categories and elements within this item
	*/
	virtual int count() = 0;
};
#endif
