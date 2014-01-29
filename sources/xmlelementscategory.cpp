/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "xmlelementscategory.h"
#include "xmlelementscollection.h"
#include "xmlelementdefinition.h"
#include "qetproject.h"

/**
	Cree une categorie XML vide
	@param parent Categorie parente
	@param collection Collection a laquelle cette categorie appartient
*/
XmlElementsCategory::XmlElementsCategory(XmlElementsCategory *parent, XmlElementsCollection *collection) :
	ElementsCategory(parent, collection),
	xml_parent_collection_(collection),
	xml_parent_category_(parent)
{
}

/**
	Cree une categorie XML a partir d'un element XML
	@param xml_element Element XML a analyser
	@param parent Categorie parente
	@param collection Collection a laquelle cette categorie appartient
*/
XmlElementsCategory::XmlElementsCategory(const QDomElement &xml_element, XmlElementsCategory *parent, XmlElementsCollection *collection) :
	ElementsCategory(parent, collection),
	xml_parent_collection_(collection),
	xml_parent_category_(parent)
{
	loadContent(xml_element);
}

/**
	Destructeur
*/
XmlElementsCategory::~XmlElementsCategory() {
	deleteContent();
}

/**
	@return le nom de la categorie utilisable dans un chemin (virtuel ou reel)
*/
QString XmlElementsCategory::pathName() const {
	return(name_);
}

/**
	@return le chemin virtuel de la categorie, sans le protocole
*/
QString XmlElementsCategory::virtualPath() {
	// il n'est pas possible d'avoir un chemin virtuel sans appartenir a une collection
	if (!hasParentCollection() || name_.isEmpty()) return(QString());
	
	if (parent_category_) {
		QString tmp(parent_category_ -> virtualPath());
		if (!tmp.isEmpty()) tmp += "/";
		return(tmp + name_);
	} else {
		return(name_);
	}
}

/**
	@return toujours false, car une categorie XML n'a pas de chemin de type
	fichier
*/
bool XmlElementsCategory::hasFilePath() {
	// une categorie XML n'a pas de chemin de type fichier
	return(false);
}

/**
	@return une chaine vide, car une categorie XML n'a pas de chemin de type
	fichier
*/
QString XmlElementsCategory::filePath() {
	// une categorie XML n'a pas de chemin de type fichier
	return(QString());
}

/**
	Ne fait rien, car une categorie XML n'a pas de chemin de type fichier
*/
void XmlElementsCategory::setFilePath(const QString &) {
	// une categorie XML n'a pas de chemin de type fichier
}

/**
	@return la liste des sous-categories de la categorie
*/
QList<ElementsCategory *> XmlElementsCategory::categories() {
	QList<ElementsCategory *> cat_list;
	
	QList<QString> keys(categories_.keys());
	qSort(keys.begin(), keys.end());
	foreach(QString key, keys) cat_list << categories_[key];
	return(cat_list);
}

/**
	@return la categorie correspondant au chemin virtuel cat_path, ou 0 en cas d'echec
	@param cat_path Chemin virtuel de la categorie voulue
*/
ElementsCategory *XmlElementsCategory::category(const QString &cat_path) {
	// recupere les differentes parties du chemin
	QString cat_path_(cat_path);
	QStringList path_parts = cat_path_.split(QChar('/'), QString::SkipEmptyParts, Qt::CaseInsensitive);
	
	if (!path_parts.count()) {
		return(this);
	} else {
		// a-t-on la premiere sous-categorie ?
		if (!categories_.contains(path_parts.first())) {
			return(0);
		}
		
		// on a la premiere sous-categorie
		ElementsCategory *first_sub_cat = categories_.value(path_parts.first());
		
		if (path_parts.count() == 1) return(first_sub_cat);
		
		// on demande a la premiere sous-categorie de fournir la categorie finale
		path_parts.removeFirst();
		return(first_sub_cat -> category(path_parts.join("/")));
	}
}

/**
	Cree une categorie. La categorie parente doit exister.
	@param path chemin d'une categorie a creer sous la forme d'une adresse
	virtuelle comme common://cat1/cat2/cat3
	@return la nouvelle categorie demandee, ou 0 en cas d'echec
*/
ElementsCategory *XmlElementsCategory::createCategory(const QString &path) {
	// on ne doit pas etre en lecture seule
	if (!isWritable()) return(0);
	
	// recupere les differentes parties du chemin
	QString cat_path_(path);
	QStringList path_parts = cat_path_.split(QChar('/'), QString::SkipEmptyParts, Qt::CaseInsensitive);
	
	if (!path_parts.count()) {
		// le chemin est vide, on renvoie 0
		return(0);
	} else if (path_parts.count() == 1) {
		// il n'y a plus qu'une categorie dans le chemin : il faut la creer ici
		
		// on verifie si la categorie n'existe pas deja
		if (categories_.contains(path_parts[0])) {
			return(categories_.value(path_parts[0]));
		}
		
		// on cree un objet
		XmlElementsCategory *new_category = new XmlElementsCategory(
			this,
			xml_parent_collection_
		);
		new_category -> name_ = path_parts[0];
		
		// on l'integre dans la liste des sous-categories connues
		categories_.insert(path_parts[0], new_category);
		connect(new_category, SIGNAL(written()), this, SLOT(componentWritten()));
		connect(new_category, SIGNAL(removed(const QString &)), this, SLOT(componentRemoved(const QString &)));
		
		// on le renvoie
		return(new_category);
	} else {
		// il y a plusieurs categories dans le chemin :
		// on delegue le travail a la premiere sous-categorie
		
		// a-t-on la premiere sous-categorie ?
		if (!categories_.contains(path_parts.first())) {
			return(0);
		}
		
		// on a la premiere sous-categorie
		ElementsCategory *first_sub_cat = categories_.value(path_parts.first());
		
		// on demande a la premiere sous-categorie de fournir la categorie finale
		path_parts.removeFirst();
		return(first_sub_cat -> category(path_parts.join("/")));
	}
}

