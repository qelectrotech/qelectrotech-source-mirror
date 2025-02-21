/*
	Copyright 2006-2025 The QElectroTech Team
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

#include "../elementscollectioncache.h"
#include "../factory/elementpicturefactory.h"
#include "../qetapp.h"
#include "../qetgraphicsitem/element.h"
#include "../qetproject.h"
#include "../qetxml.h"
#include "xmlelementcollection.h"

#include <QPicture>

// make this class usable with QVariant
int ElementsLocation::MetaTypeId = qRegisterMetaType<ElementsLocation>("ElementsLocation");

/**
	@brief ElementsLocation::ElementsLocation
	Constructor
*/
ElementsLocation::ElementsLocation()
{}

/**
	@brief ElementsLocation::ElementsLocation
	\~ @param path : Item location path
	\~French Chemin de l'emplacement de l'element
	\~ @param project : Project of the location of the element
	\~French Projet de l'emplacement de l'element
*/
ElementsLocation::ElementsLocation(const QString &path, QETProject *project) :
	m_project(project)
{
	setPath(path);
}

/**
	@brief ElementsLocation::~ElementsLocation
	Destructeur
*/
ElementsLocation::~ElementsLocation()
{
}

/**
	@brief ElementsLocation::ElementsLocation
	The copy constructor
	\~French Constructeur de copie
	\~ @param other : Alternate item location to copy
	\~French Autre emplacement d'element a copier
*/
ElementsLocation::ElementsLocation(const ElementsLocation &other) :
	m_collection_path(other.m_collection_path),
	m_file_system_path(other.m_file_system_path),
	m_project(other.m_project)
{}

/**
	@brief ElementsLocation::ElementLocation
	Constructor, build an ElementLocation from a QMimeData,
	the mime data format must be "application/x-qet-element-uri"
	or "application/x-qet-category-uri".
	This location can be null even if format is valid.
	@param data
*/
ElementsLocation::ElementsLocation(const QMimeData *data)
{
	if (data->hasFormat("application/x-qet-element-uri")
			|| data->hasFormat("application/x-qet-category-uri"))
		setPath(data->text());
}

/**
	@brief ElementsLocation::operator =
	Assignment operator
	\~French Operateur d'affectation
	\~ @param other :
	Other item location to assign
	\~French Autre emplacement d'element a affecter
	\~ @return *this ElementsLocation
*/
ElementsLocation &ElementsLocation::operator=(const ElementsLocation &other) {
	m_collection_path = other.m_collection_path;
	m_file_system_path = other.m_file_system_path;
	m_project = other.m_project;
	return(*this);
}

/**
	@brief ElementsLocation::operator ==
	\~French Operateur de comparaison
	\~ @param other : other item location to compare
	\~French Autre emplacement d'element a comparer
	\~ @return true if other and this ElementsLocation are identical,
	false otherwise
	\~French true si other et cet ElementsLocation sont identiques,
	false sinon
*/
bool ElementsLocation::operator==(const ElementsLocation &other) const
{
	return(
		m_collection_path == other.m_collection_path &&\
		m_project == other.m_project
	);
}

/**
	@brief ElementsLocation::operator !=
	Operateur de comparaison
	@param other Autre emplacement d'element a comparer
	@return true si other et cet ElementsLocation sont differents,
	false sinon
*/
bool ElementsLocation::operator!=(const ElementsLocation &other) const
{
	return(
		m_collection_path != other.m_collection_path ||\
		m_project != other.m_project
	);
}

/**
	@brief ElementsLocation::baseName
	@return The base name of the element or directory.
	Unlike ElementsLocation::fileName,
	this method don't return the extension name.
	For example if this location represent an element they return myElement.
	@see fileName()
*/
QString ElementsLocation::baseName() const
{
	QRegularExpression regexp("^.*(?<name>[^/]+)\\.elmt$");
	if (!regexp.isValid())
	{
		qWarning() <<"this is an error in the code"
			  << regexp.errorString()
			  << regexp.patternErrorOffset();
		return QString();
	}

	QRegularExpressionMatch match = regexp.match(m_collection_path);
	if (!match.hasMatch())
	{
		qDebug()<<"no Match => return"
			<<m_collection_path;
		return QString();
	}
	return match.captured("name");
}

