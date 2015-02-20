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
#include "elementscollection.h"
#include "elementscategory.h"
#include "elementdefinition.h"
#include "moveelementshandler.h"

/**
	Constructeur
	@param parent Item parent
*/
ElementsCollection::ElementsCollection(ElementsCollectionItem *parent) :
	ElementsCollectionItem(parent),
	cache_(0)
{
}

/**
	Destructeur
*/
ElementsCollection::~ElementsCollection() {
}

/**
	@return the title for this collection
*/
QString ElementsCollection::title() const {
	return(title_);
}

/**
	@param title New title for this collection
*/
void ElementsCollection::setTitle(const QString &title) {
	if (title_ == title) return;
	title_ = title;
	emit(elementsCollectionChanged(this));
}

/**
	@return the icon for this collection
*/
QIcon ElementsCollection::icon() const {
	return(icon_);
}

/**
	@param icon the icon for this collection
*/
void ElementsCollection::setIcon(const QIcon &icon) {
	if (icon_.cacheKey() == icon.cacheKey()) return;
	icon_ = icon;
	emit(elementsCollectionChanged(this));
}

/**
	@return toujours true
*/
bool ElementsCollection::isCollection() const {
	return(true );
}

/**
	@return toujours false
*/
bool ElementsCollection::isRootCategory() const {
	return(false);
}

/**
	@return toujours false
*/
bool ElementsCollection::isCategory()  const {
	return(false);
}

/**
	@return toujours false
*/
bool ElementsCollection::isElement() const {
	return(false);
}

/**
	@return un pointeur ElementsCollection * sur cette collection
*/
ElementsCollection *ElementsCollection::toCollection() {
	return(this);
}

/**
	@return un pointeur vers la categorie racine de cette collection
*/
ElementsCategory *ElementsCollection::toCategory() {
	return(rootCategory());
}

/**
	@return toujours 0 - une collection n'est pas a proprement parler une
	categorie
*/
ElementsCategory *ElementsCollection::toPureCategory() {
	return(0);
}

/**
	@return toujours 0 - une collection n'est pas un element
*/
ElementDefinition *ElementsCollection::toElement() {
	return(0);
}

/**
	@param target_category Categorie cible pour la copie ; la categorie racine
	de cette collection sera copiee en tant que sous-categorie de la categorie
	cible
	@param handler Gestionnaire d'erreurs a utiliser pour effectuer la copie
	@param deep_copy true pour copier recursivement le contenu (elements et
	sous-categories) de la categorie racine, false sinon
	@return La copie de la categorie, ou 0 si le processus a echoue.
*/
ElementsCollectionItem *ElementsCollection::copy(ElementsCategory *target_category, MoveElementsHandler *handler, bool deep_copy) {
	if (ElementsCategory *root = rootCategory()) {
		return(root -> copy(target_category, handler, deep_copy));
	}
	return(0);
}

/**
	Il n'est pas possible de deplacer une collection. Cette methode demande
	simplement au gestionnaire d'erreur handler d'afficher un message.
*/
ElementsCollectionItem *ElementsCollection::move(ElementsCategory *, MoveElementsHandler *handler) {
	if (ElementsCategory *root = rootCategory()) {
		if (handler) {
			handler -> errorWithACategory(root, tr("Il n'est pas possible de d\351placer une collection."));
		}
	}
	return(0);
}


/**
	Vide la collection de son contenu sans la supprimer
	@return true si l'operation a reussi, false sinon
*/
bool ElementsCollection::removeContent() {
	if (!rootCategory()) return(true);
	
	// demande a la categorie racine de supprimer son contenu
	return(rootCategory() -> removeContent());
}

/**
	Vide la collection de son contenu sans la supprimer
	@return true si l'operation a reussi, false sinon
*/
bool ElementsCollection::remove() {
	return(removeContent());
}

/**
	@return le projet auquel est rattachee cette collection ou 0 si
	celle-ci n'est pas liee a un projet.
*/
QETProject *ElementsCollection::project() {
	return(project_);
}

/**
	@param project le nouveau projet auquel est rattachee cette collection
	Indiquer 0 pour que cette collection ne soit plus liee a un projet.
*/
void ElementsCollection::setProject(QETProject *project) {
	project_ = project;
}

