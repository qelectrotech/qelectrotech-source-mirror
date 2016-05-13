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
#include "elementdefinition.h"
#include "elementscollection.h"
#include "moveelementshandler.h"
#include "moveelementsdescription.h"

/**
	@return true si l'element est rattache a une collection d'elements
	Un element appartenant a une collection a forcement un chemin virtuel.
*/
bool ElementDefinition::hasParentCategory() {
	return(parent_category_);
}

/**
 * @brief ElementDefinition::uuid
 * @return The uuid of this element definition.
 * If uuid can't be found, return a null QUuid.
 */
QUuid ElementDefinition::uuid()
{
	if (!m_uuid.isNull()) return m_uuid;

		//Get the uuid of element
	QList<QDomElement>  list_ = QET::findInDomElement(xml(), "uuid");

	if (!list_.isEmpty())
		m_uuid = QUuid(list_.first().attribute("uuid"));
	else
		qDebug() << "The element : " << filePath() << "haven't got an uuid, please edit and save this element with element editor to create an uuid";

	return m_uuid;

}

ElementsCategory *ElementDefinition::parentCategory() {
	return(parent_category_);
}

/**
	@return la liste des categories parentes de cet item.
*/
QList<ElementsCategory *> ElementDefinition::parentCategories() {
	QList<ElementsCategory *> cat_list;
	if (ElementsCategory *par_cat = parentCategory()) {
		cat_list << par_cat -> parentCategories() << par_cat;
	}
	return(cat_list);
}

/**
	@return true si l'element est rattache a une collection d'elements
	Un element appartenant a une collection a forcement un chemin virtuel.
*/
bool ElementDefinition::hasParentCollection() {
	return(parent_collection_);
}

/**
	@param other_item Autre item
	@return true si other_item est parent (direct ou indirect) de cet item, false sinon
*/
bool ElementDefinition::isChildOf(ElementsCollectionItem *other_item) {
	// soit l'autre item est le parent direct de cet element
	if (ElementsCategory *other_item_cat = other_item -> toCategory()) {
		if (other_item_cat == parentCategory()) {
			return(true);
		}
	}
	
	// soit il est un parent indirect, auquel cas, on peut demander a la categorie parente de repondre a la question
	if (ElementsCategory *parent_cat = parentCategory()) {
		return(parent_cat -> isChildOf(other_item));
	}
	
	// arrive ici, l'autre item n'est pas parent de cet item
	return(false);
}

/**
	@return la collection d'element a laquelle appartient cet element
*/
ElementsCollection *ElementDefinition::parentCollection() {
	return(parent_collection_);
}

/**
	@return le projet auquel appartient cette categorie, si celle-ci
	appartient a une collection.
*/
QETProject *ElementDefinition::project() {
	if (hasParentCollection()) {
		return(parentCollection() -> project());
	}
	return(0);
}

/**
	Ne fait rien ; le projet doit etre defini au niveau d'une collection
*/
void ElementDefinition::setProject(QETProject *) {
}

/**
	@return le protocole utilise par la collection a laquelle appartient cet element
*/
QString ElementDefinition::protocol() {
	// il n'est pas possible d'avoir un protocole sans appartenir a une collection
	if (!hasParentCollection()) return(QString());
	
	return(parentCollection() -> protocol());
}

/**
	Ne fait rien
*/
void ElementDefinition::setProtocol(const QString &) {
}

/**
	@return le chemin virtuel complet de cet element (protocole + chemin)
*/
QString ElementDefinition::fullVirtualPath() {
	// il n'est pas possible d'avoir un chemin virtuel sans appartenir a une collection
	if (!hasParentCollection()) return(QString());
	
	return(protocol() + "://" + virtualPath());
}

/**
	@return l'emplacement de l'element
*/
ElementsLocation ElementDefinition::location() {
	return(ElementsLocation(fullVirtualPath(), project()));
}

/**
	@return une liste vide - un element ne possede pas de categorie
*/
QList<ElementsCategory *> ElementDefinition::categories() {
	return(QList<ElementsCategory *>());
}

/**
	@return toujours 0 - un element ne possede pas de categorie
*/
ElementsCategory *ElementDefinition::category(const QString &) {
	return(0);
}

/**
	@return toujours 0 - un element ne possede pas de categorie
*/
ElementsCategory *ElementDefinition::createCategory(const QString &) {
	return(0);
}

