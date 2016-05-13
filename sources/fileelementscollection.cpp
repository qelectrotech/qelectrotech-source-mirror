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
#include "fileelementscollection.h"
#include "fileelementscategory.h"

/**
	Constructeur
	@param path Chemin du dossier racine de la collection
	@param parent QObject parent de la collection
*/
FileElementsCollection::FileElementsCollection(const QString &path, ElementsCollectionItem *parent) :
	ElementsCollection(parent),
	coll_path(path)
{
	protocol_ = "unknown";
	project_ = 0;
	root = 0;
}

/**
	Destructeur
*/
FileElementsCollection::~FileElementsCollection() {
	deleteContent();
}

/**
	@return la categorie racine de la collection
*/
ElementsCategory *FileElementsCollection::rootCategory() {
	return(root);
}

/**
	Recharge l'arborescence des categories et elements.
*/
void FileElementsCollection::reload() {
	QMutexLocker reload_lock(&reload_mutex_);
	// oublie l'arborescence precedente
	deleteContent();
	
	// le dossier doit exister et etre lisible
	QDir coll_dir(coll_path);
	if (!coll_dir.exists() || !coll_dir.isReadable()) return;
	coll_path = coll_dir.canonicalPath();
	
	root = new FileElementsCategory(coll_path, 0, this);
}

/**
	@return true si cette collection est representee quelque part sur le
	systeme de fichiers.
*/
bool FileElementsCollection::hasFilePath() {
	return(!coll_path.isEmpty());
}

/**
	@return le chemin du repertoire representant cette collection
*/
QString FileElementsCollection::filePath() {
	return(coll_path);
}

/**
	@param path Nouveau chemin de la collection
	Cette methode ne recharge pas la collection
*/
void FileElementsCollection::setFilePath(const QString &path) {
	coll_path = path;
}

/**
	Supprime le contenu en memoire de cette collection
*/
void FileElementsCollection::deleteContent() {
	delete root;
	root = 0;
}

/**
	@return ttrue si la categorie racine de la collection existe
*/
bool FileElementsCollection::exists() {
	return(root && root -> exists());
}

/**
	@return true si la collection est accessible en lecture
*/
bool FileElementsCollection::isReadable() {
	// une collection est accessible en lecture si sa categorie racine l'est
	return(root && root -> isReadable());
}

/**
	@return true si la collection est accessible en ecriture
*/
bool FileElementsCollection::isWritable() {
	// une collection est accessible en ecriture si sa categorie racine l'est
	return(root && root -> isWritable());
}

/**
	Ne fait rien
	@return toujours true
*/
bool FileElementsCollection::write() {
	return(true);
}

/**
	@return always true, since a file-based elements collection can always be
	cached.
*/
bool FileElementsCollection::isCacheable() const {
	return(true);
}
