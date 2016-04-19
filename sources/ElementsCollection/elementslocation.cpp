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
#include "elementslocation.h"
#include "qetapp.h"
#include "xmlelementcollection.h"
#include "qetproject.h"
#include "elementscollectioncache.h"
#include "elementfactory.h"
#include "element.h"
#include "qetxml.h"

// make this class usable with QVariant
int ElementsLocation::MetaTypeId = qRegisterMetaType<ElementsLocation>("ElementsLocation");

/**
 * @brief ElementsLocation::ElementsLocation
 * Constructor
 */
ElementsLocation::ElementsLocation()
{}

/**
	Constructeur
	@param p Chemin de l'emplacement de l'element
	@param pr Projet de l'emplacement de l'element
*/
ElementsLocation::ElementsLocation(const QString &path, QETProject *project) :
	m_project(project)
{
	setPath(path);
}

/**
	Destructeur
*/
ElementsLocation::~ElementsLocation() {
}

/**
	Constructeur de copie
	@param other Autre emplacement d'element a copier
*/
ElementsLocation::ElementsLocation(const ElementsLocation &other) :
	m_collection_path(other.m_collection_path),
	m_file_system_path(other.m_file_system_path),
	m_project(other.m_project)
{}

/**
 * @brief ElementsLocation::ElementLocation
 * Constructor, build an ElementLocation from a QMimeData, the mime data format
 * must be "application/x-qet-element-uri" or "application/x-qet-category-uri".
 * This location can be null even if format is valid.
 * @param data
 */
ElementsLocation::ElementsLocation(const QMimeData *data)
{
	if (data->hasFormat("application/x-qet-element-uri") || data->hasFormat("application/x-qet-category-uri"))
		setPath(data->text());
}

/**
	Operateur d'affectation
	@param other Autre emplacement d'element a affecter
*/
ElementsLocation &ElementsLocation::operator=(const ElementsLocation &other) {
	m_collection_path = other.m_collection_path;
	m_file_system_path = other.m_file_system_path;
	m_project = other.m_project;
	return(*this);
}

/**
	Operateur de comparaison
	@param other Autre emplacement d'element a comparer
	@return true si other et cet ElementsLocation sont identiques, false sinon
*/
bool ElementsLocation::operator==(const ElementsLocation &other) const {
	return(
		m_collection_path == other.m_collection_path &&\
		m_project == other.m_project
	);
}

/**
	Operateur de comparaison
	@param other Autre emplacement d'element a comparer
	@return true si other et cet ElementsLocation sont differents, false sinon
*/
bool ElementsLocation::operator!=(const ElementsLocation &other) const {
	return(
		m_collection_path != other.m_collection_path ||\
		m_project != other.m_project
	);
}

/**
 * @brief ElementsLocation::baseName
 * @return The base name of the element or directory.
 * Unlike ElementsLocation::fileName , this method don't return the extension name.
 * For exemple if this location represent an element they return myElement.
 * @see fileName()
 */
QString ElementsLocation::baseName() const {
	QRegExp regexp("^.*([^/]+)\\.elmt$");
	if (regexp.exactMatch(m_collection_path)) {
		return(regexp.capturedTexts().at(1));
	}
	return(QString());
}

/**
 * @brief ElementsLocation::collectionPath
 * Return the path of the represented element relative to collection
 * if @protocol is true the path is prepended by the collection type (common://, custom:// or embed://)
 * else if false, only the collection path is returned without the collection type.
 * @param protocol
 * @return the path
 */
QString ElementsLocation::collectionPath(bool protocol) const
{
	if (protocol)
		return m_collection_path;
	else
	{
		QString path = m_collection_path;
		return path.remove(QRegularExpression("common://|custom://|embed://"));
	}
}

/**
 * @brief ElementsLocation::projectCollectionPath
 * @return The path is in form : project0+embed://dir/subdir/myElement.elmt
 * If this item represent a file system thing, return a null QString;
 */
QString ElementsLocation::projectCollectionPath() const
{
	if (isFileSystem())
		return QString();
	else
		return QString("project" + QString::number(QETApp::projectId(m_project)) + "+" + collectionPath());
}

/**
 * @brief ElementsLocation::fileSystemPath
 * @return The file system path of this element, (the separator is always '/' see QDir::toNativeSeparators())
 * If this element is embedded in a project return an empty string;
 */