/**
	@return une liste contenant seulement cet element
*/
QList<ElementDefinition *> ElementDefinition::elements() {
	return(QList<ElementDefinition *>() << this);
}

/**
	@return cet element si path est vide, 0 sinon
*/
ElementDefinition *ElementDefinition::element(const QString &path) {
	if (path.isEmpty()) return(this);
	return(0);
}

/**
	@return toujours 0 - un element n'en cree pas d'autre
*/
ElementDefinition *ElementDefinition::createElement(const QString &) {
	return(0);
}

/**
	@return always true - an element contains nothing but itself
*/
bool ElementDefinition::isEmpty() {
	return(false);
}

/**
	@return always 1
*/
int ElementDefinition::count() {
	return(1);
}

/**
	@return toujours 0 - un element n'est pas une collection
*/
ElementsCollection *ElementDefinition::toCollection() {
	return(0);
}

/**
	@return la categorie parente de cet element
*/
ElementsCategory *ElementDefinition::toCategory() {
	return(parentCategory());
}

/**
	@return toujours 0 - un element n'est pas une categorie
*/
ElementsCategory *ElementDefinition::toPureCategory() {
	return(0);
}

/**
	@return un pointeur ElementDefinition * sur cet element
*/
ElementDefinition *ElementDefinition::toElement() {
	return(this);
}

/**
 * @brief ElementDefinition::equals
 * @param other : ElementDefinition to compare with this
 * @return true if this element definition and other element definition is the same, else false
 */
bool ElementDefinition::equals(ElementDefinition &other)
{
		//Compare the uuid of the elements
	QList <QDomElement> this_uuid_dom = QET::findInDomElement(xml(), "uuid");
	QList <QDomElement> other_uuid_dom = QET::findInDomElement(other.xml(), "uuid");
	if ((this_uuid_dom.size() == 1) && (other_uuid_dom.size() == 1))
		return this_uuid_dom.first().attribute("uuid") == other_uuid_dom.first().attribute("uuid") ? true : false;

		//********
		//The code below is used to keep compatibility with previous version of qet
		//The uuid store in .elmt file, to compare two elements was created at version svn 4032
		///@TODO remove this code at version 0.6 or 0.7 (all users should already used the version with uuid)
		//********
		//Compare the xml definition transformed in QString. This method can return a false positive (notably with Qt5,
		//because the attributes of the xml isn't at the same order,with two instance of qet, for the same element)
	QDomDocument this_xml_document;
	this_xml_document.appendChild(this_xml_document.importNode(xml(), true));
	QString this_text = this_xml_document.toString(0);
	
	QDomDocument other_xml_document;
	other_xml_document.appendChild(other_xml_document.importNode(other.xml(), true));
	QString other_text = other_xml_document.toString(0);
	
	return(other_text == this_text);
}

/**
	@param target_category Categorie cible pour la copie ; elle doit exister
	@param handler Gestionnaire d'erreurs a utiliser pour effectuer la copie
	@param deep_copy Argument ignore - une copie "recursive" n'a pas de sens pour un element
	@return La copie de l'element ou 0 si le processus a echoue
*/
ElementsCollectionItem *ElementDefinition::copy(ElementsCategory *target_category, MoveElementsHandler *handler, bool deep_copy) {
	Q_UNUSED(deep_copy);
	if (!target_category) return(0);
	
	// echec si le path name de cet element est vide
	QString elmt_name(pathName());
	if (elmt_name.isEmpty()) return(0);
	
	// cree une description du mouvement a effectuer
	MoveElementsDescription mvt_desc;
	mvt_desc.setDestinationParentCategory(target_category);
	// on tente une copie avec le meme nom interne
	mvt_desc.setOriginalDestinationInternalName(pathName());
	mvt_desc.setFinalDestinationInternalName(pathName());
	mvt_desc.setHandler(handler);
	
	copy(&mvt_desc);
	return(mvt_desc.createdItem());
}

