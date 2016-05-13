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
#include "fileelementscategory.h"
#include "fileelementscollection.h"
#include "fileelementdefinition.h"
#include "qet.h"

/**
	Constructeur
	@param path Chemin du dossier de la categorie
	@param parent Categorie parente
	@param coll Collection parente
*/
FileElementsCategory::FileElementsCategory(const QString &path, FileElementsCategory *parent, FileElementsCollection *coll) :
	ElementsCategory(parent, coll),
	file_parent_collection_(coll),
	file_parent_category_(parent),
	cat_dir(path)
{
	reload();
}

/**
	Destructeur
*/
FileElementsCategory::~FileElementsCategory() {
	deleteContent();
}

/**
	@return le nom de la categorie utilisable dans un chemin (virtuel ou reel)
*/
QString FileElementsCategory::pathName() const {
	return(cat_dir.dirName());
}

/**
	@return le chemin virtuel de la categorie, sans le protocole
*/
QString FileElementsCategory::virtualPath() {
	
	// il n'est pas possible d'avoir un chemin virtuel sans appartenir a une collection
	if (!hasParentCollection()) return(QString());
	
	// recupere le chemin absolu de la racine de la collection
	QString root_abs_path(parentCollection() -> filePath());
	if (!filePath().startsWith(root_abs_path)) return(QString());
	
	QString virtual_path(filePath());
	virtual_path.remove(root_abs_path);
	virtual_path.remove(QRegExp("^/"));
	return(virtual_path);
}

/**
	@return true si la categorie possede un chemin sur un systeme de fichiers
*/
bool FileElementsCategory::hasFilePath() {
	return(!filePath().isEmpty());
}

/**
	@return le chemin sur le systeme de fichiers de la categorie
*/
QString FileElementsCategory::filePath() {
	return(cat_dir.path());
}

/**
	Definit le chemin du dossier representant la categorie
	@param p nouveau chemin du dossier representant la categorie
*/
void FileElementsCategory::setFilePath(const QString &p) {
	cat_dir.setPath(p);
}

/**
	@return la liste des sous-categories de la categorie
*/
QList<ElementsCategory *> FileElementsCategory::categories() {
	QList<ElementsCategory *> result;
	
	QList<QString> keys(categories_.keys());
	qSort(keys.begin(), keys.end());
	foreach(QString key, keys) result << categories_[key];
	
	return(result);
}