QString ElementsLocation::fileSystemPath() const
{
	if (!m_project)
		return m_file_system_path;
	else
		return QString();
}

/**
 * @brief ElementsLocation::path
 * @return The path of this location.
 * OBSOLETE, use instead collectionPath(true)
 */
QString ElementsLocation::path() const {
	return(m_collection_path);
}

/**
 * @brief ElementsLocation::setPath
 * Set the path of this item.
 * The path can be relative to a collection (start by common:// , custom:// or embed://) or not.
 * @param path
 */
void ElementsLocation::setPath(const QString &path)
{
		QString tmp_path = path;
#ifdef Q_OS_WIN32
		//On windows, we convert backslash to slash
	tmp_path = QDir::fromNativeSeparators(path);

#endif

		//There is a project, the path is for an embedded coolection.
	if (m_project)
	{
		m_collection_path = path;
			//Add the protocol to the collection path
		if (!path.startsWith("embed://"))
			m_collection_path.prepend("embed://");

	}

		//The path start with project, we get the project and the path from the string
	else if (tmp_path.startsWith("project"))
	{
		QRegExp rx("^project([0-9]+)\\+(embed:\\/\\/.*)$", Qt::CaseInsensitive);
		if (rx.exactMatch(tmp_path))
		{
			bool conv_ok;
			uint project_id = rx.capturedTexts().at(1).toUInt(&conv_ok);
			if (conv_ok)
			{
				QETProject *project = QETApp::project(project_id);
				if (project)
				{
					m_collection_path = rx.capturedTexts().at(2);
					m_project = project;
				}
			}
		}
	}

		//The path is in file system, the given path is relative to common or custom collection
	else if (path.startsWith("common://") || path.startsWith("custom://"))
	{
		QString p;
		if (path.startsWith("common://"))
		{
			tmp_path.remove("common://");
			p = QETApp::commonElementsDirN() + "/" + tmp_path;
		}
		else
		{
			tmp_path.remove("custom://");
			p = QETApp::customElementsDirN() + "/" + tmp_path;
		}

		m_file_system_path = p;
		m_collection_path = path;
	}
		//In this case, the path is supposed to be relative to the file system.
	else
	{
		QString path_ = path;
		if(path_.endsWith(".elmt"))
		{
			m_file_system_path = path_;
			if (path_.startsWith(QETApp::commonElementsDirN()))
			{
				path_.remove(QETApp::commonElementsDirN()+="/");
				path_.prepend("common://");
				m_collection_path = path_;
			}
			else if (path_.startsWith(QETApp::customElementsDirN()))
			{
				path_.remove(QETApp::customElementsDirN()+="/");
				path_.prepend("custom://");
				m_collection_path = path_;
			}
		}
		else
		{
			m_file_system_path = path_;
			if (path_.startsWith(QETApp::commonElementsDirN()))
			{
				path_.remove(QETApp::commonElementsDirN()+="/");
				path_.prepend("common://");
				m_collection_path = path_;
			}
			else if (path_.startsWith(QETApp::customElementsDirN()))
			{
				path_.remove(QETApp::customElementsDirN()+="/");
				path_.prepend("custom://");
				m_collection_path = path_;
			}
		}
	}
}

/**
 * @brief ElementsLocation::addToPath
 * Add a string to the actual path of this location
 * @param string
 * @return True if the operation success
 */
bool ElementsLocation::addToPath(const QString &string)
{
	if (m_collection_path.endsWith(".elmt", Qt::CaseInsensitive))
	{
		qDebug() << "ElementsLocation::addToPath : Can't add string to the path of an element";
		return(false);
	}

	QString added_path = string;

	if (!m_collection_path.endsWith("/") && !added_path.startsWith("/"))
		added_path.prepend("/");

	if (isFileSystem())
		m_file_system_path += added_path;

	m_collection_path += added_path;
	return(true);
}

/**
	@return the location of the parent category, or a copy of this location
	when it represents a root category.
*/
ElementsLocation ElementsLocation::parent() const {
	ElementsLocation copy(*this);
	QRegExp re1("^([a-z]+://)(.*)/*$");
	if (re1.exactMatch(m_collection_path)) {
		QString path_proto = re1.capturedTexts().at(1);
		QString path_path = re1.capturedTexts().at(2);
		QString parent_path = path_path.remove(QRegExp("/*[^/]+$"));
		copy.setPath(path_proto + parent_path);
	}
	return(copy);
}