/**
	@return la liste des elements de la categorie
*/
QList<ElementDefinition *> XmlElementsCategory::elements() {
	QList<ElementDefinition *> elmt_list;
	
	QList<QString> keys(elements_.keys());
	qSort(keys.begin(), keys.end());
	foreach(QString key, keys) elmt_list << elements_[key];
	return(elmt_list);
}

/**
	@return l'element correspondant au chemin virtuel elmt_path, ou 0 en cas d'echec
	@param elmt_path Chemin virtuel de l'element voulu
*/
ElementDefinition *XmlElementsCategory::element(const QString &elmt_path) {
	// recupere les differentes parties du chemin
	QString elmt_path_(elmt_path);
	QStringList path_parts = elmt_path_.split(QChar('/'), QString::SkipEmptyParts, Qt::CaseInsensitive);
	
	if (!path_parts.count()) {
		// chemin vide
		return(0);
	} else if (path_parts.count() == 1) {
		// seulement le nom de fichier
		QString element_filename = path_parts.takeLast();
		if (!elements_.contains(element_filename)) {
			return(0);
		} else {
			return(elements_.value(element_filename));
		}
	} else {
		// separe le nom de fichier du chemin de la categorie et recupere celle-ci
		QString element_filename = path_parts.takeLast();
		ElementsCategory *elmt_cat = category(path_parts.join("/"));
		if (!elmt_cat) {
			return(0);
		} else {
			return(elmt_cat -> element(element_filename));
		}
	}
}

/**
	Cree un element. La categorie parente doit exister.
	@param path chemin d'un element a creer sous la forme d'une adresse
	virtuelle comme common://cat1/cat2/cat3/dog.elmt
	@return le nouvel element demande, ou 0 en cas d'echec
*/
ElementDefinition *XmlElementsCategory::createElement(const QString &path) {
	// on ne doit pas etre en lecture seule
	if (!isWritable()) return(0);
	
	// recupere les differentes parties du chemin
	QString cat_path_(path);
	QStringList path_parts = cat_path_.split(QChar('/'), QString::SkipEmptyParts, Qt::CaseInsensitive);
	
	if (!path_parts.count()) {
		// le chemin est vide, on renvoie 0
		return(0);
	} else if (path_parts.count() == 1) {
		// il n'y a plus que l'element dans le chemin : il faut le creer ici
		
		// on verifie si l'element n'existe pas deja
		if (elements_.contains(path_parts[0])) {
			return(elements_.value(path_parts[0]));
		}
		
		// on cree un objet
		XmlElementDefinition *new_element = new XmlElementDefinition(
			path_parts[0],
			this,
			xml_parent_collection_
		);
		
		// on l'integre dans la liste des elements connus
		elements_.insert(path_parts[0], new_element);
		connect(new_element, SIGNAL(written()), this, SLOT(componentWritten()));
		connect(new_element, SIGNAL(removed(const QString &)), this, SLOT(componentRemoved(const QString &)));
		
		// on le renvoie
		return(new_element);
	} else {
		// il y a plusieurs categories dans le chemin :
		// on delegue le travail a la premiere sous-categorie
		
		// a-t-on la premiere sous-categorie ?
		if (!categories_.contains(path_parts.first())) {
			return(0);
		}
		
		// on a la premiere sous-categorie
		ElementsCategory *first_sub_cat = categories_.value(path_parts.first());
		
		// on demande a la premiere sous-categorie de fournir la categorie finale
		path_parts.removeFirst();
		return(first_sub_cat -> createElement(path_parts.join("/")));
	}
}

/**
	@return true si la categorie existe, false sinon
*/
bool XmlElementsCategory::exists() {
	// la seule raison qu'une categorie aurait de ne pas exister est l'absence
	// de nom
	return(!name_.isEmpty());
}

/**
	@return true si la categorie est accessible en lecture, false sinon
*/
bool XmlElementsCategory::isReadable() {
	// une categorie XML n'a aucune raison de ne pas etre accessible en lecture
	return(true);
}