/**
	@return la categorie correspondant au chemin virtuel cat_path, ou 0 en cas d'echec
	@param cat_path Chemin virtuel de la categorie voulue
*/
ElementsCategory *FileElementsCategory::category(const QString &cat_path) {
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
ElementsCategory *FileElementsCategory::createCategory(const QString &path) {
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
		FileElementsCategory *new_category = new FileElementsCategory(
			cat_dir.absolutePath() + "/" + path_parts[0],
			this,
			file_parent_collection_
		);
		
		// on l'integre dans la liste des sous-categories connues
		categories_.insert(path_parts[0], new_category);
		
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
QList<ElementDefinition *> FileElementsCategory::elements() {
	QList<ElementDefinition *> result;
	
	QList<QString> keys(elements_.keys());
	qSort(keys.begin(), keys.end());
	foreach(QString key, keys) result << elements_[key];
	
	return(result);
}

/**
	@return l'element correspondant au chemin virtuel elmt_path, ou 0 en cas d'echec
	@param elmt_path Chemin virtuel de l'element voulu
*/
ElementDefinition *FileElementsCategory::element(const QString &elmt_path) {
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
ElementDefinition *FileElementsCategory::createElement(const QString &path) {
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
		FileElementDefinition *new_element = new FileElementDefinition(
			cat_dir.absolutePath() + "/" + path_parts[0],
			this,
			file_parent_collection_
		);
		
		// on l'integre dans la liste des elements connus
		elements_.insert(path_parts[0], new_element);
		
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
	@return true si le dossier representant la categorie existe
*/
bool FileElementsCategory::exists() {
	return(cat_dir.exists());
}

/**
	@return true si la categorie est accessible en lecture
	Cett methode retourne true a partir du moment ou le dossier representant
	cette categorie est accessible en lecture. Il se peut que les elements ou
	le fichier qet_directory a l'interieur ne soient pas accessibles en
	ecriture.
*/
bool FileElementsCategory::isReadable() {
	return(QFileInfo(cat_dir.absolutePath()).isReadable());
}

/**
	@return true s'il est possible d'ecrire le fichier qet_directory dans la
	categorie
*/
bool FileElementsCategory::isWritable() {
	// informations sur le dossier de la categorie
	QFileInfo category(cat_dir.absolutePath());
	QFileInfo qet_directory(cat_dir.absolutePath() + "/qet_directory");
	/*
	soit qet_directory n'existe pas et le dossier est accessible en ecriture,
	soit qet_directory existe et est accessible en ecriture
	*/
	return(
		category.isWritable() &&           // le dossier lui-meme est accessible en ecriture
		(
			!qet_directory.exists() ||
			(qet_directory.exists() && qet_directory.isWritable())
		)                                  // le fichier qet_directory est accessible en ecriture
	);
}

/**
	Recharge le contenu et les noms de la categorie
*/
void FileElementsCategory::reload() {
	// supprime l'ancien contenu
	deleteContent();
	category_names.clearNames();
	
	// la categorie doit exister
	if (!cat_dir.exists()) return;
	
	// charge les noms de la categorie
	loadNames();
	
	// charge les sous-categories
	QStringList dirs = cat_dir.entryList(QStringList(), QDir::AllDirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDir::Name);
	foreach(QString dir, dirs) {
		categories_.insert(dir, new FileElementsCategory(cat_dir.absoluteFilePath(dir), this, file_parent_collection_));
	}
	
	// charge les elements
	QStringList elmts = cat_dir.entryList(QStringList("*.elmt"), QDir::Files, QDir::Name);
	foreach(QString elmt, elmts) {
		elements_.insert(elmt, new FileElementDefinition(cat_dir.absoluteFilePath(elmt), this, file_parent_collection_));
	}
}

/**
	Supprime le contenu de la categorie puis la categorie elle-meme
	@return true si l'operation s'est bien passee, false sinon
*/
bool FileElementsCategory::remove() {
	// suppression du contenu de la categorie
	if (!removeContent()) return(false);
	
	// une categorie racine ne se supprime pas elle-meme
	if (isRootCategory()) return(true);
	
	// suppression du fichier de description de la categorie
	if (cat_dir.exists("qet_directory")) {
		if (!cat_dir.remove("qet_directory")) return(false);
	}
	
	// suppression de la categorie elle-meme
	return(cat_dir.rmdir(cat_dir.absolutePath()));
}

/**
	Supprime le contenu de la categorie sans supprimer la categorie elle-meme.
	@return true si l'operation s'est bien passee, false sinon
*/
bool FileElementsCategory::removeContent() {
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
	return(true);
}

/**
	Cree la categorie
	@return true si la creation a reussi, false sinon
*/
bool FileElementsCategory::write() {
	// cree le dossier de la categorie
	if (!cat_dir.mkpath(cat_dir.path())) return(false);
	
	// prepare la structure XML
	QDomDocument document;
	QDomElement root = document.createElement("qet-directory");
	document.appendChild(root);
	root.appendChild(category_names.toXml(document));
	
	QString filepath = cat_dir.absolutePath() + "/qet_directory";
	return(QET::writeXmlFile(document, filepath));
}

/**
	Supprime un repertoire recursivement.
	@return true si la suppression a reussie, false sinon
*/
bool FileElementsCategory::rmdir(const QString &path) const {
	QDir directory(path);
	
	// supprime les dossiers, recursivement
	foreach (QString file, directory.entryList(QDir::AllDirs | QDir::NoDotAndDotDot)) {
		if (!rmdir(directory.absolutePath() + "/" + file)) return(false);
	}
	
	// supprime les fichiers
	foreach (QString file, directory.entryList(QDir::Files | QDir::NoDotAndDotDot)) {
		if (!directory.remove(file)) return(false);
	}
	
	// supprime le dossier lui-meme
	return(directory.rmdir(path));
}

/**
	Charge la liste des noms possibles pour la categorie
*/
void FileElementsCategory::loadNames() {
	// repere le chemin du fichier de configuration de la categorie
	QFile directory_conf(cat_dir.absolutePath() + "/qet_directory");
	
	// verifie l'existence du fichier
	if (!directory_conf.exists()) return;
	
	// ouvre le fichier
	if (!directory_conf.open(QIODevice::ReadOnly | QIODevice::Text)) return;
	
	// lit le contenu du fichier dans un QDomDocument XML
	QDomDocument document;
	if (!document.setContent(&directory_conf)) return;
	
	// verifie la racine
	QDomElement root = document.documentElement();
	if (root.tagName() != "qet-directory") return;
	
	category_names.fromXml(root);
	
	// ferme le fichier
	directory_conf.close();
}

/**
	Supprime le contenu de la categorie en memoire
*/
void FileElementsCategory::deleteContent() {
	// suppression des elements
	foreach(QString elmt_name, elements_.keys()) {
		FileElementDefinition *elmt = elements_.take(elmt_name);
		delete elmt;
	}
	
	// suppression des categories
	foreach(QString cat_name, categories_.keys()) {
		FileElementsCategory *cat = categories_.take(cat_name);
		delete cat;
	}
}
