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
#include "xmlelementcollection.h"
#include "qdebug.h"
#include "nameslist.h"

/**
 * @brief XmlElementCollection::XmlElementCollection
 * Build an empty collection
 * @param parent
 */
XmlElementCollection::XmlElementCollection(QObject *parent) :
	QObject(parent)
{
	QDomElement collection = m_dom_document.createElement("collection");
	m_dom_document.appendChild(collection);
	QDomElement import = m_dom_document.createElement("category");
	import.setAttribute("name", "import");
	collection.appendChild(import);

	NamesList names;

	const QChar russian_data[24] = { 0x0418, 0x043C, 0x043F, 0x043E, 0x0440, 0x0442, 0x0438, 0x0440, 0x043E, 0x0432, 0x0430, 0x043D, 0x043D, 0x044B, 0x0435, 0x0020, 0x044D, 0x043B, 0x0435, 0x043C, 0x0435, 0x043D, 0x0442, 0x044B };
	const QChar greek_data[18] = { 0x0395, 0x03b9, 0x03c3, 0x03b7, 0x03b3, 0x03bc, 0x03ad, 0x03bd, 0x03b1, 0x0020, 0x03c3, 0x03c4, 0x03bf, 0x03b9, 0x03c7, 0x03b5, 0x03af, 0x03b1 };

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

	import.appendChild(names.toXml(m_dom_document));
}

/**
 * @brief XmlElementCollection::XmlElementCollection
 * Constructor with an collection. The tagName of @dom_element must be "collection"
 * @param dom_element -the collection in a dom_element (the dom element in cloned)
 * @param parent -parent QObject
 */
XmlElementCollection::XmlElementCollection(const QDomElement &dom_element, QObject *parent) :
	QObject(parent)
{
	QDomElement collection = m_dom_document.createElement("collection");
	m_dom_document.appendChild(collection);
	collection.appendChild(dom_element.firstChildElement("category").cloneNode(true));
}

/**
 * @brief XmlElementCollection::root
 * @return The root QDomElement of the collection
 */
QDomElement XmlElementCollection::root() const {
	return m_dom_document.documentElement();
}

/**
 * @brief XmlElementCollection::childs
 * @param parent_element
 * @return All childs element in the @parent_element tree
 */
QDomNodeList XmlElementCollection::childs(const QDomElement &parent_element)
{
	if (parent_element.ownerDocument() != m_dom_document) return QDomNodeList();
	return parent_element.childNodes();
}

/**
 * @brief XmlElementCollection::directory
 * @param parent_element
 * @return A list of directory stored in @parent_element
 */
QList<QDomElement> XmlElementCollection::directory(const QDomElement &parent_element)
{
	QList <QDomElement> directory_list;
	QDomNodeList node_list = childs(parent_element);
	if (node_list.isEmpty()) return directory_list;

	for (int i=0 ; i < node_list.count() ; i++)
	{
		QDomNode node = node_list.at(i);
		if (node.isElement() &&  node.toElement().tagName() == "category")
			directory_list << node.toElement();
	}

	return directory_list;
}

/**
 * @brief XmlElementCollection::elements
 * @param parent_element
 * @return A list of element stored in @parent_element
 */
QList<QDomElement> XmlElementCollection::elements(const QDomElement &parent_element)
{
	QList <QDomElement> element_list;
	QDomNodeList node_list = childs(parent_element);
	if (node_list.isEmpty()) return element_list;

	for (int i=0 ; i < node_list.count() ; i++)
	{
		QDomNode node = node_list.at(i);
		if (node.isElement() &&  node.toElement().tagName() == "element")
			element_list << node.toElement();
	}

	return element_list;
}