/**
	@return le projet de cet emplacement ou 0 si celui-ci n'est pas lie a
	un projet.
*/
QETProject *ElementsLocation::project() const {
	return(m_project);
}

/**
	@param project le nouveau projet pointe par cet emplacement
	Indiquer 0 pour que cet emplacement ne soit plus lie a un projet.
*/
void ElementsLocation::setProject(QETProject *project) {
	m_project = project;
}

/**
	@return true si l'emplacement semble utilisable (chemin virtuel non vide).
*/
bool ElementsLocation::isNull() const {
	return(m_collection_path.isEmpty());
}

/**
	@return Une chaine de caracteres representant l'emplacement
*/
QString ElementsLocation::toString() const {
	QString result;
	if (m_project) {
		int project_id = QETApp::projectId(m_project);
		if (project_id != -1) {
			result += "project" + QString().setNum(project_id) + "+";
		}
	}
	result += m_collection_path;
	return(result);
}

/**
 * @brief ElementsLocation::isElement
 * @return true if this location represent an element
 */
bool ElementsLocation::isElement() const {
	return m_collection_path.endsWith(".elmt");
}

/**
 * @brief ElementsLocation::isDirectory
 * @return true if this location represent a directory
 */
bool ElementsLocation::isDirectory() const {
	return (!isElement() && !m_collection_path.isEmpty());
}

/**
 * @brief ElementsLocation::isFileSystem
 * @return
 */
bool ElementsLocation::isFileSystem() const
{
	if (m_project) return false;
	if (m_file_system_path.isEmpty()) return false;
	return true;
}

/**
 * @brief ElementsLocation::isProject
 * @return True if this location represent an item from a project.
 */
bool ElementsLocation::isProject() const
{
	if (m_project && !m_collection_path.isEmpty())
		return true;
	else
		return false;
}

/**
 * @brief ElementsLocation::exist
 * @return True if this location represent an existing directory or element.
 */
bool ElementsLocation::exist() const
{
	if (m_project)
	{
		return m_project->embeddedElementCollection()->exist(collectionPath(false));
	}
	else
	{
		if (fileSystemPath().isEmpty()) return false;

		if (isDirectory())
		{
			QDir dir(fileSystemPath());
			return dir.exists();
		}
		else if (isElement())
			return QFile::exists(fileSystemPath());
		else
			return false;
	}
}

/**
 * @brief ElementsLocation::isWritable
 * @return True if this element can be writable (can use set xml)
 */
bool ElementsLocation::isWritable() const
{
	if (m_project)
		return !m_project->isReadOnly();
	else if (isFileSystem())
	{
		if (fileSystemPath().startsWith(QETApp::commonElementsDirN()))
			return false;
		else
			return true;
	}
	return false;
}

/**
 * @brief ElementsLocation::projectCollection
 * @return If this location represente a item in an embedded project collection, return this collection
 * else return nullptr.
 */
XmlElementCollection *ElementsLocation::projectCollection() const
{
	if (m_project)
		return m_project->embeddedElementCollection();
	else
		return nullptr;
}

/**
 * @brief ElementsLocation::nameList
 * @return the namelist of the represented element or directory.
 * If namelist can't be set, return a empty namelist
 */
NamesList ElementsLocation::nameList()
{
	NamesList nl;

	if (isElement())
		nl.fromXml(xml());

	if (isDirectory())
	{
		if (m_project)
			nl.fromXml(m_project->embeddedElementCollection()->directory(collectionPath(false)));
		else
		{
				//Open the qet_directory file, to get the traductions name of this dir
			QFile dir_conf(fileSystemPath() + "/qet_directory");
			if (dir_conf.exists() && dir_conf.open(QIODevice::ReadOnly | QIODevice::Text))
			{
					//Get the content of the file
				QDomDocument document;
				if (document.setContent(&dir_conf))
				{
					QDomElement root = document.documentElement();
					if (root.tagName() == "qet-directory")
						nl.fromXml(root);
				}
			}
		}
	}

	return nl;
}

/**
 * @brief ElementsLocation::xml
 * @return The definition of this element.
 * The definition can be null.
 */
