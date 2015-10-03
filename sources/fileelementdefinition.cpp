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
#include "elementscollectioncache.h"
#include "fileelementdefinition.h"
#include "fileelementscategory.h"
#include "fileelementscollection.h"
#include "qetapp.h"
#include "qet.h"

/**
	Constructeur
	@param uri Chemin du fichier contenant la definition de l'element
	@param category Categorie parente
	@param collection collection parente
*/
FileElementDefinition::FileElementDefinition(const QString &uri, FileElementsCategory *category, FileElementsCollection *collection) :
	ElementDefinition(category, collection),
	is_null(true),
	file_path(uri)
{
	reload();
}

/**
	Destructeur
*/
FileElementDefinition::~FileElementDefinition() {
}

/**
	@return la definition XML de l'element
*/
QDomElement FileElementDefinition::xml() {
	// ouvre le fichier
	QFile file(file_path);
	
	// charge le contenu du fichier en s'attendant a du XML
	is_null = !xml_element_.setContent(&file);
	if (is_null) {
		return(QDomElement());
	} else {
		// l'ouverture de la definition a reussi
		return(xml_element_.documentElement());
	}
}

/**
	Change la definition XML de l'element
	@param xml_element Nouvelle definition XML de l'element
	@return true si l'operation s'est bien passee, false sinon
*/
bool FileElementDefinition::setXml(const QDomElement &xml_element) {
	xml_element_.clear();
	xml_element_.appendChild(xml_element_.importNode(xml_element, true));
	is_null = false;
	return(true);
}

/**
	Enregistre la definition de l'element.
	@return true si l'operation a reussi, false sinon
*/
bool FileElementDefinition::write() {
	return(QET::writeXmlFile(xml_element_, file_path));
}

/**
	@return true si la definition n'est pas disponible
*/
bool FileElementDefinition::isNull() const {
	return(is_null);
}

/**
	@return Le nom de cet element dans l'arborescence
*/
QString FileElementDefinition::pathName() const {
	return(QFileInfo(file_path).fileName());
}

/**
	@return le chemin virtuel de cet element
*/
QString FileElementDefinition::virtualPath() {
	// il n'est pas possible d'avoir un chemin virtuel sans appartenir a une collection
	if (!hasParentCollection()) return(QString());
	// recupere le chemin absolu de la racine de la collection
	QString root_abs_path(parentCollection() -> filePath());
	
	if (!file_path.startsWith(root_abs_path)) return(QString());
	QString virtual_path(file_path);
	virtual_path.remove(root_abs_path);
	virtual_path.remove(QRegExp("^/"));
	return(virtual_path);
}

/**
 * @brief FileElementDefinition::reload
 * Reload this file element definition
 */
void FileElementDefinition::reload()
{
	if (file_path.isEmpty())
	{
		is_null = true;
		return;
	}
	
	// recupere le chemin du fichier *.elmt correspondant
	QFileInfo file_info(file_path);
	if (!file_info.exists() || !file_info.isReadable())
	{
		is_null = true;
		return;
	}
	file_path = file_info.canonicalFilePath();
	
	if (parentCollection())
	{
		ElementsCollectionCache *cache = parentCollection() -> cache();
		if (cache && cache -> fetchNameFromCache(location().toString(), uuid()))
		{
			// the element file has not been modified since the last time
			// we put its name in cache: we do not need to load it.
			is_null = false;
			return;
		}
	}
	
	// we need to ensure this is a valid XML document
	QFile file(file_path);
	QDomDocument xml_document;
	is_null = !xml_document.setContent(&file);
	xml_document.clear();
}

/**
	@return true si le fichier existe, false sinon
*/
bool FileElementDefinition::exists() {
	if (isNull()) return(false);
	return(QFileInfo(file_path).exists());
}

/**
	@return true si le fichier representant l'element est accessible en
	lecture.
*/
bool FileElementDefinition::isReadable() {
	if (isNull()) return(false);
	return(QFileInfo(file_path).isReadable());
}

/**
	@return true si le fichier representant l'element est accessible en
	ecriture.
*/
bool FileElementDefinition::isWritable() {
	if (isNull()) return(false);
	return(QFileInfo(file_path).isWritable());
}

/**
	Supprime le fichier representant l'element
	@return true si l'operation s'est bien passee, false sinon
*/
bool FileElementDefinition::remove() {
	QFile elmt_file(file_path);
	if (!elmt_file.exists()) return(true);
	return(elmt_file.remove());
}

/**
	@return true si cet element est represente quelque part sur le systeme de
	fichiers
*/
bool FileElementDefinition::hasFilePath() {
	return(!file_path.isEmpty());
}

/**
	@return le fichier representant cet element sur le systeme de fichiers
*/
QString FileElementDefinition::filePath() {
	return(file_path);
}

/**
	Definit le nouveau chemin de cet element dans le systeme de fichiers
*/
void FileElementDefinition::setFilePath(const QString &path) {
	// recupere le chemin du fichier *.elmt correspondant
	QFileInfo file_info(path);
	if (!file_info.exists() || !file_info.isReadable()) {
		return;
	}
	file_path = file_info.canonicalFilePath();
}

/**
	@return the time of the last modification (mtime) for this element file
*/
QDateTime FileElementDefinition::modificationTime() const {
	QFileInfo file_info(file_path);
	if (!file_info.exists() || !file_info.isReadable()) {
		return QDateTime();
	}
	return(file_info.lastModified());
}
