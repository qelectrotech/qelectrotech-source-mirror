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
#include "titleblockproperties.h"
#include "qet.h"
#include "qetapp.h"

#include "qetxml.h"

/**
	Constructeur. Initialise un objet TitleBlockProperties avec tous les champs
	vides (date vide + useDate a UseDateValue).
*/
TitleBlockProperties::TitleBlockProperties()
{
    setTagName("titleblock");
}

/**
	Destructeur
*/
TitleBlockProperties::~TitleBlockProperties()
{
}

/**
	@param ip autre conteneur TitleBlockProperties
	@return true si ip et ce conteneur sont identiques, false sinon
*/
bool TitleBlockProperties::operator==(const TitleBlockProperties &ip) {
	return(
		ip.title == title &&\
		ip.author == author &&\
		ip.date == date &&\
		ip.filename == filename &&\
		ip.plant == plant &&\
		ip.locmach == locmach &&\
		ip.indexrev == indexrev &&\
		ip.version == version &&\
		ip.folio == folio &&\
		ip.auto_page_num == auto_page_num &&\
		ip.template_name == template_name &&\
		ip.context == context &&\
		ip.display_at == display_at &&\
		ip.collection == collection
	);
}

/**
	@param ip autre conteneur TitleBlockProperties
	@return false si ip et ce conteneur sont identiques, true sinon
*/
bool TitleBlockProperties::operator!=(const TitleBlockProperties &ip) {
	return(!(*this == ip));
}

/**
    Exporte le cartouche sous formes d'attributs XML ajoutes a l'element e.
    @param e Element XML auquel seront ajoutes des attributs
*/
void TitleBlockProperties::toXmlPriv(QDomElement& e) const {
    e.appendChild(QETXML::createXmlProperty("author", author));
    e.appendChild(QETXML::createXmlProperty("title", title));
    e.appendChild(QETXML::createXmlProperty("filename", filename));
    e.appendChild(QETXML::createXmlProperty("plant", plant));
    e.appendChild(QETXML::createXmlProperty("locmach", locmach));
    e.appendChild(QETXML::createXmlProperty("indexrev", indexrev));
    e.appendChild(QETXML::createXmlProperty("version", version));
    e.appendChild(QETXML::createXmlProperty("folio", folio));
    e.appendChild(QETXML::createXmlProperty("date", exportDate()));
    e.appendChild(QETXML::createXmlProperty("display_at", display_at == Qt::BottomEdge? "bottom" : "right"));
    if (!template_name.isEmpty())
    {
        e.appendChild(QETXML::createXmlProperty("titleblocktemplate", template_name));
        e.appendChild(QETXML::createXmlProperty("titleblocktemplateCollection", QET::qetCollectionToString(collection)));
    }

    if (context.keys().count()) {
        QDomElement properties = e.ownerDocument().createElement("properties");
        context.toXml(properties);
        e.appendChild(properties);
    }

}

/** RETURNS True
	Importe le cartouche a partir des attributs XML de l'element e
	@param e Element XML dont les attributs seront lus
*/
bool TitleBlockProperties::fromXmlPriv(const QDomElement &e) {


	// reads the historical fields
	QETXML::propertyString(e, "author", &author);
	QETXML::propertyString(e, "title", &title);
	QETXML::propertyString(e, "filename", &filename);
	QETXML::propertyString(e, "plant", &plant);
	QETXML::propertyString(e, "locmach", &locmach);
	QETXML::propertyString(e, "indexrev", &indexrev);
	QETXML::propertyString(e, "version", &version);
	QETXML::propertyString(e, "folio", &folio);
	QETXML::propertyString(e, "auto_page_num", &auto_page_num);
	QString date;
	QETXML::propertyString(e, "date", &date);
	setDateFromString(date);

	QString display_at_temp;
	if (QETXML::propertyString(e, "displayAt", &display_at_temp) == QETXML::PropertyFlags::Success)
		display_at = (display_at_temp == "bottom" ? Qt::BottomEdge : Qt::RightEdge); // otherwise it gets default in header file
	
		// reads the template used to render the title block
	if (QETXML::propertyString(e, "titleblocktemplate", &template_name) == QETXML::PropertyFlags::Success) {
		QString tbc;
		if (QETXML::propertyString(e, "titleblocktemplateCollection", &tbc) == QETXML::PropertyFlags::Success)
			collection = QET::qetCollectionFromString(tbc);
	}
	
	// reads the additional fields used to fill the title block
	context.clear();
	foreach (QDomElement e, QET::findInDomElement(e, "properties")) {
		context.fromXml(e);
	}
	return true;
}

/**
	@brief TitleBlockProperties::toSettings
	Export the TitleBlockProperties into a QSettings
	@param settings : setting to use
	@param prefix : name to use as prefix for this property
*/
void TitleBlockProperties::toSettings(QSettings &settings, const QString &prefix) const
{
	settings.setValue(prefix + "title",	title);
	settings.setValue(prefix + "author",   author);
	settings.setValue(prefix + "filename", filename);
	settings.setValue(prefix + "plant", plant);
	settings.setValue(prefix + "locmach", locmach);
	settings.setValue(prefix + "indexrev", indexrev);
	settings.setValue(prefix + "version", version);
	settings.setValue(prefix + "folio",	folio);
	settings.setValue(prefix + "auto_page_num",	auto_page_num);
	settings.setValue(prefix + "date",	 exportDate());
	settings.setValue(prefix + "displayAt", (display_at == Qt::BottomEdge? "bottom" : "right"));
	settings.setValue(prefix + "titleblocktemplate", template_name.isEmpty()? QString() : template_name);
	settings.setValue(prefix + "titleblocktemplateCollection", QET::qetCollectionToString(collection));
	context.toSettings(settings, prefix + "properties");
}

/**
	Importe le cartouche depuis une configuration.
	@param settings Parametres a lire
	@param prefix prefixe a ajouter devant les noms des parametres
*/
void TitleBlockProperties::fromSettings(QSettings &settings, const QString &prefix) {
	title	= settings.value(prefix + "title").toString();
	author   = settings.value(prefix + "author").toString();
	filename = settings.value(prefix + "filename").toString();
	plant	= settings.value(prefix + "plant").toString();
	locmach  = settings.value(prefix + "locmach").toString();
	indexrev = settings.value(prefix + "indexrev").toString();
	version  = settings.value(prefix + "version").toString();
	folio	= settings.value(prefix + "folio", "%id/%total").toString();
	auto_page_num = settings.value(prefix + "auto_page_num").toString();
	setDateFromString(settings.value(prefix + "date").toString());
	display_at = (settings.value(prefix + "displayAt", QVariant("bottom")).toString() == "bottom" ? Qt::BottomEdge : Qt::RightEdge);
	template_name = settings.value(prefix + "titleblocktemplate").toString();
	collection = QET::qetCollectionFromString(settings.value(prefix + "titleblocktemplateCollection").toString());
	context.fromSettings(settings, prefix + "properties");
}

/**
	@brief TitleBlockProperties::defaultProperties
	Return the default properties stored in the setting file
*/
TitleBlockProperties TitleBlockProperties::defaultProperties()
{
	QSettings settings;

	TitleBlockProperties def;
	def.fromSettings(settings, "diagrameditor/default");

	return(def);
}

/**
	@return La date a utiliser
*/
QDate TitleBlockProperties::finalDate() const
{
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
QString TitleBlockProperties::exportDate() const
{
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
void TitleBlockProperties::setDateFromString(const QString &date_string) {
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