QDomElement ElementsLocation::xml() const
{
	if (!m_project)
	{
		QFile file (m_file_system_path);
		QDomDocument docu;
		if (docu.setContent(&file))
			return docu.documentElement();
	}
	else
	{
		QString str = m_collection_path;
		if (isElement())
		{
			QDomElement element = m_project->embeddedElementCollection()->element(str.remove("embed://"));
			return element.firstChildElement("definition");
		}
		else
		{
			QDomElement element = m_project->embeddedElementCollection()->directory(str.remove("embed://"));
			return element;
		}
	}

	return QDomElement();
}

/**
 * @brief ElementsLocation::setXml
 * Replace the current xml description by @xml_element;
 * The document element of @xml_document must have tagname "definition" to be written
 * This definition must be writable
 * @param xml_element
 * @return true if success
 */
bool ElementsLocation::setXml(const QDomDocument &xml_document) const
{
	if (!isWritable())
		return false;

	if (xml_document.documentElement().tagName() != "definition")
	{
		qDebug() << "ElementsLocation::setXml : tag name of document element isn't 'definition'";
		return false;
	}

	if (isFileSystem())
	{
		QString error;
		QETXML::writeXmlFile(xml_document, fileSystemPath(), &error);

		if (!error.isEmpty()) {
			qDebug() << "ElementsLocation::setXml error : " << error;
			return false;
		}
		else {
			return true;
		}
	}
	else if (isProject())
	{
			//Element exist, we overwrite the existing element.
		if (exist())
		{
			QDomElement dom_element = xml();
			QDomNode parent_node = dom_element.parentNode();
			parent_node.removeChild(dom_element);
			parent_node.appendChild(xml_document.documentElement().cloneNode(true));
			return true;
		}
			//Element doesn't exist, we create the element
		else
		{
			QString path_ = collectionPath(false);
			QRegExp rx ("^(.*)/(.*\\.elmt)$");

			if (rx.exactMatch(path_)) {
				return project()->embeddedElementCollection()->addElementDefinition(rx.cap(1), rx.cap(2), xml_document.documentElement());
			}
			else {
				qDebug() << "ElementsLocation::setXml : rx don't match";
			}

		}
	}

	return false;
}

/**
 * @brief ElementsLocation::uuid
 * @return The uuid of the pointed element
 * Uuid can be null
 */
QUuid ElementsLocation::uuid() const
{
		//Get the uuid of element
	QList<QDomElement>  list_ = QET::findInDomElement(xml(), "uuid");

	if (!list_.isEmpty())
		return QUuid(list_.first().attribute("uuid"));

	return QUuid();
}

/**
 * @brief ElementLocation::icon
 * @return The icon of the represented element.
 * If icon can't be set, return a null QIcon
 */
QIcon ElementsLocation::icon() const
{
	if (!m_project)
	{
		ElementsCollectionCache *cache = QETApp::collectionCache();
		ElementsLocation loc(*this); //Make a copy of this to keep this method const
		if (cache->fetchElement(loc))
			return QIcon(cache->pixmap());
	}
	else
	{
		ElementFactory *factory = ElementFactory::Instance();
		int state;
		Element *elmt = factory->createElement(*this, 0, &state);

		if (state == 0)
			return QIcon(elmt->pixmap());
	}

	return QIcon();
}

/**
 * @brief ElementLocation::name
 * @return The name of the represented element in the current local
 */
QString ElementsLocation::name() const
{
	if (!m_project)
	{
		ElementsCollectionCache *cache = QETApp::collectionCache();
		ElementsLocation loc(*this); //Make a copy of this to keep this method const
		if (cache->fetchElement(loc))
			return cache->name();
		else
			return QString();
	}
	else
	{
		NamesList nl;
		nl.fromXml(xml());
		return nl.name(fileName());
	}
}

/**
 * @brief ElementLocation::fileName
 * @return Return the file name of the represented item, whatever the storage system (file system, xml collection)
 * with is file extension.
 * For example if this location represent an element, they return myElement.elmt.
 * For a directory return myDirectory.
 * @see baseName
 */
QString ElementsLocation::fileName() const
{
	if (m_collection_path.isEmpty()) return QString();

	QStringList qsl = m_collection_path.split("/");
	if (qsl.isEmpty()) return QString();
	else return qsl.last();
}

/**
	@param location A standard element location
	@return a hash identifying this location
*/
uint qHash(const ElementsLocation &location) {
	return(qHash(location.toString()));
}
