/*
	Copyright 2006-2021 The QElectroTech Team
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
#include "xmlelementcollection.h"

#include "../NameList/nameslist.h"
#include "../qetproject.h"
#include "../qetxml.h"
#include "elementslocation.h"

/**
	@brief XmlElementCollection::XmlElementCollection
	Build an empty collection.
	The collection start by :
	@code{xml}
		<collection>
			<category name="import>
			</category>
		</collection>
	@endcode
	All elements and category are stored as child of <category name="import>
	@param project : the project of this collection
*/
XmlElementCollection::XmlElementCollection(QETProject *project) :
	QObject(project),
	m_project(project)
{
	QDomElement collection = m_dom_document.createElement("collection");
	m_dom_document.appendChild(collection);
	QDomElement import = m_dom_document.createElement("category");
	import.setAttribute("name", "import");
	collection.appendChild(import);

	NamesList names;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0) // ### Qt 6: remove

	const QChar russian_data[24] = {
		0x0418, 0x043C, 0x043F, 0x043E, 0x0440, 0x0442, 0x0438, 0x0440,
		0x043E, 0x0432, 0x0430, 0x043D, 0x043D, 0x044B, 0x0435, 0x0020,
		0x044D, 0x043B, 0x0435, 0x043C, 0x0435, 0x043D, 0x0442, 0x044B};
	const QChar greek_data[18] = {
		0x0395,
		0x03b9,
		0x03c3,
		0x03b7,
		0x03b3,
		0x03bc,
		0x03ad,
		0x03bd,
		0x03b1,
		0x0020,
		0x03c3,
		0x03c4,
		0x03bf,
		0x03b9,
		0x03c7,
		0x03b5,
		0x03af,
		0x03b1};
	const QChar turkish_data[12] = {
		0x0130,
		0x0074,
		0x0068,
		0x0061,
		0x006C,
		0x0020,
		0x00F6,
		0x011F,
		0x0065,
		0x006C,
		0x0065,
		0x0072};
	names.addName("fr", "Éléments importés");
	names.addName("en", "Imported elements");
	names.addName("de", "Importierte elemente");
	names.addName("es", "Elementos importados");
	names.addName("ru", QString(russian_data, 24));
	names.addName("cs", "Zavedené prvky");
	names.addName("pl", "Elementy importowane");
	names.addName("pt", "elementos importados");
	names.addName("it", "Elementi importati");
	names.addName("el", QString(greek_data, 18));
	names.addName("nl", "Elementen geïmporteerd");
	names.addName("hr", "Uvezeni elementi");
	names.addName("ca", "Elements importats");
	names.addName("ro", "Elemente importate");
	names.addName("tr", QString(turkish_data, 12));
	names.addName("da", "Importerede elementer");
	names.addName("hr", "Uvezeni elementi");
	names.addName("sl", "Uvoženi elementi");
#else
#	if TODO_LIST
#		pragma message("@TODO remove code for QT 6 or later")
#	endif
	qDebug() << "Help code for QT 6 or later";
	names.addName("fr", "Éléments importés");
	names.addName("en", "Imported elements");
	names.addName("de", "Importierte elemente");
	names.addName("es", "Elementos importados");
	names.addName("cs", "Zavedené prvky");
	names.addName("pl", "Elementy importowane");
	names.addName("pt", "elementos importados");
	names.addName("it", "Elementi importati");
	names.addName("nl", "Elementen geïmporteerd");
	names.addName("hr", "Uvezeni elementi");
	names.addName("ca", "Elements importats");
	names.addName("ro", "Elemente importate");
	names.addName("da", "Importerede elementer");
	names.addName("hr", "Uvezeni elementi");
	names.addName("sl", "Uvoženi elementi");
#endif

	import.appendChild(names.toXml(m_dom_document));
}

/**
	@brief XmlElementCollection::XmlElementCollection
	Constructor with an collection.
	The tagName of dom_element must be "collection"
	@param dom_element :
	-the collection in a dom_element (the dom element in cloned)
	@param project : the project of this collection
*/
XmlElementCollection::XmlElementCollection(const QDomElement &dom_element,
					   QETProject *project) :
	QObject(project),
	m_project(project)
{
	if (dom_element.tagName() == "collection")
		m_dom_document.appendChild(m_dom_document.importNode(
						   dom_element, true));
	else
		qDebug() << "XmlElementCollection : tagName of dom_element is not collection";
}

