/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "elementscategory.h"
#include "elementscollection.h"
#include "elementdefinition.h"
#include "moveelementshandler.h"
#include "moveelementsdescription.h"
#include "qet.h"
#include "qetproject.h"

/**
	Constructeur
*/
ElementsCategory::ElementsCategory(ElementsCategory *parent, ElementsCollection *collection) :
	ElementsCollectionItem(parent),
	parent_collection_(collection),
	parent_category_(parent)
{
}

/**
	Destructeur
*/
ElementsCategory::~ElementsCategory() {
}

/**
	@return le projet auquel appartient cette categorie, si celle-ci
	appartient a une collection.
*/
QETProject *ElementsCategory::project() {
	if (parent_collection_) {
		return(parent_collection_ -> project());
	}
	return(0);
}

/**
	Ne fait rien ; le projet doit etre defini au niveau d'une collection
*/
void ElementsCategory::setProject(QETProject *) {
}

/**
	@return le protocole utilise pour designer la collection a laquelle
	appartient cette categorie
*/
QString ElementsCategory::protocol() {
	// il n'est pas possible d'avoir un protocole sans appartenir a une collection
	if (!hasParentCollection()) return(QString());
	
	return(parentCollection() -> protocol());
}

/**
	Ne fait rien
*/
void ElementsCategory::setProtocol(const QString &) {
}

/**
	@return la categorie parente de cette categorie, ou 0 si celle-ci n'en possede
*/
ElementsCategory *ElementsCategory::parentCategory() {
	return(parent_category_);
}

/**
	@return la liste des categories parentes de cet item.
	Cette liste peut etre vide s'il s'agit d'une categorie racine
*/
QList<ElementsCategory *> ElementsCategory::parentCategories() {
	QList<ElementsCategory *> cat_list;
	if (ElementsCategory *par_cat = parentCategory()) {
		cat_list << par_cat -> parentCategories() << par_cat;
	}
	return(cat_list);
}

/**
	@return true si cette categorie possede une categorie parente
*/
bool ElementsCategory::hasParentCategory() {
	return(parent_category_);
}

/**
	@return la collection a laquelle appartient la categorie
*/
ElementsCollection *ElementsCategory::parentCollection() {
	return(parent_collection_);
}

/**
	@return true si la categorie appartient a une collection
*/
bool ElementsCategory::hasParentCollection() {
	return(parent_collection_);
}

/**
	@param other_item Un autre item
	@return true si other_item est parent (direct ou indirect) de cet item, false sinon
*/
bool ElementsCategory::isChildOf(ElementsCollectionItem *other_item) {
	// verifie si l'autre item n'est pas la collection a laquelle cette categorie appartient
	if (ElementsCollection *other_item_coll = other_item -> toCollection()) {
		if (other_item_coll == parentCollection()) {
			return(true);
		}
	}
	
	// remonte les categories parentes pour voir si l'une d'entre elles correspond a cet autre item
	ElementsCategory *parent_cat = this;
	while ((parent_cat = parent_cat -> parentCategory())) {
		if (parent_cat == other_item) {
			return(true);
		}
	}
	
	// arrive ici, l'autre item n'est pas parent de cet item
	return(false);
}

/**
	@return le chemin virtuel de la categorie, avec le protocole
*/
QString ElementsCategory::fullVirtualPath() {
	// il n'est pas possible d'avoir un chemin virtuel sans appartenir a une collection
	if (!hasParentCollection()) return(QString());
	
	return(protocol() + "://" + virtualPath());
}

/**
	@return l'emplacement de la categorie
*/
ElementsLocation ElementsCategory::location() {
	return(ElementsLocation(fullVirtualPath(), project()));
}

/**
	@return true si cette categorie est la categorie racine de la collection
	a laquelle elle appartient, false sinon
*/
bool ElementsCategory::isRootCategory() const {
	// il faut appartenir a une collection pour etre categorie racine
	if (!parent_collection_) return(false);
	
	return(this == parent_collection_ -> rootCategory());
}

/**
	@return toujours false
*/
bool ElementsCategory::isCollection() const {
	return(false);
}

/**
	@return toujours true
*/
bool ElementsCategory::isCategory() const {
	return(true);
}

/**
	@return toujours false
*/
bool ElementsCategory::isElement() const {
	return(false);
}