/**
	@return le protocole utilise par cette collection ; exemples :
	"common" pour la collection commune qui utilise des URLs en common://
	"custom" pour la collection perso qui utilise des URLs en custom://
	"embed" pour une collection embarquee qui utilise des URLs en embed://
*/
QString ElementsCollection::protocol() {
	return(protocol_);
}

/**
	Definit le protocole de cette collection
	@param p Nouveau protocole de cette collection
*/
void ElementsCollection::setProtocol(const QString &p) {
	if (!p.isEmpty()) protocol_ = p;
}

/**
	@return toujours 0 - une collection n'a pas de categorie parente. En
	revanche, elle peut posseder un projet parent.
	@see project()
*/
ElementsCategory *ElementsCollection::parentCategory() {
	return(0);
}

/**
	@return toujours une liste vide - une collection n'a pas de categorie
	parente. En revanche, elle peut posseder un projet parent.
	@see project()
*/
QList<ElementsCategory *> ElementsCollection::parentCategories() {
	return(QList<ElementsCategory *>());
}

/**
	@return toujours false0 - une collection n'a pas de categorie parente. En
	revanche, elle peut posseder un projet parent.
	@see project()
*/
bool ElementsCollection::hasParentCategory() {
	return(false);
}

/**
	@return toujours 0 - une collection n'a pas de collection parente. En
	revanche, elle peut posseder un projet parent.
	@see project()
*/
ElementsCollection *ElementsCollection::parentCollection() {
	return(0);
}

/**
	@return toujours false - une collection n'a pas de collection parente. En
	revanche, elle peut posseder un projet parent.
	@see project()
*/
bool ElementsCollection::hasParentCollection() {
	return(false);
}

/**
	@return toujours false - une collection ne peut etre l'enfant de quoi que ce
	soit.
*/
bool ElementsCollection::isChildOf(ElementsCollectionItem *) {
	return(false);
}

/**
	@return toujours une chaine vide
*/
QString ElementsCollection::pathName() const {
	return(QString());
}

/**
	@return toujours une chaine vide
*/
QString ElementsCollection::virtualPath() {
	return(QString());
}

/**
	@return le protocole suivi de :// ou une chaine vide si cette collection
	n'a pas de protocole defini.
*/
QString ElementsCollection::fullVirtualPath() {
	if (protocol().isEmpty()) return(QString());
	return(protocol() + "://");
}

/**
	@return l'emplacement de cette collection
*/
ElementsLocation ElementsCollection::location() {
	return(ElementsLocation(fullVirtualPath(), project()));
}

/**
	@return une liste ne contenant que la categorie racine de la collection
*/
QList<ElementsCategory *> ElementsCollection::categories() {
	QList<ElementsCategory *> result;
	if (ElementsCategory *root = rootCategory()) {
		result << root;
	}
	return(result);
}

/**
	@param cat_path chemin d'une categorie sous la forme d'une adresse
	virtuelle comme common://cat1/cat2/cat3
	@return la categorie demandee, ou 0 si celle-ci n'a pas ete trouvee
*/
ElementsCategory *ElementsCollection::category(const QString &cat_path) {
	ElementsCategory *root = rootCategory();
	// on doit avoir une categorie racine
	if (!root) return(0);
	
	// le protocole de l'adresse virtuelle doit correspondre
	if (!cat_path.startsWith(protocol_ + "://", Qt::CaseInsensitive)) return(0);
	
	// on enleve le protocole
	QString cat_path_(cat_path);
	cat_path_.remove(QRegExp("^" + protocol_ + ":\\/\\/", Qt::CaseInsensitive));
	
	// on fait appel a la categorie racine pour le reste du traitement
	return(root -> category(cat_path_));
}

/**
	Cree une categorie. La categorie parente doit exister.
	@param path chemin d'une categorie a creer sous la forme d'une adresse
	virtuelle comme common://cat1/cat2/cat3
	@return la nouvelle categorie demandee, ou 0 en cas d'echec
*/
ElementsCategory *ElementsCollection::createCategory(const QString &path) {
	ElementsCategory *root = rootCategory();
	// on doit avoir une categorie racine
	if (!root) return(0);
	
	// on ne doit pas etre en lecture seule
	if (!isWritable()) return(0);
	
	// le protocole de l'adresse virtuelle doit correspondre
	if (!path.startsWith(protocol_ + "://", Qt::CaseInsensitive)) return(0);
	
	// on enleve le protocole
	QString path_(path);
	path_.remove(QRegExp("^" + protocol_ + ":\\/\\/", Qt::CaseInsensitive));
	
	// on fait appel a la categorie racine pour le reste du traitement
	return(root -> createCategory(path_));
}