/**
	@brief ElementsLocation::collectionPath
	Return the path of the represented element relative to collection
	if protocol is true the path is prepended by
	the collection type (common://, company://, custom:// or embed://)
	else if false,
	only the collection path is returned without the collection type.
	@param protocol
	@return the path
*/
QString ElementsLocation::collectionPath(bool protocol) const
{
	if (protocol)
		return m_collection_path;
	else
	{
		QString path = m_collection_path;
		return path.remove(QRegularExpression("common://|company://|custom://|embed://"));
	}
}

/**
	@brief ElementsLocation::projectCollectionPath
	@return The path is in form : project0+embed://dir/subdir/myElement.elmt
	If this item represent a file system thing, return a null QString;
*/
QString ElementsLocation::projectCollectionPath() const
{
	if (isFileSystem())
		return QString();
	else
		return QString("project"
				   + QString::number(QETApp::projectId(m_project))
				   + "+"
				   + collectionPath());
}

/**
	@brief ElementsLocation::fileSystemPath
	@return The file system path of this element,
	(the separator is always '/' see QDir::toNativeSeparators())
	If this element is embedded in a project return an empty string;
*/
QString ElementsLocation::fileSystemPath() const
{
	if (!m_project)
		return m_file_system_path;
	else
		return QString();
}

/**
	@brief ElementsLocation::path
	@return The path of this location.
	@deprecated use instead collectionPath(true)
*/
QString ElementsLocation::path() const
{
	return(m_collection_path);
}