/**
	@return toujours 0 - une categorie n'est pas une collection
*/
ElementsCollection *ElementsCategory::toCollection() {
	return(0);
}

/**
	@return un pointeur ElementsCategory * sur cette categorie
*/
ElementsCategory *ElementsCategory::toCategory() {
	return(this);
}

/**
	@return un pointeur ElementsCategory * sur cette categorie
*/
ElementsCategory *ElementsCategory::toPureCategory() {
	return(this);
}

/**
	@return toujours 0 - une categorie n'est pas un element
*/
ElementDefinition *ElementsCategory::toElement() {
	return(0);
}

/**
	@param target_category Categorie cible pour la copie ; cette categorie
	sera copiee en tant que sous-categorie de la categorie cible
	@param handler Gestionnaire d'erreurs a utiliser pour effectuer la copie
	Si handler vaut 0, les erreurs, problemes et questions sont purement et
	simplement ignores.
	@param deep_copy true pour copier recursivement le contenu (elements et
	sous-categories) de cette categorie, false sinon
	@return La copie de la categorie, ou 0 si le processus a echoue.
*/
ElementsCollectionItem *ElementsCategory::copy(ElementsCategory *target_category, MoveElementsHandler *handler, bool deep_copy) {
	if (!target_category) return(0);
	
	// echec si le path name de cette categorie est vide
	QString cat_name(pathName());
	if (cat_name.isEmpty()) return(0);
	
	// cree une description du mouvement a effectuer
	MoveElementsDescription mvt_desc;
	mvt_desc.setDestinationParentCategory(target_category);
	// on tente une copie avec le meme nom interne
	mvt_desc.setOriginalDestinationInternalName(cat_name);
	mvt_desc.setFinalDestinationInternalName(cat_name);
	mvt_desc.setHandler(handler);
	mvt_desc.setRecursive(deep_copy);
	
	// effectue le mouvement
	copy(&mvt_desc);
	return(mvt_desc.createdItem());
}

