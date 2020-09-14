/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "borderproperties.h"

/**
	@brief BorderProperties::BorderProperties
	constructor
	Initializes a BorderProperties object with default properties

	Initializes a BorderProperties object
	with the following default properties:
	- 17 columns of 60.0 px wide by 20.0px high
	- 8    lines of 80.0 px high by 20.0px wide

	Initialise un objet BorderProperties avec les proprietes par
	defaut suivantes :
	- 17 colonnes affichees de 60.0 px de large pour 20.0px de haut
	- 8    lignes affichees de 80.0 px de haut pour 20.0px de large
*/
BorderProperties::BorderProperties()
{
}

/**
	@brief BorderProperties::~BorderProperties
	destructor
*/
BorderProperties::~BorderProperties() {
}

/**
	@brief BorderProperties::operator ==
	@param bp :
	- other BorderProperties container/class
	- autre conteneur BorderProperties
	@return
	- true if it and this container are identical, false otherwise
	- true si ip et ce conteneur sont identiques, false sinon
*/
bool BorderProperties::operator==(const BorderProperties &bp) {
	return(
		bp.columns_count == columns_count &&\
		bp.columns_width == columns_width &&\
		bp.columns_header_height == columns_header_height &&\
		bp.display_columns == display_columns &&\
		bp.rows_count == rows_count &&\
		bp.rows_height == rows_height &&\
		bp.rows_header_width == rows_header_width &&\
		bp.display_rows == display_rows
	);
}

/**
	@brief BorderProperties::operator !=
	@param bp :
	- other BorderProperties container/class
	- autre conteneur BorderProperties
	@return
	- false if it and this container are identical, true otherwise
	- false si bp et ce conteneur sont identiques, true sinon
*/
bool BorderProperties::operator!=(const BorderProperties &bp) {
	return(!(*this == bp));
}

/**
	@brief BorderProperties::toXml
	- Exports dimensions as XML attributes added to element e.
	- Exporte les dimensions sous formes d'attributs XML ajoutes a l'element e.
	@param e :
	- XML element to which attributes will be added
	- Element XML auquel seront ajoutes des attributs
*/
QDomElement BorderProperties::toXml(QDomDocument &dom_doc) const {

    QDomElement e = dom_doc.createElement("border");

    e.appendChild(createXmlProperty(dom_doc, "cols", columns_count));
    e.appendChild(createXmlProperty(dom_doc, "colsize", columns_width));
    e.appendChild(createXmlProperty(dom_doc, "rows", rows_count));
    e.appendChild(createXmlProperty(dom_doc, "rowsize", rows_height));
    e.appendChild(createXmlProperty(dom_doc, "displayrows", display_rows));
    e.appendChild(createXmlProperty(dom_doc, "displaycols", display_columns));

    return e;
}

/**RETURNS True
	@brief BorderProperties::fromXml
	- Import dimensions from XML attributes of element e
	- Importe les dimensions a partir des attributs XML de l'element e
	@param e :
	- XML element whose attributes will be read
	- Element XML dont les attributs seront lus
*/
bool BorderProperties::fromXml(const QDomElement &e) {

    if (propertyInteger(e, "cols", &columns_count) == PropertyFlags::NoValidConversion ||
        propertyDouble(e, "colsize", &columns_width) == PropertyFlags::NoValidConversion ||
        propertyInteger(e, "rows", &rows_count) == PropertyFlags::NoValidConversion ||
        propertyDouble(e, "rowsize", &rows_height) == PropertyFlags::NoValidConversion ||
        propertyBool(e, "displaycols", &display_columns) == PropertyFlags::NoValidConversion ||
        propertyBool(e, "displayrows", &display_rows) == PropertyFlags::NoValidConversion)
        return false;

    return true;
}

bool BorderProperties::valideXml(QDomElement& e) {

    if (propertyInteger(e, "cols") == PropertyFlags::Success ||
        propertyDouble(e, "colsize") == PropertyFlags::Success ||
        propertyInteger(e, "rows") == PropertyFlags::Success ||
        propertyDouble(e, "rowsize") == PropertyFlags::Success ||
        propertyBool(e, "displaycols") == PropertyFlags::Success ||
        propertyBool(e, "displayrows") == PropertyFlags::Success)
        return true;
    return false;
}

/**
	@brief BorderProperties::toSettings
	- Export dimensions in a QSettings object.
	- Exporte les dimensions dans une configuration.
	@param settings :
	- QSettings object to write
	- Parametres a ecrire
	@param prefix :
	- prefix to be added before the names of the parameters
	- prefixe a ajouter devant les noms des parametres
*/
void BorderProperties::toSettings(QSettings &settings, const QString &prefix) const {
	settings.setValue(prefix + "cols",        columns_count);
	settings.setValue(prefix + "colsize",     columns_width);
	settings.setValue(prefix + "displaycols", display_columns);
	settings.setValue(prefix + "rows",        rows_count);
	settings.setValue(prefix + "rowsize",     rows_height);
	settings.setValue(prefix + "displayrows", display_rows);
}

/**
	@brief BorderProperties::fromSettings
	- Import dimensions from a QSettings object.
	- Importe les dimensions depuis une configuration.
	@param settings :
	- QSettings object to read
	- Parametres a lire
	@param prefix :
	- prefix to be added before the names of the parameters
	- prefixe a ajouter devant les noms des parametres
*/
void BorderProperties::fromSettings(const QSettings &settings, const QString &prefix) {
	columns_count   = settings.value(prefix + "cols",            columns_count).toInt();
	columns_width   = qRound(settings.value(prefix + "colsize",  columns_width).toDouble());
	display_columns = settings.value(prefix + "displaycols",     display_columns).toBool();
	
	rows_count      = settings.value(prefix + "rows",            rows_count).toInt();
	rows_height     = qRound(settings.value(prefix + "rowsize",  rows_height).toDouble());
	display_rows    = settings.value(prefix + "displayrows",     display_rows).toBool();
}

/**
	@brief BorderProperties::defaultProperties
	@return the default properties stored in the setting file
*/
BorderProperties BorderProperties::defaultProperties()
{
	QSettings settings;

	BorderProperties def;
	def.fromSettings(settings, "diagrameditor/default");

	return(def);
}