/**
	@brief XmlElementCollection::root
	The root is the first DOM-Element the xml collection, the tag name
	of the dom element is : collection
	@return The root QDomElement of the collection
*/
QDomElement XmlElementCollection::root() const
{
	return m_dom_document.documentElement();
}

/**
	@brief XmlElementCollection::importCategory
	@return The QDomElement import (the beginning of an xml collection) or
	a null QDomElement if doesn't exist.
*/
QDomElement XmlElementCollection::importCategory() const
{
	return root().firstChildElement("category");
}

/**
	@brief XmlElementCollection::childs
	@param parent_element
	@return All childs element in the parent_element tree
*/
QDomNodeList XmlElementCollection::childs(
		const QDomElement &parent_element) const
{
	if (parent_element.ownerDocument() != m_dom_document)
		return QDomNodeList();
	return parent_element.childNodes();
}

/**
	@brief XmlElementCollection::child
	If parent_element have child element
	with an attribute name = child_name, return it,
	else return a null QDomElement.
	Only search for element with tag-name "category" and "element"
	(if child_name end with ".elmt")
	@param parent_element : the parent DomElement where we search for child.
	parent_element must be a child node of this XmlElementCollection.
	@param child_name : name of child to search.
	@return The child QDomElement or a null QDomElement if not found
*/
QDomElement XmlElementCollection::child(const QDomElement &parent_element,
					const QString &child_name) const
{
	if (parent_element.ownerDocument() != m_dom_document)
		return QDomElement();

		//Get all childs element of parent_element
	QDomNodeList child_list = parent_element.childNodes();
	QString tag_name(child_name.endsWith(".elmt")? "element" : "category");
	QList <QDomElement> found_dom_element;
	for (int i=0 ; i<child_list.length() ; i++)
	{
		QDomElement child_element = child_list.item(i).toElement();
		if (child_element.tagName() == tag_name)
			found_dom_element << child_element;
	}

	if (found_dom_element.isEmpty()) return QDomElement();

	foreach (QDomElement elmt, found_dom_element)
		if (elmt.attribute("name") == child_name)
			return elmt;

	return QDomElement();
}

/**
	@brief XmlElementCollection::child
	@param path
	@return the DomElement at path if exist, else return a null QDomElement
*/
QDomElement XmlElementCollection::child(const QString &path) const
{
	QStringList path_list = path.split("/");
	if (path_list.isEmpty()) return QDomElement();

	QDomElement parent_element = root();
	foreach (QString str, path_list)
	{
		QDomElement child_element = child(parent_element, str);

		if (child_element.isNull())
			return QDomElement();
		else
			parent_element = child_element;
	}

	return parent_element;
}

/**
	@brief XmlElementCollection::directories
	@param parent_element
	@return A list of directory stored in parent_element
*/
QList<QDomElement> XmlElementCollection::directories(
		const QDomElement &parent_element) const
{
	QList <QDomElement> directory_list;
	QDomNodeList node_list = childs(parent_element);
	if (node_list.isEmpty()) return directory_list;

	for (int i=0 ; i < node_list.count() ; i++)
	{
		QDomNode node = node_list.at(i);
		if (node.isElement()
				&& node.toElement().tagName() == "category")
			directory_list << node.toElement();
	}

	return directory_list;
}

/**
	@brief XmlElementCollection::directoriesNames
	@param parent_element
	@return a list of names for every child directories of parent_element
*/
QStringList XmlElementCollection::directoriesNames(
		const QDomElement &parent_element) const
{
	QList <QDomElement> childs = directories(parent_element);
	QStringList names;

	foreach (QDomElement child, childs)
	{
		QString name = child.attribute("name");
		if (!name.isEmpty())
			names.append(name);
	}

	return names;
}

/**
	@brief XmlElementCollection::elements
	@param parent_element
	@return A list of element stored in parent_element
*/
QList<QDomElement> XmlElementCollection::elements(
		const QDomElement &parent_element) const
{
	QList <QDomElement> element_list;
	QDomNodeList node_list = childs(parent_element);
	if (node_list.isEmpty()) return element_list;

	for (int i=0 ; i < node_list.count() ; i++)
	{
		QDomNode node = node_list.at(i);
		if (node.isElement() && node.toElement().tagName() == "element")
			element_list << node.toElement();
	}

	return element_list;
}

