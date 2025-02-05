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
#include "nameslist.h"

#include "../qetapp.h"

// make this class usable with QVariant
int NamesList::MetaTypeId = qRegisterMetaType<NamesList>("NamesList");

/**
	Constructeur
*/
NamesList::NamesList()
{
}

/**
	Destructeur
*/
NamesList::~NamesList()
{
}

/**
	Ajoute un nom a la liste
	@param lang Sigle de deux ou cinq lettres representant une langue. Si cela
	n'est pas respecte, l'insertion n'est pas effectuee.
	@param name Nom lui-meme. Ce ne doit pas etre une chaine de caractere vide.
	Si cela n'est pas respecte, l'insertion n'est pas effectuee.
*/
void NamesList::addName(const QString &lang, const QString &name) {
	if ((lang.length() != 2) && (lang.length() != 5)) return;
	if ((lang.length() == 5) && (lang[2] != '_')) return;
	map_names.insert(lang, name);
}

/**
	Enleve le nom dont une langue donnee
	@param lang la langue pour laquelle il faut supprimer le nom
*/
void NamesList::removeName(const QString &lang) {
	map_names.remove(lang);
}

/**
	Supprime tous les noms
*/
void NamesList::clearNames()
{
	map_names.clear();
}

/**
	@return La liste de toutes les langues disponibles
*/
QList<QString> NamesList::langs() const
{
	return(map_names.keys());
}

/**
	@return true si la liste de noms est vide, false sinon
*/
bool NamesList::isEmpty() const
{
	return(map_names.isEmpty());
}

/**
	@return Le nombre de noms dans la liste
*/
int NamesList::count() const
{
	return(map_names.count());
}

/**
	@param lang une langue
	@return Le nom dans la langue donnee ou QString() si ce nom n'est pas
	defini
*/
QString &NamesList::operator[](const QString &lang) {
	return(map_names[lang]);
}

/**
	@param lang une langue
	@return Le nom dans la langue donnee ou QString() si ce nom n'est pas
	defini
*/
const QString NamesList::operator[](const QString &lang) const
{
	return(map_names.value(lang));
}



/**
	Loads the list of names from an XML element. This element is assumed to be
	the parent of a ‘names’ element, which itself contains the ‘names’. The
	names previously contained in the list are not deleted, but can be overwritten.
	French:
	Charge la liste de noms depuis un element XML. Cet element est sense etre
	le parent d'un element "names", qui contient lui meme les "name".
	Les noms precedemment contenus dans la liste ne sont pas effaces mais
	peuvent etre ecrases.
	@param xml_element L'element XML a analyser / The XML element to be parsed
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
	@brief NamesList::fromXml
	Load the list of lang <-> name from an xml description.
	xml_element must be the parent of a child element tagged "names"
	If a couple lang <-> name already exist, they will overwritten, else
	they will be appended.
	@param xml_element : xml element to analyze
	@param xml_options : A set of options related to XML parsing.
	@see getXmlOptions()
*/
void NamesList::fromXml(const pugi::xml_node &xml_element, const QHash<QString, QString> &xml_options)
{
	QHash<QString, QString> xml_opt = getXmlOptions(xml_options);

		//Walk the childs "names" of the xml element
	for (auto names = xml_element.first_child() ; names ; names = names.next_sibling())
	{
		if (names.type() != pugi::node_element ||
			QString(names.name()) != xml_opt["ParentTagName"]) {
			continue;
		}
		for (auto name = names.first_child(); name; name = name.next_sibling()) {
			if (name.type() != pugi::node_element ||
				QString(name.name()) != xml_opt["TagName"]) {
				continue;
			}
			QString lang_str(name.attribute(xml_opt["LanguageAttribute"].toStdString().c_str()).as_string());
			QString name_str(name.text().get());
			addName(lang_str, name_str);
		}
	}
}

/**
	Exports the list of names to an XML element.
	Make sure that the list of names is not empty before exporting.
	If list is empty, set name to "en" / "NoName"
	French:
	Exporte la liste des noms vers un element XML. Veillez a verifier que la
	liste de noms n'est pas vide avant de l'exporter.
	Si la liste est vide, le nom sera "en" / "NoName".
	@param xml_document Le document XML dans lequel l'element XML sera insere
	@param xml_options A set of options related to XML parsing.
	@return L'element XML correspondant a la section "names"
	@see count()
*/
QDomElement NamesList::toXml(QDomDocument &xml_document, const QHash<QString, QString> &xml_options) const
{
	QHash<QString, QString> xml_opt = getXmlOptions(xml_options);
	QDomElement names_elmt = xml_document.createElement(xml_opt["ParentTagName"]);
	if (map_names.isEmpty()) {
		qInfo() << " NamesList of element is empty - add default: [" << "en" << "] = " << "NoName" << "";
		QDomElement name_elmt = xml_document.createElement(xml_opt["TagName"]);
		name_elmt.setAttribute(xml_opt["LanguageAttribute"], "en");
		name_elmt.appendChild(xml_document.createTextNode("NoName"));
		names_elmt.appendChild(name_elmt);
	} else {
		QMapIterator<QString, QString> names_iterator(map_names);
		while (names_iterator.hasNext()) {
			names_iterator.next();
			QDomElement name_elmt = xml_document.createElement(xml_opt["TagName"]);
			name_elmt.setAttribute(xml_opt["LanguageAttribute"], names_iterator.key());
			name_elmt.appendChild(xml_document.createTextNode(names_iterator.value().trimmed()));
			names_elmt.appendChild(name_elmt);
		}
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
QHash<QString, QString> NamesList::getXmlOptions(const QHash<QString, QString> &xml_options) const
{
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
bool NamesList::operator!=(const NamesList &nl) const
{
	return(map_names != nl.map_names);
}

/**
	@param nl une autre liste de noms
	@return true si les listes de noms sont identiques, false sinon
*/
bool NamesList::operator==(const NamesList &nl) const
{
	return(map_names == nl.map_names);
}

/**
 * @brief NamesList::name
 * Return the adequate name regarding the current system locale.
 * By order of preference, this function chooses:
 *  - the name in the system language
 *  - the English name
 *  - the provided fallback name if non-empty
 *  - the first language encountered in the list
 *  - an empty string
 * @param fallback_name
 * name to be returned when no adequate name has been found
 * @return The adequate name regarding the current system locale.
 */
QString NamesList::name(const QString &fallback_name) const
{
	QString system_language = QETApp::langFromSetting();
	if (! map_names[system_language].isEmpty())
		return (map_names[system_language]);
	if (! map_names["en"].isEmpty()) return (map_names["en"]);
	if (! fallback_name.isEmpty()) return (fallback_name);
	if (map_names.count()) return (map_names.begin().value());
	return (QString(""));
}