/**
	@return true si la categorie est accessible en ecriture, false sinon
*/
bool XmlElementsCategory::isWritable() {
	// une categorie XML peut etre en lecture seule si le projet auquel elle
	// appartient l'est
	if (QETProject *parent_project = project()) {
		return(!parent_project -> isReadOnly());
	} else {
		return(true);
	}
}

/**
	Cette methode ne fait rien. Recharger une categorie XML n'a pas vraiment de
	sens.
*/
void XmlElementsCategory::reload() {
}

/**
	Supprime la categorie et son contenu
*/
bool XmlElementsCategory::remove() {
	removeContent();
	emit(removed(name_));
	write();
	return(true);
}

/**
	Supprime le contenu de la categorie sans supprimer la categorie elle-meme.
*/
bool XmlElementsCategory::removeContent() {
	// suppression des sous-categories
	foreach(QString cat_name, categories_.keys()) {
		ElementsCategory *cat = categories_.value(cat_name);
		if (cat -> remove()) {
			categories_.take(cat_name);
			delete cat;
		} else {
			return(false);
		}
	}
	
	// suppression des elements
	foreach(QString elmt_name, elements_.keys()) {
		ElementDefinition *elmt = elements_.value(elmt_name);
		if (elmt -> remove()) {
			elements_.take(elmt_name);
			delete elmt;
		} else {
			return(false);
		}
	}
	write();
	return(true);
}

/**
	Ecrit la categorie.
	Comme il s'agit d'une categorie embarquee, cette methode emet simplement le
	signal written pour indiquer qu'il faut enregistrer la collection / le
	projet.
*/
bool XmlElementsCategory::write() {
	// indique que la categorie a ete changee
	emit(written());
	return(true);
}

/**
	@return un Element XML decrivant la categorie et son contenu
	@param xml_doc Document XML a utiliser pour creer l'element XML
*/
QDomElement XmlElementsCategory::writeXml(QDomDocument &xml_doc) const {
	QDomElement category_elmt = xml_doc.createElement("category");
	if (!isRootCategory()) {
		category_elmt.setAttribute("name", name_);
		category_elmt.appendChild(category_names.toXml(xml_doc));
	}
	
	foreach(XmlElementsCategory *subcat, categories_) {
		category_elmt.appendChild(subcat -> writeXml(xml_doc));
	}
	
	foreach(XmlElementDefinition *elmt, elements_) {
		category_elmt.appendChild(elmt -> writeXml(xml_doc));
	}
	
	return(category_elmt);
}

/**
	Gere le fait qu'une sous-categorie ou un element ait ete enregistre
*/
void XmlElementsCategory::componentWritten() {
	write();
}

/**
	Gere le fait qu'une sous-categorie ou un element ait ete supprime
	@param path Chemin de l'element ou de la categorie supprime(e)
*/
void XmlElementsCategory::componentRemoved(const QString &path) {
	if (elements_.contains(path)) {
		elements_.remove(path);
		write();
	} else if (categories_.contains(path)) {
		categories_.remove(path);
		write();
	}
}

/**
	Supprime le contenu de la categorie en memoire
*/
void XmlElementsCategory::deleteContent() {
	// suppression des elements
	foreach(QString elmt_name, elements_.keys()) {
		XmlElementDefinition *elmt = elements_.take(elmt_name);
		delete elmt;
	}
	
	// suppression des categories
	foreach(QString cat_name, categories_.keys()) {
		XmlElementsCategory *cat = categories_.take(cat_name);
		delete cat;
	}
}

/**
	Charge dans cet objet le contenu de la categorie a partir d'un element XML.
	@param xml_element element XML a analyser
*/
void XmlElementsCategory::loadContent(const QDomElement &xml_element) {
	deleteContent();
	name_.clear();
	category_names.clearNames();
	
	// charge le nom de la categorie pour son chemin virtuel
	name_ = xml_element.attribute("name");
	
	// charge les noms affiches de la categorie
	category_names.fromXml(xml_element);
	
	// charge les categories et elements
	QDomElement current_element;
	for (QDomNode node = xml_element.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
		if (!node.isElement()) continue;
		current_element = node.toElement();
		
		// les sous-categories et elements sans nom sont ignores
		if (!current_element.hasAttribute("name")) continue;
		
		if (current_element.tagName() == "category") {
			XmlElementsCategory *new_category = new XmlElementsCategory(current_element, this, xml_parent_collection_);
			categories_.insert(current_element.attribute("name"), new_category);
			connect(new_category, SIGNAL(written()), this, SLOT(componentWritten()));
			connect(new_category, SIGNAL(removed(const QString &)), this, SLOT(componentRemoved(const QString &)));
		} else if (current_element.tagName() == "element") {
			
			XmlElementDefinition *new_element = new XmlElementDefinition(current_element, this, xml_parent_collection_);
			elements_.insert(current_element.attribute("name"), new_element);
			connect(new_element, SIGNAL(written()), this, SLOT(componentWritten()));
			connect(new_element, SIGNAL(removed(const QString &)), this, SLOT(componentRemoved(const QString &)));
		}
	}
}