/**
	@brief XmlElementCollection::elementsNames
	@param parent_element
	@return A list of names fr every childs element of parent_element
*/
QStringList XmlElementCollection::elementsNames(
		const QDomElement &parent_element) const
{
	QList <QDomElement> childs = elements(parent_element);
	QStringList names;

	foreach (QDomElement child, childs)
	{
		QString name = child.attribute("name");
		if (!name.isEmpty())
			names.append(name);
	}

	return names;
}

/**
	@brief XmlElementCollection::element
	@param path : path of the element in this collection
	@return the QDomElement that represent the element at path path
	or a null QDomElement if not found or doesn't represent an element
*/
QDomElement XmlElementCollection::element(const QString &path) const
{
	if (!path.endsWith(".elmt")) return QDomElement();

	QDomElement element = child(path);

	if (element.tagName() == "element")
		return element;
	else
		return QDomElement();
}

/**
	@brief XmlElementCollection::directory
	@param path : path of the directory in this collection
	@return the QDomElement that represent the directory at path path
	or a null QDomElement if not found.
*/
QDomElement XmlElementCollection::directory(const QString &path) const
{
	QDomElement directory = child(path);

	if (directory.tagName() == "category")
		return directory;
	else
		return QDomElement();
}

/**
	@brief XmlElementCollection::addElement
	Add the element at location to this collection.
	The element is copied in this collection
	in "import" dir with the same path, in other word
	if the path is dir1/dir2/dir3/myElement.elmt,
	myElement is copied to this collection at the
	path : import/dir1/dir2/dir3/myElement.elmt
	If the path doesn't exist, he was created.
	If the element already exist, do nothing.
	@param location : location of the element
	@return the collection path of the added item
	or a null QString if element can't be added.
*/
QString XmlElementCollection::addElement(ElementsLocation &location)
{
	//location must be an element and exist
	if (!(location.exist() && location.isElement()))
		return QString();

	//Add an element from this collection to this collection have no sense
	if (location.isProject() && location.projectCollection() == this)
		return QString();

	//First we check if this location exist in this collection if so, we do nothing
	if ( exist("import/" + location.collectionPath(false)) )
		return QString();

	//Get the import dir of this collection
	QDomElement parent_element = importCategory();
	if (parent_element.isNull())
		return QString();

	QString integrated_path = parent_element.attribute("name");

	//Split the path
	QStringList splitted_path = location.collectionPath(false).split("/");
	if (splitted_path.isEmpty())
		return QString();

	if (location.isFileSystem()) {
		//Get the root dir of the filesystem collection
		QDir dir(location.fileSystemPath().remove(
				 location.collectionPath(false)));
		if (!dir.exists())
			return QString();

		foreach(QString str, splitted_path) {
			QDomElement child_element = child(parent_element, str);

			//Child doesn't exist, we create it
			if (child_element.isNull()) {
				QDomElement created_child;

				//str is the path of an element, we integrate an element
				if (str.endsWith(".elmt")) {
					QFile element_file(dir.filePath(str));
					if (!element_file.exists())
						return QString();

					created_child = QETXML::
							fileSystemElementToXmlCollectionElement(
								m_dom_document,
								element_file);
				}

				//str is the path of a directory, we integrate a directory.
				else {
					//Dir doesn't exist.
					if (!dir.cd(str))
						return QString();

					created_child = QETXML::
							fileSystemDirToXmlCollectionDir(
								m_dom_document,
								dir);
				}

				if(created_child.isNull())
					return QString();

				parent_element.appendChild(created_child);
				parent_element = created_child;
			}
			//Child exist
			else {
				if (!dir.cd(str))
					return QString();

				parent_element = child_element;
			}

			integrated_path.append("/"+str);
		}
	}
	else if (location.isProject()) {
		QString path;
		foreach(QString str, splitted_path) {
			if (path.isEmpty())
				path = str;
			else
				path = path + "/" + str;

			QDomElement child_element = child(parent_element, str);

			//Child doesn't exist, we create it
			if (child_element.isNull()) {
				QDomElement created_child;

				//str is the path of an element, we integrate an element
				if (str.endsWith(".elmt")) {
					created_child = m_dom_document.createElement("element");
					created_child.setAttribute("name", str);

					created_child.appendChild(location.xml().cloneNode(true));
				}

				//str is the path of a directory, we integrate a directory.
				else {
					created_child = m_dom_document.createElement("category");
					created_child.setAttribute("name", str);

					ElementsLocation sub_dir_location(path, location.project());
					QDomElement names_element = sub_dir_location.nameList().toXml(m_dom_document);
					created_child.appendChild(names_element);
				}

				if(created_child.isNull())
					return QString();

				parent_element.appendChild(created_child);
				parent_element = created_child;
			}
			//Child exist
			else
				parent_element = child_element;

			integrated_path.append("/"+str);
		}
	}

	emit elementAdded(integrated_path);
	return integrated_path;
}

