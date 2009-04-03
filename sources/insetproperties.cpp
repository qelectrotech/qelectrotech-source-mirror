/*
	Copyright 2006-2009 Xavier Guerrin
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
#include "insetproperties.h"

/**
	Constructeur. Initialise un objet InsetProperties avec tous les champs
	vides (date vide + useDate a UseDateValue).
*/
InsetProperties::InsetProperties() :
	date(),
	useDate(UseDateValue)
{
}

/**
	Destructeur
*/
InsetProperties::~InsetProperties() {
}

/**
	@param ip autre conteneur InsetProperties
	@return true si ip et ce conteneur sont identiques, false sinon
*/
bool InsetProperties::operator==(const InsetProperties &ip) {
	return(
		ip.title == title &&\
		ip.author == author &&\
		ip.date == date &&\
		ip.filename == filename &&\
		ip.folio == folio
	);
}

/**
	@param ip autre conteneur InsetProperties
	@return false si ip et ce conteneur sont identiques, true sinon
*/
bool InsetProperties::operator!=(const InsetProperties &ip) {
	return(!(*this == ip));
}


/**
	Exporte le cartouche sous formes d'attributs XML ajoutes a l'element e.
	@param e Element XML auquel seront ajoutes des attributs
*/
void InsetProperties::toXml(QDomElement &e) const {
	e.setAttribute("author",   author);
	e.setAttribute("title",    title);
	e.setAttribute("filename", filename);
	e.setAttribute("folio",    folio);
	e.setAttribute("date",     exportDate());
}

/**
	Importe le cartouche a partir des attributs XML de l'element e
	@param e Element XML dont les attributs seront lus
*/
void InsetProperties::fromXml(QDomElement &e) {
	if (e.hasAttribute("author"))      author   = e.attribute("author");
	if (e.hasAttribute("title"))       title    = e.attribute("title");
	if (e.hasAttribute("filename"))    filename = e.attribute("filename");
	if (e.hasAttribute("folio"))       folio    = e.attribute("folio");
	if (e.hasAttribute("date"))        setDateFromString(e.attribute("date"));
}

/**
	Exporte le cartouche dans une configuration.
	@param settings Parametres a ecrire
	@param prefix prefixe a ajouter devant les noms des parametres
*/
void InsetProperties::toSettings(QSettings &settings, const QString &prefix) const {
	settings.setValue(prefix + "title",    title);
	settings.setValue(prefix + "author",   author);
	settings.setValue(prefix + "filename", filename);
	settings.setValue(prefix + "folio",    folio);
	settings.setValue(prefix + "date",     exportDate());
}

/**
	Importe le cartouche depuis une configuration.
	@param settings Parametres a lire
	@param prefix prefixe a ajouter devant les noms des parametres
*/
void InsetProperties::fromSettings(QSettings &settings, const QString &prefix) {
	title    = settings.value(prefix + "title").toString();
	author   = settings.value(prefix + "author").toString();
	filename = settings.value(prefix + "filename").toString();
	folio    = settings.value(prefix + "folio", "%id/%total").toString();
	setDateFromString(settings.value(prefix + "date").toString());
}

/**
	@return La date a utiliser
*/
QDate InsetProperties::finalDate() const {
	if (useDate == UseDateValue) {
		return(date);
	} else {
		return(QDate::currentDate());
	}
}

/**
	@return une chaine de caracteres decrivant comment gerer la date dans le
	cartouche : la chaine peut valoir :
	  * null pour ne pas afficher de date
	  * now pour afficher la date courante (a la creation du schema)
	  * une date au format yyyyMMdd pour utiliser une date fixe
*/
QString InsetProperties::exportDate() const {
	QString date_setting_value;
	if (useDate == UseDateValue) {
		if (date.isNull()) date_setting_value = "null";
		else date_setting_value = date.toString("yyyyMMdd");
	} else {
		date_setting_value = "now";
	}
	return(date_setting_value);
}

/**
	Charge les attributs date et useDate a partir d'une chaine de caracteres.
	@param date_string Chaine de caracteres a analyser
	@see exportDate
*/
void InsetProperties::setDateFromString(const QString &date_string) {
	if (date_string == "now") {
		date = QDate::currentDate();
		useDate = CurrentDate;
	} else if (date_string.isEmpty() || date_string == "null") {
		date = QDate();
		useDate = UseDateValue;
	} else {
		date = QDate::fromString(date_string, "yyyyMMdd");
		useDate = UseDateValue;
	}
}
