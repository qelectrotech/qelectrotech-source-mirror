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
#include "xmlelementdefinition.h"
#include "xmlelementscategory.h"
#include "xmlelementscollection.h"
#include "qet.h"
#include "qetproject.h"

/**
	Construit une definition d'element vide
*/
XmlElementDefinition::XmlElementDefinition(const QString &name, XmlElementsCategory *category, XmlElementsCollection *collection) :
	ElementDefinition(category, collection),
	parent_category_(category)
{
	name_ = name;
	QDomElement new_elmt = xml_element_.createElement("element");
	new_elmt.setAttribute("name", name_);
	xml_element_.appendChild(new_elmt);
	element_definition_ = xml_element_.createElement("definition");
	new_elmt.appendChild(element_definition_);
}

/**
	Construit une definition d'element a partir de sa representation XML
*/
XmlElementDefinition::XmlElementDefinition(const QDomElement &xml_element, XmlElementsCategory *category, XmlElementsCollection *collection) :
	ElementDefinition(category, collection),
	parent_category_(category)
{
	xml_element_.appendChild(xml_element_.importNode(xml_element, true));
	reload();
}

/**
	Destructeur
*/
XmlElementDefinition::~XmlElementDefinition() {
}

/**
	@return la definition XML de l'element
*/
QDomElement XmlElementDefinition::xml() {
	return(element_definition_);
}

/**
	Change la definition XML de l'element
	@param xml_element Nouvelle definition XML de l'element
	@return true si l'operation s'est bien passee, false sinon
*/
bool XmlElementDefinition::setXml(const QDomElement &xml_element) {
	// oublie toute la structure XML
	xml_element_.clear();
	
	// cree la structure XML contenant le nom de l'element
	QDomElement new_elmt = xml_element_.createElement("element");
	new_elmt.setAttribute("name", name_);
	xml_element_.appendChild(new_elmt);
	
	// importe la nouvelle definition XML de l'element
	element_definition_ = xml_element_.importNode(xml_element, true).toElement();
	new_elmt.appendChild(element_definition_);
	is_null_ = false;
	return(true);
}

bool XmlElementDefinition::write() {
	/*
		Contrairement a un schema, cet objet ne contient pas deux versions de
		l'element (element avant edition et element edite) Cette methode ne
		fait donc rien d'autre qu'emettre le signal written(), le travail ayant
		deja ete fait par la methode setXml().
	*/
	emit(written());
	return(true);
}

/**
	@return true si l'element n'est pas exploitable (definition ou nom non
	trouve)
*/
bool XmlElementDefinition::isNull() const {
	return(name_.isEmpty() || is_null_);
}

/**
	@return Le nom de cet element dans l'arborescence
*/
QString XmlElementDefinition::pathName() const {
	return(name_);
}

/**
	@return le chemin virtuel de cet element
*/
QString XmlElementDefinition::virtualPath() {
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
	Recharge le contenu de l'element
*/
void XmlElementDefinition::reload() {
	is_null_ = true;
	
	// on recupere le nom de l'element
	QDomElement doc_elmt = xml_element_.documentElement();
	name_ = doc_elmt.attribute("name");
	if (name_.isEmpty()) return;
	
	// on recupere la definition de l'element
	for (QDomNode node = doc_elmt.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
		if (!node.isElement()) continue;
		QDomElement current_element = node.toElement();
		if (current_element.tagName() == "definition" && current_element.attribute("type") == "element") {
			element_definition_ = current_element;
			break;
		}
	}
	
	// l'element est nul si aucune definition n'a ete trouvee
	is_null_ = (element_definition_.isNull());
}

/**
	@return true si l'element existe, false sinon
*/
bool XmlElementDefinition::exists() {
	// la seule raison qu'un element aurait de ne pas exister est l'absence
	// de nom
	return(!name_.isEmpty());
}

/**
	@return true si la categorie est accessible en lecture, false sinon
*/
bool XmlElementDefinition::isReadable() {
	// une categorie XML n'a aucune raison de ne pas etre accessible en lecture
	return(true);
}

/**
	@return true si la categorie est accessible en ecriture, false sinon
*/
bool XmlElementDefinition::isWritable() {
	// une categorie XML peut etre en lecture seule si le projet auquel elle
	// appartient l'est
	if (QETProject *parent_project = project()) {
		return(!parent_project -> isReadOnly());
	} else {
		return(true);
	}
}

/**
	Supprime l'element
	@return true si l'operation s'est bien passee, false sinon
*/
bool XmlElementDefinition::remove() {
	removeContent();
	emit(removed(name_));
	return(true);
}

/**
	@return toujours false, car un element XML n'a pas de chemin de type
	fichier
*/
bool XmlElementDefinition::hasFilePath() {
	// une categorie XML n'a pas de chemin de type fichier
	return(false);
}

/**
	@return une chaine vide, car un element XML n'a pas de chemin de type
	fichier
*/
QString XmlElementDefinition::filePath() {
	// une categorie XML n'a pas de chemin de type fichier
	return(QString());
}

/**
	Ne fait rien, car un element XML n'a pas de chemin de type fichier
*/
void XmlElementDefinition::setFilePath(const QString &) {
	// une categorie XML n'a pas de chemin de type fichier
}

/**
	@return a null QDateTime object since an XML element does not have a
	modification time.
*/
/**
	@return the time of the last modification (mtime) for this element file
*/
QDateTime XmlElementDefinition::modificationTime() const {
	return QDateTime();
}

QDomElement XmlElementDefinition::writeXml(QDomDocument &xml_doc) const {
	QDomElement element_elmt = xml_element_.documentElement();
	QDomNode new_node = xml_doc.importNode(element_elmt, true);
	return(new_node.toElement());
}