/**
	@brief XmlElementCollection::addElementDefinition
	Add the element definition xml_definition
	in the directory at path dir_path with the name elmt_name.
	@param dir_path :
	the path of the directory where we must add the element.
	The path must be an existing directory of this collection.
	@param elmt_name : The name used to store the element
	(the name must end with .elmt, if not, .elmt will be append to elmt_name)
	@param xml_definition : The xml definition of the element.
	The tag name of xml_definition must be "definition".
	@return True if the element is added with success.
*/
bool XmlElementCollection::addElementDefinition(
		const QString &dir_path,
		const QString &elmt_name,
		const QDomElement &xml_definition)
{
	QDomElement dom_dir = directory(dir_path);
	if (dom_dir.isNull()) {
		qDebug() << "XmlElementCollection::addElementDefinition : No directory at path : "
			 << dir_path;
		return false;
	}

	if (xml_definition.tagName() != "definition") {
		qDebug() << "XmlElementCollection::addElementDefinition : xml_defintion tag name is not \"definition\"";
		return false;
	}

	QString name = elmt_name;
	if (!name.endsWith(".elmt")) {
		name += ".elmt";
	}

	QDomElement dom_elmt = m_dom_document.createElement("element");
	dom_elmt.setAttribute("name", name);
	dom_elmt.appendChild(xml_definition.cloneNode(true));
	dom_dir.appendChild(dom_elmt);

	emit elementAdded(dir_path + "/" + name);

	return true;
}

/**
	@brief XmlElementCollection::removeElement
	Remove the element at path path.
	@param path
	@return True if element is removed and emit the signal elementRemoved.
	else false.
*/
bool XmlElementCollection::removeElement(const QString& path)
{
	QDomElement elmt = element(path);

	if (!elmt.isNull()) {
		elmt.parentNode().removeChild(elmt);
		emit elementRemoved(path);
		return true;
	}

	return false;
}

/**
	@brief XmlElementCollection::copy
	Copy the content represented by source (an element or a directory)
	to destination.
	Destination must be a directory of this collection.
	If the destination already have an item at the same path of source,
	he will be replaced by source.
	@param source : content to copy
	@param destination : destination of the copy, must be a directory
	of this collection
	@param rename : rename the copy with rename else use the name of source
	@param deep_copy : if true copy all childs of source
	(only if source is directory)
	@return the ElementLocation that represent the copy,
	if copy failed return a null ElementLocation
*/
ElementsLocation XmlElementCollection::copy(
		ElementsLocation &source,
		ElementsLocation &destination,
		const QString& rename,
		bool deep_copy)
{
	if (!(source.exist()
	      && destination.isDirectory()
	      && destination.isProject()
	      && destination.projectCollection() == this))
		return ElementsLocation();

	if (source.isElement())
		return copyElement(source, destination, rename);
	else
		return copyDirectory(source, destination, rename, deep_copy);
}

/**
	@brief XmlElementCollection::exist
	Return true if the path path exist in this collection
	@param path
	@return
*/
bool XmlElementCollection::exist(const QString &path) const
{
	if (child(path).isNull())
		return false;
	else
		return true;
}

/**
	@brief XmlElementCollection::createDir
	Create a child directory at path path with the name name.
	Emit directorieAdded if success.
	@param path : path of parent diectorie
	@param name : name of the directori to create.
	@param name_list : translation of the directory name.
	@return true if creation success,
	if directory already exists, return true.
*/
bool XmlElementCollection::createDir(const QString& path,
				     const QString& name,
				     const NamesList &name_list)
{
	QString new_dir_path = path + "/" + name;

	if (!directory(new_dir_path).isNull()) {
		return true;
	}

	QDomElement parent_dir = directory(path);
	if (parent_dir.isNull()) {
		qDebug() << "XmlElementCollection::createDir : directory at path doesn't exist";
		return false;
	}

	QDomElement new_dir = m_dom_document.createElement("category");
	new_dir.setAttribute("name", name);
	new_dir.appendChild(name_list.toXml(m_dom_document));

	parent_dir.appendChild(new_dir);

	emit directorieAdded(new_dir_path);

	return true;
}