/**
	@brief ElementsLocation::setPath
	Set the path of this item.
	The path can be relative to a collection
	(start by common://, company://, custom:// or embed://) or not.
	@param path
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
		QRegularExpression re
			("^project(?<project_id>[0-9])\\+(?<collection_path>embed://*.*)$");
		if (!re.isValid())
		{
			qWarning() <<QObject::tr("this is an error in the code")
				  << re.errorString()
				  << re.patternErrorOffset();
			return;
		}
		QRegularExpressionMatch match = re.match(tmp_path);
		if (!match.hasMatch())
		{
			qDebug()<<"no Match => return"
				   <<tmp_path;
			return;
		}
		bool conv_ok;
		uint project_id = match.captured("project_id").toUInt(&conv_ok);
		if (!conv_ok)
		{
			qWarning()<<"toUint failed"
				 <<match.captured("project_id")
				 <<re
				 <<tmp_path;
			return;
		}
		QETProject *project = QETApp::project(project_id);
		if (project)
		{
			m_collection_path = match.captured("collection_path");
			m_project = project;
		}
	}

	// The path is in file system,
	// the given path is relative to common or custom collection
	else if (path.startsWith("common://") || path.startsWith("company://") || path.startsWith("custom://"))
	{
		QString p;
		if (path.startsWith("common://"))
		{
			tmp_path.remove("common://");
			p = QETApp::commonElementsDirN() + "/" + tmp_path;
		}
		else if (path.startsWith("company://"))
		{
			tmp_path.remove("company://");
			p = QETApp::companyElementsDirN() + "/" + tmp_path;
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
			else if (path_.startsWith(QETApp::companyElementsDirN()))
			{
				path_.remove(QETApp::companyElementsDirN()+="/");
				path_.prepend("company://");
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
			else if (path_.startsWith(QETApp::companyElementsDirN()))
			{
				path_.remove(QETApp::companyElementsDirN()+="/");
				path_.prepend("company://");
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
	@brief ElementsLocation::addToPath
	Add a string to the actual path of this location
	@param string
	@return True if the operation success
*/
bool ElementsLocation::addToPath(const QString &string)
{
	if (m_collection_path.endsWith(".elmt", Qt::CaseInsensitive))
	{
		qDebug() << "ElementsLocation::addToPath :"
				" Can't add string to the path of an element";
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
	@brief ElementsLocation::parent
	@return the location of the parent category, or a copy of this location
	when it represents a root category.
*/
ElementsLocation ElementsLocation::parent() const
{
	ElementsLocation copy(*this);
	QRegularExpression re ("^(?<path_proto>[a-z]+://.*)/.*$");
	if (!re.isValid())
	{
		qWarning()
			<<QObject::tr("this is an error in the code")
			<< re.errorString()
			<< re.patternErrorOffset();
	}
	QRegularExpressionMatch match = re.match(m_collection_path);
	if (!match.hasMatch())
	{
		qDebug()
			<<"no Match => return"
			<<m_collection_path;
	}else {
		copy.setPath(match.captured("path_proto"));
	}
	return(copy);
}

/**
	@brief ElementsLocation::project
	@return
	the project of this location or 0 if it is not linked to a project.
	\~French
	le projet de cet emplacement ou 0 si celui-ci n'est pas lie a un projet.
*/
QETProject *ElementsLocation::project() const
{
	return(m_project);
}

/**
	@brief ElementsLocation::setProject
	@param project :
	the new project points to this location
	Indicate 0 so that this location is no longer linked to a project.
	\~French le nouveau projet pointe par cet emplacement
	Indiquer 0 pour que cet emplacement ne soit plus lie a un projet.
*/
void ElementsLocation::setProject(QETProject *project) {
	m_project = project;
}

/**
	@brief ElementsLocation::isNull
	@return true if the location seems usable (virtual path not empty).
	\~French
	true si l'emplacement semble utilisable (chemin virtuel non vide).
*/
bool ElementsLocation::isNull() const
{
	return(m_collection_path.isEmpty());
}

/**
	@brief ElementsLocation::toString
	@return A character string representing the location
	\~French Une chaine de caracteres representant l'emplacement
*/
QString ElementsLocation::toString() const
{
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
	@brief ElementsLocation::isElement
	@return true if this location represent an element
*/
bool ElementsLocation::isElement() const
{
	return m_collection_path.endsWith(".elmt");
}

/**
	@brief ElementsLocation::isDirectory
	@return true if this location represent a directory
*/
bool ElementsLocation::isDirectory() const
{
	return (!isElement() && !m_collection_path.isEmpty());
}

/**
	@brief ElementsLocation::isFileSystem
	@return true if
*/
bool ElementsLocation::isFileSystem() const
{
	if (m_project) return false;
	if (m_file_system_path.isEmpty()) return false;
	return true;
}

/**
	@brief ElementsLocation::isCommonCollection
	@return
	True if this location represent an item from the common collection
*/
bool ElementsLocation::isCommonCollection() const
{
	return fileSystemPath().startsWith(QETApp::commonElementsDirN());
}

/**
	@brief ElementsLocation::isCompanyCollection
	@return
	True if this location represent an item from the company collection
*/
bool ElementsLocation::isCompanyCollection() const
{
	return fileSystemPath().startsWith(QETApp::companyElementsDirN());
}

/**
	@brief ElementsLocation::isCustomCollection
	@return
	True if this location represent an item from the custom collection
*/
bool ElementsLocation::isCustomCollection() const
{
	return fileSystemPath().startsWith(QETApp::customElementsDirN());
}

/**
	@brief ElementsLocation::isProject
	@return True if this location represent an item from a project.
*/
bool ElementsLocation::isProject() const
{
	if (m_project && !m_collection_path.isEmpty())
		return true;
	else
		return false;
}

/**
	@brief ElementsLocation::exist
	@return
	True if this location represent an existing directory or element.
*/
bool ElementsLocation::exist() const
{
	if (m_project)
	{
		return m_project->embeddedElementCollection()
				->exist(collectionPath(false));
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
	@brief ElementsLocation::isWritable
	@return True if this element can be writable (can use set xml)
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
	@brief ElementsLocation::projectCollection
	@return
	If this location represents an item in an embedded project collection,
	return this collection else return nullptr.
*/
XmlElementCollection *ElementsLocation::projectCollection() const
{
	if (m_project)
		return m_project->embeddedElementCollection();
	else
		return nullptr;
}

/**
	@brief ElementsLocation::nameList
	@return the namelist of the represented element or directory.
	If namelist can't be set, return a empty namelist
*/
NamesList ElementsLocation::nameList()
{
	NamesList nl;

	if (isElement()) {
		nl.fromXml(pugiXml());
	}

	if (isDirectory())
	{
		if (m_project)
			nl.fromXml(m_project->embeddedElementCollection()
				   ->directory(collectionPath(false)));
		else
		{
			//Open the qet_directory file,
			// to get the traductions name of this dir
			QFile dir_conf(fileSystemPath() + "/qet_directory");
			if (dir_conf.exists() && dir_conf.open(
						QIODevice::ReadOnly
						| QIODevice::Text))
			{
					//Get the content of the file
				QDomDocument document;
				if (document.setContent(&dir_conf))
				{
					QDomElement root = document
							.documentElement();
					if (root.tagName() == "qet-directory")
						nl.fromXml(root);
				}
			}
		}
	}

	return nl;
}

/**
	@brief ElementsLocation::xml
	@return The definition of this element or directory.
	The definition can be null.
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
			QDomElement element = m_project
					->embeddedElementCollection()
					->element(str.remove("embed://"));
			return element.firstChildElement("definition");
		}
		else
		{
			QDomElement element = m_project
					->embeddedElementCollection()
					->directory(str.remove("embed://"));
			return element;
		}
	}
	return QDomElement();
}

/**
	@brief ElementsLocation::pugiXml
	@return the xml document of this element or directory
	The definition can be null
*/
pugi::xml_document ElementsLocation::pugiXml() const
{
	pugi::xml_document docu; //empty xml_document();
	/* Except for linux OS (because linux keep in cache the file),
	 * we keep in memory the xml
	 * to avoid multiple access to file.
	 * keep in memory the XML,
	 * consumes a little more RAM,
	 * for this reason we don't use it for linux to minimize the RAM footprint.
	 */
#ifndef Q_OS_LINUX
	if (!m_string_stream.str().empty())
	{
		docu.load_string(m_string_stream.str().c_str());
		return docu;
	}
#endif
	if (!m_project)
	{
#ifndef Q_OS_LINUX
		if (docu.load_file(m_file_system_path.toStdString().c_str())) {
			docu.save(m_string_stream);
		}
#else
		docu.load_file(m_file_system_path.toStdString().c_str());
#endif
	}
	else
	{
			//Get the xml dom from Qt xml and copie to pugi xml
		QDomDocument qdoc;
		QString str = m_collection_path;
		if (isElement()) {
			QDomElement element = m_project->embeddedElementCollection()->element(str.remove("embed://"));
			qdoc.appendChild(qdoc.importNode(element.firstChildElement("definition"),true));
		} else {
			QDomElement element = m_project->embeddedElementCollection()->directory(str.remove("embed://"));
			qdoc.appendChild(qdoc.importNode(element, true));
		}
		docu.load_string(qdoc.toString(4).toStdString().c_str());
	}
	return docu;
}

/**
	@brief ElementsLocation::setXml
	Replace the current xml description by xml_document;
	The document element of xml_document must have
	tagname "definition" to be written
	This definition must be writable
	@param xml_document
	@return true if success
*/
bool ElementsLocation::setXml(const QDomDocument &xml_document) const
{
	if (!isWritable())
		return false;

	if (xml_document.documentElement().tagName() != "definition")
	{
		qDebug() << "ElementsLocation::setXml :"
				" tag name of document element isn't 'definition'";
		return false;
	}

	if (isFileSystem())
	{
		QString error;
		QETXML::writeXmlFile(xml_document, fileSystemPath(), &error);

		if (!error.isEmpty()) {
			qDebug() << "ElementsLocation::setXml error : "
				 << error;
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
			parent_node.appendChild(xml_document
						.documentElement()
						.cloneNode(true));
			return true;
		}
		//Element doesn't exist, we create the element
		else
		{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)	// ### Qt 6: remove
			QString path_ = collectionPath(false);
			QRegExp rx ("^(.*)/(.*\\.elmt)$");

			if (rx.exactMatch(path_)) {
				return project()
						->embeddedElementCollection()
						->addElementDefinition(
							rx.cap(1),
							rx.cap(2),
							xml_document
							.documentElement());
			}
			else {
				qDebug() << "ElementsLocation::setXml :"
						" rx don't match";
			}
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#		pragma message("@TODO ad Core5Compat to Cmake")
#endif
			qDebug() << "Help code for QT 6 or later";

			QString			   path_ = collectionPath(false);
			QRegularExpression rx(QRegularExpression::anchoredPattern("(.*)/(.*\\.elmt)"));
			QRegularExpressionMatch match = rx.match(path_);
			if (match.hasMatch())
			{
				return project()
					->embeddedElementCollection()
					->addElementDefinition(
						match.captured(1),
						match.captured(2),
						xml_document.documentElement());
			}
			else
			{
				qDebug() << "ElementsLocation::setXml :"
							" rx don't match";
			}
#endif
		}
	}

	return false;
}

/**
	@brief ElementsLocation::uuid
	@return The uuid of the pointed element
	Uuid can be null
*/
QUuid ElementsLocation::uuid() const
{
	if (!isElement()) {
		return QUuid();
	}

	auto document = pugiXml();
	auto uuid_node = document.document_element().child("uuid");
	if (uuid_node.empty()) {
		return QUuid();
	}
	return QUuid(uuid_node.attribute("uuid").as_string());
}

/**
	@brief ElementLocation::icon
	@return The icon of the represented element.
	If icon can't be set, return a null QIcon
*/
QIcon ElementsLocation::icon() const
{
	if (!m_project)
	{
		ElementsCollectionCache *cache = QETApp::collectionCache();
		// Make a copy of this to keep this method const
		ElementsLocation loc(*this);
		if (cache->fetchElement(loc))
			return QIcon(cache->pixmap());
	} else {
		return QIcon(ElementPictureFactory::instance()->pixmap(*this));
	}

	return QIcon();
}

/**
	@brief ElementLocation::name
	@return The name of the represented element in the current locale
*/
QString ElementsLocation::name() const
{
	NamesList nl;
	nl.fromXml(pugiXml().document_element());
	return nl.name(fileName());
}

/**
	@brief ElementLocation::fileName
	@return Return the file name of the represented item,
	whatever the storage system (file system, xml collection)
	with is file extension.
	For example if this location represent an element,
	they return myElement.elmt.
	For a directory return myDirectory.
	@see baseName
*/
QString ElementsLocation::fileName() const
{
	if (m_collection_path.isEmpty())
		return QString();

	QStringList qsl = m_collection_path.split("/");
	if (qsl.isEmpty())
		return QString();
	else
		return qsl.last();
}

/**
	@brief ElementsLocation::elementInformations
	@return
	the element information of the element represented by this location.
	If the location is a directory, the returned diagram context is empty
*/
DiagramContext ElementsLocation::elementInformations() const
{
	DiagramContext context;
	if (isDirectory()) {
		return context;
	}
	context.fromXml(pugiXml().document_element().child(
				"elementInformations"),
			"elementInformation");
	return  context;
}

/**
	@brief operator <<
	debug for processing ElementsLocation
	@param debug
	@param location
	@return debug msg
*/
QDebug operator<< (QDebug debug, const ElementsLocation &location)
{
	QDebugStateSaver saver(debug);
	debug.noquote();

	QString msg;
	msg += "ElementsLocation(";
	msg += (location.isProject()? location.projectCollectionPath()
					: location.collectionPath(true));
	msg += location.exist()? ", true" : ", false";
	msg +=")";

	debug << msg;

	return debug;
}