/**
	Methode privee effectuant une copie de cette categorie a partir d'une
	description du mouvement
*/
void ElementsCategory::copy(MoveElementsDescription *mvt_desc) {
	// quelques pointeurs pour simplifier l'ecriture de la methode
	MoveElementsHandler *handler = mvt_desc -> handler();
	ElementsCategory *target_category = mvt_desc -> destinationParentCategory();
	
	// verifie que la categorie parente cible est accessible en lecture
	if (!target_category -> isReadable()) {
		if (!handler) {
			return;
		} else {
			do {
				QET::Action todo = handler -> categoryIsNotReadable(target_category);
				
				// on agit en fonction de la reponse du handler
				if (todo == QET::Abort) {
					mvt_desc -> abort();
					return;
				} else if (todo == QET::Ignore || todo == QET::Managed) {
					return;
				} else if (todo == QET::Retry || todo == QET::Erase) {
					// reessayer = repasser dans la boucle
				} else if (todo == QET::Rename) {
					// cas non gere
				}
			} while (!target_category -> isReadable());
		}
	}
	
	// verifie que la source et la destination ne sont pas identiques
	if (target_category == this || target_category -> isChildOf(this)) {
		if (handler) {
			handler -> errorWithACategory(this, tr("La copie d'une catégorie vers elle-même ou vers l'une de ses sous-catégories n\'est pas gérée."));
		}
		return;
	}
	
	// verifie que la categorie parente cible est accessible en ecriture
	if (!target_category -> isWritable()) {
		if (!handler) {
			return;
		} else {
			do {
				QET::Action todo = handler -> categoryIsNotWritable(target_category);
				
				// on agit en fonction de la reponse du handler
				if (todo == QET::Abort) {
					mvt_desc -> abort();
					return;
				} else if (todo == QET::Ignore || todo == QET::Managed) {
					return;
				} else if (todo == QET::Retry || todo == QET::Erase) {
					// reessayer = repasser dans la boucle
				} else if (todo == QET::Rename) {
					// cas non gere
				}
			} while (!target_category -> isWritable());
		}
	}
	
	ElementsCategory *category_copy = 0;
	
	// verifie que la cible n'existe pas deja
	if ((category_copy = target_category -> category(mvt_desc -> finalDestinationInternalName()))) {
		if (!handler) {
			return;
		} else {
			do {
				// la cible existe deja : on demande au Handler ce qu'on doit faire
				QET::Action todo = handler -> categoryAlreadyExists(this, category_copy);
				
				// on agit en fonction de la reponse du handler
				if (todo == QET::Abort) {
					mvt_desc -> abort();
					return;
				} else if (todo == QET::Ignore || todo == QET::Managed) {
					return;
				} else if (todo == QET::Erase) {
					break;
				} else if (todo == QET::Rename) {
					mvt_desc -> setFinalDestinationInternalName(handler -> nameForRenamingOperation());
				}
			} while ((category_copy = target_category -> category(mvt_desc -> finalDestinationInternalName())));
		}
	}
	
	/*
		A ce stade, on peut creer la categorie cible : soit elle n'existe pas,
		soit on a l'autorisation de l'ecraser
	*/
	
	// si la cible existe deja, verifie qu'elle est accessible en ecriture
	category_copy = target_category -> category(mvt_desc -> finalDestinationInternalName());
	if (category_copy && !category_copy -> isWritable()) {
		if (!handler) {
			return;
		} else {
			do {
				// la cible n'est pas accessible en ecriture : on demande au Handler ce qu'on doit faire
				QET::Action todo = handler -> categoryIsNotWritable(category_copy);
				
				// on agit en fonction de la reponse du handler
				if (todo == QET::Abort) {
					mvt_desc -> abort();
					return;
				} else if (todo == QET::Ignore || todo == QET::Managed) {
					return;
				} else if (todo == QET::Retry || todo == QET::Erase) {
					// reessayer = repasser dans la boucle
				} else if (todo == QET::Rename) {
					// cas non gere
				}
			} while (!category_copy -> isWritable());
		}
	}
	
	// memorise la liste des sous-categories et elements directs
	QList<ElementsCategory  *> categories_list = categories();
	QList<ElementDefinition *> elements_list   = elements();
	
	// cree la categorie cible
	category_copy = target_category -> createCategory(mvt_desc -> finalDestinationInternalName());
	if (!category_copy) {
		/// @todo la creation a echoue : gerer ce cas
		return;
	}
	
	// recopie les noms de la categorie
	category_copy -> category_names = category_names;
	category_copy -> write();
	mvt_desc -> setCreatedItem(category_copy);
	
	// copie recursive
	if (mvt_desc -> isRecursive()) {
		// copie les sous-categories
		foreach(ElementsCategory *sub_category, categories_list) {
			// cree une description du mouvement a effectuer
			MoveElementsDescription sub_category_mvt_desc;
			sub_category_mvt_desc.setDestinationParentCategory(category_copy);
			// on tente une copie avec le meme nom interne
			sub_category_mvt_desc.setOriginalDestinationInternalName(sub_category -> pathName());
			sub_category_mvt_desc.setFinalDestinationInternalName(sub_category -> pathName());
			sub_category_mvt_desc.setHandler(handler);
			sub_category_mvt_desc.setRecursive(true);
			
			// effectue la copie
			sub_category -> copy(&sub_category_mvt_desc);
			
			// abort si besoin
			if (sub_category_mvt_desc.mustAbort()) {
				mvt_desc -> abort();
				return;
			}
		}
		
		// copie les elements
		foreach(ElementDefinition *element, elements_list) {
			// cree une description du mouvement a effectuer
			MoveElementsDescription element_mvt_desc;
			element_mvt_desc.setDestinationParentCategory(category_copy);
			// on tente une copie avec le meme nom interne
			element_mvt_desc.setOriginalDestinationInternalName(element -> pathName());
			element_mvt_desc.setFinalDestinationInternalName(element -> pathName());
			element_mvt_desc.setHandler(handler);
			
			element -> copy(&element_mvt_desc);
			
			// abort si besoin
			if (element_mvt_desc.mustAbort()) {
				mvt_desc -> abort();
				return;
			}
		}
	}
}