/**
	@brief XmlElementCollection::removeDir
	Remove the directory at path path.
	@param path
	@return true if successfully removed and emit directoryRemoved(QString),
	else false.
*/
bool XmlElementCollection::removeDir(const QString& path)
{
	QDomElement dir = directory(path);
	if (!dir.isNull()) {
		dir.parentNode().removeChild(dir);
		emit directoryRemoved(path);
		return true;
	}
	return false;
}

/**
	@brief XmlElementCollection::elementsLocation
	Return all locations stored in dom_element (element and directory).
	If dom_element is null, return all location owned by this collection
	dom_element must be a child of this collection.
	@param dom_element : dom_element where we must search location.
	@param childs = if true return all childs location of dom_element,
	if false, only return the direct childs location of dom_element.
	@return
*/
QList<ElementsLocation> XmlElementCollection::elementsLocation(
		QDomElement dom_element,
		bool childs) const
{
	QList <ElementsLocation> location_list;

	if (dom_element.isNull())
		dom_element = m_dom_document.documentElement();

	if (dom_element.ownerDocument() != m_dom_document)
		return location_list;

		//get element childs
	QList <QDomElement> element_list = elements(dom_element);

	foreach (QDomElement elmt, element_list) {
		ElementsLocation location = domToLocation(elmt);
		if (location.exist())
			location_list << location;
	}

		//get directory childs
	QList <QDomElement> directory_list = directories(dom_element);

	foreach (QDomElement dir, directory_list) {
		ElementsLocation location = domToLocation(dir);
		if (location.exist())
			location_list << location;

		if (childs)
			location_list.append(elementsLocation(dir, childs));
	}

	return location_list;
}

/**
	@brief XmlElementCollection::domToLocation
	Return the element location who represent the xml element : dom_element
	dom_element must be owned by this collection
	@param dom_element :
	the dom_element of this collection that represent an element.
	The tag name of dom_element must be "element"
	@return the element location, location can be null if fail.
*/
ElementsLocation XmlElementCollection::domToLocation(
		QDomElement dom_element) const
{
	if (dom_element.ownerDocument() == m_dom_document) {
		QString path = dom_element.attribute("name");

		while (!dom_element.parentNode().isNull()
				&& dom_element.parentNode().isElement()) {
			dom_element = dom_element.parentNode().toElement();

			if (dom_element.tagName() == "category")
				path.prepend(dom_element.attribute("name") + "/");
		}

		return ElementsLocation(path, m_project);
	}
	else
		return ElementsLocation();
}

/**
	@brief XmlElementCollection::cleanUnusedElement
	Remove elements in this collection which is not used in the owner project
*/
void XmlElementCollection::cleanUnusedElement()
{
	foreach (ElementsLocation loc, m_project->unusedElements())
		removeElement(loc.collectionPath(false));
}

/**
	@brief XmlElementCollection::cleanUnusedDirectory
	Remove the empty directories of this collection
*/
void XmlElementCollection::cleanUnusedDirectory()
{
	QDomNodeList lst = importCategory().elementsByTagName("category");

	for(int i=0 ; i<lst.size() ; i++) {
		QDomElement dir = lst.item(i).toElement();
		//elmt haven't got child node "element" or "category",
		//so he is emty, we can remove it
		if (dir.elementsByTagName("element").isEmpty()
				&& dir.elementsByTagName("category").isEmpty()) {
			if (removeDir(domToLocation(dir).collectionPath(false)))
				i=-1;
		}
	}
}

