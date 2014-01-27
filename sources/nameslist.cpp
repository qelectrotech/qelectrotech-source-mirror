/*
	Copyright 2006-2013 The QElectroTech Team
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
#include "nameslist.h"
#include "qetapp.h"

// make this class usable with QVariant
int NamesList::MetaTypeId = qRegisterMetaType<NamesList>("NamesList");

/**
	Constructeur
*/
NamesList::NamesList() {
}

/**
	Constructeur de copie
	@param other La NamesList a copier
*/
NamesList::NamesList(const NamesList &other) : hash_names(other.hash_names) {	
}

/**
	Destructeur
*/
NamesList::~NamesList() {
}

/**
	Ajoute un nom a la liste
	@param lang Sigle de deux lettres representant une langue. Si cela n'est
	pas respecte, l'insertion n'est pas effectuee.
	@param name Nom lui-meme. Ce ne doit pas etre une chaine de caractere vide.
	Si cela n'est pas respecte, l'insertion n'est pas effectuee.
*/
void NamesList::addName(const QString &lang, const QString &name) {
	if (lang.length() != 2) return;
	hash_names.insert(lang, name);
}

/**
	Enleve le nom dont une langue donnee
	@param lang la langue pour laquelle il faut supprimer le nom
*/
void NamesList::removeName(const QString &lang) {
	hash_names.remove(lang);
}

/**
	Supprime tous les noms
*/
void NamesList::clearNames() {
	hash_names.clear();
}

/**
	@return La liste de toutes les langues disponibles
*/
QList<QString> NamesList::langs() const {
	return(hash_names.keys());
}

/**
	@return true si la liste de noms est vide, false sinon
*/
bool NamesList::isEmpty() const {
	return(hash_names.isEmpty());
}

/**
	@return Le nombre de noms dans la liste
*/
int NamesList::count() const {
	return(hash_names.count());
}

/**
	@param lang une langue
	@return Le nom dans la langue donnee ou QString() si ce nom n'est pas
	defini
*/
QString &NamesList::operator[](const QString &lang) {
	return(hash_names[lang]);
}

/**
	@param lang une langue
	@return Le nom dans la langue donnee ou QString() si ce nom n'est pas
	defini
*/
const QString NamesList::operator[](const QString &lang) const {
	return(hash_names.value(lang));
}



/**
	Charge la liste de noms depuis un element XML. Cet element est sense etre
	le parent d'un element "names", qui contient lui meme les "name".
	Les noms precedemment contenus dans la liste ne sont pas effaces mais
	peuvent etre ecrases.
	@param xml_element L'element XML a analyser
	@param xml_options A set of options related to XML parsing.
	@see getXmlOptions()
*/
void NamesList::fromXml(const QDomElement &xml_element, const QHash<QString, QString> &xml_options) {
	QHash<QString, QString> xml_opt = getXmlOptions(xml_options);
	// parcourt les enfants "names" de l'element XML
	for (QDomNode node = xml_element.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
		QDomElement names = node.toElement();
		if (names.isNull() || names.tagName() != xml_opt["ParentTagName"]) continue;
		// parcourt les petits-enfants "name"
		for (QDomNode n = names.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
			QDomElement name = n.toElement();
			if (name.isNull() || name.tagName() != xml_opt["TagName"]) continue;
			addName(name.attribute(xml_opt["LanguageAttribute"]), name.text());
		}
	}
}

/**
	Exporte la liste des noms vers un element XML. Veillez a verifier que la
	liste de noms n'est pas vide avant de l'exporter.
	@param xml_document Le document XML dans lequel l'element XML sera insere
	@param xml_options A set of options related to XML parsing.
	@return L'element XML correspondant a la section "names"
	@see count()
*/
QDomElement NamesList::toXml(QDomDocument &xml_document, const QHash<QString, QString> &xml_options) const {
	QHash<QString, QString> xml_opt = getXmlOptions(xml_options);
	QDomElement names_elmt = xml_document.createElement(xml_opt["ParentTagName"]);
	QHashIterator<QString, QString> names_iterator(hash_names);
	while (names_iterator.hasNext()) {
		names_iterator.next();
		QDomElement name_elmt = xml_document.createElement(xml_opt["TagName"]);
		name_elmt.setAttribute(xml_opt["LanguageAttribute"], names_iterator.key());
		name_elmt.appendChild(xml_document.createTextNode(names_iterator.value()));
		names_elmt.appendChild(name_elmt);
	}
	return(names_elmt);
}

/**
	@param xml_options A set of options related to XML parsing. Available keys:
		* ParentTagName (falls back to "names")
		* TagName (falls back to "name")
		* LanguageAttribute (falls back to "lang")
	@return the same set, with at least all the known options
*/
QHash<QString, QString> NamesList::getXmlOptions(const QHash<QString, QString> &xml_options) const {
	QHash<QString, QString> new_xml_options = xml_options;
	if (!xml_options.contains("ParentTagName")) {
		new_xml_options.insert("ParentTagName", "names");
	}
	if (!xml_options.contains("TagName")) {
		new_xml_options.insert("TagName", "name");
	}
	if (!xml_options.contains("LanguageAttribute")) {
		new_xml_options.insert("LanguageAttribute", "lang");
	}
	return new_xml_options;
}

/**
	@param nl une autre liste de noms
	@return true si les listes de noms sont differentes, false sinon
*/
bool NamesList::operator!=(const NamesList &nl) const {
	return(hash_names != nl.hash_names);
}

/**
	@param nl une autre liste de noms
	@return true si les listes de noms sont identiques, false sinon
*/
bool NamesList::operator==(const NamesList &nl) const {
	return(hash_names == nl.hash_names);
}

/**
	Return the adequate name regarding the current system locale.
	By order of preference, this function chooses:
		- the name in the system language
		- the English name
		- the provided fallback name if non-empty
		- the first language encountered in the list
		- an empty string
	@param fallback_name name to be returned when no adequate name has been found
	@return The adequate name regarding the current system locale.
*/
QString NamesList::name(const QString &fallback_name) const {
	QString system_language = QETApp::langFromSetting();
	QString returned_name;
	if (!hash_names[system_language].isEmpty()) {
		returned_name = hash_names[system_language];
	} else if (!hash_names["en"].isEmpty()) {
		returned_name = hash_names["en"];
	} else if (!fallback_name.isEmpty()) {
		returned_name = fallback_name;
	} else if (hash_names.count()) {
		returned_name = hash_names.value(hash_names.keys().first());
	}
	return(returned_name);
}