/**
	Cette methode copie la categorie recursivement puis la supprime, ce qui
	equivaut a un deplacement. Elle cree donc un nouvel objet representant la
	categorie, qu'elle retourne ensuite.
	@param target_category Categorie cible pour le deplacement ; cette categorie
	sera deplacee de faon a devenir une sous-categorie de la categorie cible
	@param handler Gestionnaire d'erreurs a utiliser pour effectuer le
	deplacement. Si handler vaut 0, les erreurs, problemes et questions sont
	purement et simplement ignores.
	@return Un pointeur vers la categorie apres le deplacement, ou 0 si le
	processus a echoue.
*/
ElementsCollectionItem *ElementsCategory::move(ElementsCategory *target_category, MoveElementsHandler *handler) {
	if (!target_category) return(0);
	
	// echec si le path name de cette categorie est vide
	QString cat_name(pathName());
	if (cat_name.isEmpty()) return(0);
	
	// cree une description du mouvement a effectuer
	MoveElementsDescription mvt_desc;
	mvt_desc.setDestinationParentCategory(target_category);
	// on tente une copie avec le meme nom interne
	mvt_desc.setOriginalDestinationInternalName(cat_name);
	mvt_desc.setFinalDestinationInternalName(cat_name);
	mvt_desc.setHandler(handler);
	mvt_desc.setRecursive(true);  // un deplacement est forcement recursif
	
	// effectue le mouvement
	move(&mvt_desc);
	return(mvt_desc.createdItem());
}

/**
	Methode privee effectuant le deplacement de cette categorie a partir d'une
	description du mouvement
	@param mvt_desc Description du mouvement
*/
void ElementsCategory::move(MoveElementsDescription *mvt_desc) {
	// quelques pointeurs pour simplifier l'ecriture de la methode
	MoveElementsHandler *handler = mvt_desc -> handler();
	ElementsCategory *target_category = mvt_desc -> destinationParentCategory();
	
	// empeche le deplacement s'il s'agit d'une categorie racine
	if (isRootCategory()) {
		if (handler) handler -> errorWithACategory(this, tr("Il n'est pas possible de déplacer une collection."));
		return;
	}
	
	// empeche le deplacement de la categorie dans une sous-categorie
	if (target_category == this || target_category -> isChildOf(this)) {
		if (handler) handler -> errorWithACategory(this, tr("Le déplacement d'une catégorie dans une de ses sous-catégories n'est pas possible."));
		return;
	}
	
	// effectue une copie non recursive de cette categorie
	ElementsCollectionItem *item_copy = copy(target_category, handler, false);
	if (!item_copy) return;
	ElementsCategory *category_copy = item_copy -> toCategory();
	if (!category_copy) return;
	
	// memorise la liste des sous-categories et elements directs
	QList<ElementsCategory  *> categories_list = categories();
	QList<ElementDefinition *> elements_list   = elements();
	
	// booleen indiquant si on pourra tenter de supprimer la categorie apres la copie
	bool remove_category = true;
	
	// tente de deplacer les sous-categories
	foreach(ElementsCategory *sub_category, categories_list) {
		// cree une description du mouvement a effectuer
		MoveElementsDescription sub_category_mvt_desc;
		sub_category_mvt_desc.setDestinationParentCategory(category_copy);
		// on tente un deplacement avec le meme nom interne
		sub_category_mvt_desc.setOriginalDestinationInternalName(sub_category -> pathName());
		sub_category_mvt_desc.setFinalDestinationInternalName(sub_category -> pathName());
		sub_category_mvt_desc.setHandler(handler);
		sub_category_mvt_desc.setRecursive(true);
		
		// effectue le deplacement
		sub_category -> move(&sub_category_mvt_desc);
		
		// abort si besoin
		if (sub_category_mvt_desc.mustAbort()) {
			mvt_desc -> abort();
			return;
		}
		
		// si la sous-categorie n'a pas ete supprimee, on ne supprimera pas cette categorie
		if (remove_category) remove_category = sub_category_mvt_desc.sourceItemWasDeleted();
		
		// si la sous-categorie n'a pas ete supprimee, ...
		if (!sub_category_mvt_desc.sourceItemWasDeleted()) {
			// on ne supprimera pas cette categorie
			if (remove_category) remove_category = false;
		}
	}
	
	// tente de deplacer les elements
	foreach(ElementDefinition *element, elements_list) {
		// cree une description du mouvement a effectuer
		MoveElementsDescription element_mvt_desc;
		element_mvt_desc.setDestinationParentCategory(category_copy);
		// on tente une copie avec le meme nom interne
		element_mvt_desc.setOriginalDestinationInternalName(element -> pathName());
		element_mvt_desc.setFinalDestinationInternalName(element -> pathName());
		element_mvt_desc.setHandler(handler);
		
		element -> move(&element_mvt_desc);
		
		// abort si besoin
		if (element_mvt_desc.mustAbort()) {
			mvt_desc -> abort();
			return;
		}
		
		// si l'element n'a pas ete supprime, ...
		if (!element_mvt_desc.sourceItemWasDeleted()) {
			// on ne supprimera pas cette categorie
			if (remove_category) remove_category = false;
		}
	}
	
	// supprime cette categorie (sinon il ne s'agirait que d'une copie, pas d'un deplacement)
	if (remove_category) {
		reload();
		bool category_deletion = remove();
		mvt_desc -> setSourceItemDeleted(category_deletion);
		if (!category_deletion && handler) {
			handler -> errorWithACategory(this, tr("La suppression de cette catégorie a échoué."));
		}
	}
}