/**
	@brief XmlElementCollection::copyDirectory
	Copy the directory represented by source to destination.
	if destination have a directory with the same name as source,
	then this directory is removed
	@param source : directory to copy
	@param destination : destination of the copy
	@param rename : rename the copy with rename else use the name of source
	@param deep_copy :if true copy all childs of source
	@return the ElementLocation that represent the copy,
	if copy failed return a null ElementLocation
*/
ElementsLocation XmlElementCollection::copyDirectory(
		ElementsLocation &source,
		ElementsLocation &destination,
		const QString& rename,
		bool deep_copy)
{
	QString new_dir_name = rename.isEmpty() ? source.fileName() : rename;

	//Get the xml directory where the new directory must be added
	QDomElement parent_dir_dom = directory(destination.collectionPath(false));
	if (parent_dir_dom.isNull()) return ElementsLocation();

	//Remove the previous directory with the same path
	QDomElement element = child(destination.collectionPath(false)
				    + "/" + new_dir_name);
	if (!element.isNull()) {
		element.parentNode().removeChild(element);
		emit directoryRemoved(destination.collectionPath(false)
				      + "/" + new_dir_name);
	}

	ElementsLocation created_location;

	//Copy with a file system collection source
	if (source.isFileSystem())
	{
		QDir source_dir(source.fileSystemPath());
		if (!source_dir.exists()) return ElementsLocation();


		QDir dir(source.fileSystemPath());
		QDomElement elmt_dom = QETXML::fileSystemDirToXmlCollectionDir(
					m_dom_document, dir, new_dir_name);
		if (elmt_dom.isNull()) return ElementsLocation();

		parent_dir_dom.appendChild(elmt_dom);

		created_location.setPath(destination.projectCollectionPath()
					 + "/" + new_dir_name);

		if (deep_copy)
		{
			//Append all directories of source to the new created directory
			foreach(QString str, source_dir.entryList(
						QDir::Dirs | QDir::NoDotAndDotDot,
						QDir::Name))
			{
				ElementsLocation sub_source(source.fileSystemPath()
							    + "/" + str);
				copyDirectory(sub_source, created_location);
			}

			//Append all elements of source to the new created directory
			source_dir.setNameFilters(QStringList() << "*.elmt");
			foreach(QString str, source_dir.entryList(
						QDir::Files | QDir::NoDotAndDotDot,
						QDir::Name))
			{
				ElementsLocation sub_source(source.fileSystemPath()
							    + "/" + str);
				copyElement(sub_source, created_location);
			}
		}
	}

	//Copy with a xml collection source
	else
	{
		if (!source.projectCollection()) return ElementsLocation();

		QDomNode other_collection_node = source.projectCollection()->child(source.collectionPath(false)).cloneNode(deep_copy);

		//We don't make a deep copy,
		// but we must get the local names of the copied directory
		if (!deep_copy) {
			QDomNode names = source.projectCollection()->child(source.collectionPath(false)).namedItem("names");
			if (!names.isNull() && names.isElement())
				other_collection_node.appendChild(names.cloneNode(true));
		}

		if (other_collection_node.isNull()) return ElementsLocation();

		QDomElement other_collection_dom_dir = other_collection_node.toElement();
		other_collection_dom_dir.setAttribute("name", new_dir_name);
		parent_dir_dom.appendChild(other_collection_dom_dir);

		created_location.setPath(destination.projectCollectionPath() + "/" + new_dir_name);
	}

	emit directorieAdded(created_location.collectionPath(false));
	return created_location;
}

/**
	@brief XmlElementCollection::copyElement
	Copy the element represented by source to destination (must be a directory)
	If element already exist in destination he will be replaced by the new.
	@param source : element to copy
	@param destination : destination of the copy
	@param rename : rename the copy with rename else use the name of source
	@return The ElementsLocation of the copy
*/
ElementsLocation XmlElementCollection::copyElement(
		ElementsLocation &source,
		ElementsLocation &destination,
		const QString& rename)
{
	QString new_elmt_name = rename.isEmpty() ? source.fileName() : rename;

	QDomElement elmt_dom;

	//Copy with a file system collection source
	if (source.isFileSystem())
	{
		QFile file(source.fileSystemPath());
		elmt_dom = QETXML::fileSystemElementToXmlCollectionElement(
					m_dom_document, file, new_elmt_name);
		if (elmt_dom.isNull()) return ElementsLocation();
	}
	//Copy with a xml collection source
	else
	{
		QDomElement other_collection = source.xml();
		elmt_dom = m_dom_document.createElement("element");
		elmt_dom.setAttribute("name", new_elmt_name);
		elmt_dom.appendChild(other_collection.cloneNode());
	}


	//Remove the previous element with the same path
	QDomElement element = child(destination.collectionPath(false)
				    + "/" + new_elmt_name);
	bool removed = false;
	if (!element.isNull()) {
		element.parentNode().removeChild(element);
		removed = true;
	}

	//Get the xml directory where the new element must be added
	QDomElement dir_dom = directory(destination.collectionPath(false));
	if (dir_dom.isNull()) return ElementsLocation();
	dir_dom.appendChild(elmt_dom);

	ElementsLocation copy_loc(destination.projectCollectionPath()
				  + "/" + new_elmt_name);

	if (removed) {
		emit elementChanged(copy_loc.collectionPath(false));
	}
	else {
		emit elementAdded(copy_loc.collectionPath(false));
	}

	return copy_loc;
}