/**
	Methode privee effectuant une copie de cet element a partir d'une
	description du mouvement.
	@param mvt_desc Description du mouvement
*/
void ElementDefinition::copy(MoveElementsDescription *mvt_desc) {
	// quelques pointeurs pour simplifier l'ecriture de la methode
	MoveElementsHandler *handler = mvt_desc -> handler();
	ElementsCategory *target_category = mvt_desc -> destinationParentCategory();
	
	ElementDefinition *element_copy = 0;
	
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
	
	// verifie que la cible n'existe pas deja
	if ((element_copy = target_category -> element(mvt_desc -> finalDestinationInternalName()))) {
		if (!handler) {
			return;
		} else {
			do {
				// la cible existe deja : on demande au Handler ce qu'on doit faire
				QET::Action todo = handler -> elementAlreadyExists(this, element_copy);
				
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
			} while ((element_copy = target_category -> element(mvt_desc -> finalDestinationInternalName())));
		}
	}
	
	/*
		A ce stade, on peut creer l'element cible : soit il n'existe pas, soit
		on a l'autorisation de l'ecraser
	*/
	
	// si la cible existe deja, verifie qu'elle est accessible en ecriture
	element_copy = target_category -> element(mvt_desc -> finalDestinationInternalName());
	if (element_copy && !element_copy -> isWritable()) {
		if (!handler) {
			return;
		} else {
			do {
				// la cible n'est pas accessible en ecriture : on demande au Handler ce qu'on doit faire
				QET::Action todo = handler -> elementIsNotWritable(element_copy);
				
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
			} while (!element_copy -> isWritable());
		}
	}
	
	// cree l'element cible
	element_copy = target_category -> createElement(mvt_desc -> finalDestinationInternalName());
	if (!element_copy) {
		if (handler) {
			handler -> errorWithAnElement(this, tr("L'élément cible n'a pu être créé."));
		}
		return;
	}
	
	// recopie la definition de l'element
	element_copy -> setXml(xml());
	element_copy -> write();
	mvt_desc -> setCreatedItem(element_copy);
}

/**
	@param target_category Categorie cible pour le deplacement ; elle doit exister
	@param handler Gestionnaire d'erreurs a utiliser pour effectuer le deplacement
	@return L'element apres deplacement ou 0 si le processus a echoue
	
*/
ElementsCollectionItem *ElementDefinition::move(ElementsCategory *target_category, MoveElementsHandler *handler) {
	if (!target_category) return(0);
	
	// echec si le path name de cet element est vide
	QString elmt_name(pathName());
	if (elmt_name.isEmpty()) return(0);
	
	// cree une description du mouvement a effectuer
	MoveElementsDescription mvt_desc;
	mvt_desc.setDestinationParentCategory(target_category);
	// on tente un deplacement avec le meme nom interne
	mvt_desc.setOriginalDestinationInternalName(pathName());
	mvt_desc.setFinalDestinationInternalName(pathName());
	mvt_desc.setHandler(handler);
	
	move(&mvt_desc);
	return(mvt_desc.createdItem());
}

/**
	Methode privee effectuant un delacement de cet element a partir d'une
	description du mouvement.
	Pour etre plus exact, cette methode effectue d'abord une copie de l'element,
	puis, si celle-ci a reussi, il supprime l'element d'origine.
	@param mvt_desc Description du mouvement
*/
void ElementDefinition::move(MoveElementsDescription *mvt_desc) {
	// effectue une copie de l'element
	copy(mvt_desc);
	ElementsCollectionItem *item_copy = mvt_desc -> createdItem();
	if (!item_copy) return;
	ElementDefinition *element_copy = item_copy -> toElement();
	if (!element_copy) return;
	
	// supprime cet element
	MoveElementsHandler *handler = mvt_desc -> handler();
	
	// cet element doit etre accessible en ecriture pour etre supprime
	if (!isWritable()) {
		if (!handler) {
			return;
		} else {
			do {
				// on demande au Handler ce qu'on doit faire
				QET::Action todo = handler -> elementIsNotWritable(this);
				
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
			} while (!isWritable());
		}
	}
	
	// supprime cet element (sinon il ne s'agirait que d'une copie, pas d'un deplacement)
	bool element_deletion = remove();
	mvt_desc -> setSourceItemDeleted(element_deletion);
	if (!element_deletion && handler) {
		handler -> errorWithAnElement(this, tr("La suppression de cet élément a échoué."));
	}
}

/**
	Cette methode n'a aucun effet
	@return toujours true
*/
bool ElementDefinition::removeContent() {
	return(true);
}