/**
	Cette methode supprime recursivement les elements inutilises dans le projet.
	Si cette categorie n'est pas rattachee a un projet, elle ne fait rien
	@param handler Gestionnaire d'erreurs a utiliser pour effectuer le
	nettoyage. Si handler vaut 0, les erreurs, problemes et questions sont
	purement et simplement ignores.
*/
void ElementsCategory::deleteUnusedElements(MoveElementsHandler *handler) {
	// si cette categorie n'est pas rattachee a un projet, elle ne fait rien
	QETProject *parent_project = project();
	if (!parent_project) return;
	
	// supprime les elements inutilises dans les sous-categories
	foreach(ElementsCategory *sub_category, categories()) {
		sub_category -> deleteUnusedElements(handler);
	}
	
	// supprime les elements inutilises dans cette categorie
	foreach(ElementDefinition *element, elements()) {
		if (!parent_project -> usesElement(element -> location())) {
			bool element_deletion = element -> remove();
			if (!element_deletion && handler) {
				handler -> errorWithAnElement(element, tr("Impossible de supprimer l'élément"));
			}
		}
	}
}

/**
	Cette methode supprime toutes les sous-categories de cette categories qui
	ne contiennent pas d'elements ou de categories contenant des elements.
	@param handler Gestionnaire d'erreurs a utiliser pour effectuer le
	nettoyage. Si handler vaut 0, les erreurs, problemes et questions sont
	purement et simplement ignores.
*/
void ElementsCategory::deleteEmptyCategories(MoveElementsHandler *handler) {
	// supprime les sous-categories qui ne comportent pas d'elements
	foreach(ElementsCategory *sub_category, categories()) {
		sub_category -> deleteEmptyCategories(handler);
		sub_category -> reload();
		if (!sub_category -> isEmpty()) {
			bool category_deletion = sub_category -> remove();
			if (!category_deletion && handler) {
				handler -> errorWithACategory(sub_category, tr("Impossible de supprimer la catégorie"));
			}
		}
	}
}

/**
	@return true si cette collection est vide (pas de sous-categorie, pas
	d'element), false sinon.
*/
bool ElementsCategory::isEmpty() {
	return(categories().count() || elements().count());
}

/**
	@return the count of categories and elements within this collection
*/
int ElementsCategory::count() {
	int items_count = elements().count();
	foreach(ElementsCategory *category, categories()) {
		items_count += category -> count();
	}
	return(items_count);
}

/**
	Methode permettant d'obtenir le nom affichable de cette categorie.
	@return Le nom affichable de la categorie
*/
QString ElementsCategory::name() const {
	return(category_names.name(pathName()));
}

/**
	@return La liste des differents noms possibles pour la categorie
*/
NamesList ElementsCategory::categoryNames() const {
	return(category_names);
}

/**
	Vide la liste des noms de la categorie
*/
void ElementsCategory::clearNames() {
	category_names.clearNames();
}

/**
	Ajoute un nom a la categorie.
	Si la langue existe deja, le nom pour cette langue est remplace.
	@param lang La langue pour laquelle le nom est utilisable
	@param value Le nom
*/
void ElementsCategory::addName(const QString &lang, const QString &value) {
	category_names.addName(lang, value);
}

/**
	Specifie les noms de la categorie.
	Tous les noms precedemment connus sont perdus
*/
void ElementsCategory::setNames(const NamesList &names_list) {
	category_names = names_list;
}