/**
	@return une liste vide
*/
QList<ElementDefinition *> ElementsCollection::elements() {
	return(QList<ElementDefinition *>());
}

/**
	@param elmt_path chemin d'un element sous la forme d'une adresse
	virtuelle comme common://cat1/cat2/cat3/dog.elmt
	@return l'element demande, ou 0 si celui-ci n'a pas ete trouve
*/
ElementDefinition *ElementsCollection::element(const QString &elmt_path) {
	ElementsCategory *root = rootCategory();
	// on doit avoir une categorie racine
	if (!root) return(0);
	
	// le protocole de l'adresse virtuelle doit correspondre
	if (!elmt_path.startsWith(protocol_ + "://", Qt::CaseInsensitive)) return(0);
	
	// on enleve le protocole
	QString elmt_path_(elmt_path);
	elmt_path_.remove(QRegExp("^" + protocol_ + ":\\/\\/", Qt::CaseInsensitive));
	
	// on fait appel a la categorie racine pour le reste du traitement
	return(root -> element(elmt_path_));
}

/**
	Cree un element. La categorie parente doit exister.
	@param path chemin d'un element a creer sous la forme d'une adresse
	virtuelle comme common://cat1/cat2/cat3/dog.elmt
	@return le nouvel element demande, ou 0 en cas d'echec
*/
ElementDefinition *ElementsCollection::createElement(const QString &path) {
	ElementsCategory *root = rootCategory();
	// on doit avoir une categorie racine
	if (!rootCategory()) return(0);
	
	// on ne doit pas etre en lecture seule
	if (!isWritable()) return(0);
	
	// le protocole de l'adresse virtuelle doit correspondre
	if (!path.startsWith(protocol_ + "://", Qt::CaseInsensitive)) return(0);
	
	// on enleve le protocole
	QString path_(path);
	path_.remove(QRegExp("^" + protocol_ + ":\\/\\/", Qt::CaseInsensitive));
	
	// on fait appel a la categorie racine pour le reste du traitement
	return(root -> createElement(path_));
}

/**
	@return true si cette collection est vide (pas de sous-categorie, pas
	d'element), false sinon.
*/
bool ElementsCollection::isEmpty() {
	ElementsCategory *root_category = rootCategory();
	if (!root_category) return(true);
	return(root_category -> isEmpty());
}

/**
	@return the count of categories and elements within this collection
*/
int ElementsCollection::count() {
	ElementsCategory *root_category = rootCategory();
	if (!root_category) return(0);
	return(root_category -> count());
}

/**
	@param item_path chemin d'un item sous la forme d'une adresse
	virtuelle comme common://cat1/cat2/cat3
	@param prefer_collections true pour renvoyer la collection lorsque le
	chemin correspond aussi bien a une collection qu'a sa categorie racine
	@return l'item demande, ou 0 si celui-ci n'a pas ete trouve
*/
ElementsCollectionItem *ElementsCollection::item(const QString &item_path, bool prefer_collections) {
	ElementsCollectionItem *result = 0;
	
	// essaye de trouver l'item en tant que categorie
	result = category(item_path);
	// si la categorie est trouvee, ...
	if (result) {
		// ... qu'il s'agit d'une categorie racine et que l'utilisateur prefere les collections
		if (prefer_collections && result -> isRootCategory()) {
			// ... alors on renvoie la collection et non la categorie
			result = this;
		}
	}
	
	// sinon essaye de trouver l'item en tant qu'element
	if (!result) result = element(item_path);
	
	return(result);
}

/**
	@return The cache used by this collection, or 0 if this collection does not have any
*/
ElementsCollectionCache *ElementsCollection::cache() const {
	return(cache_);
}

/**
	@param cache The cache to be used by this collection
*/
void ElementsCollection::setCache(ElementsCollectionCache *cache) {
	cache_ = cache;
}
