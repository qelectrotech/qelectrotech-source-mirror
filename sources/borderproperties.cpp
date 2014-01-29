/*
	Copyright 2006-2014 The QElectroTech Team
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
	Constructeur. Initialise un objet BorderProperties avec les proprietes par
	defaut suivantes :
	  * 17 colonnes affichees de 60.0 px de large pour 20.0px de haut
	  * 8 lignes affichees de 80.0 px de haut pour 20.0px de large
*/
BorderProperties::BorderProperties() :
	columns_count(17),
	columns_width(60.0),
	columns_header_height(20.0),
	display_columns(true),
	rows_count(8),
	rows_height(80.0),
	rows_header_width(20.0),
	display_rows(true)
{
}

/**
	Destructeur
*/
BorderProperties::~BorderProperties() {
}

/**
	@param bp autre conteneur BorderProperties
	@return true si ip et ce conteneur sont identiques, false sinon
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
	@param bp autre conteneur BorderProperties
	@return false si bp et ce conteneur sont identiques, true sinon
*/
bool BorderProperties::operator!=(const BorderProperties &bp) {
	return(!(*this == bp));
}

/**
	Exporte les dimensions sous formes d'attributs XML ajoutes a l'element e.
	@param e Element XML auquel seront ajoutes des attributs
*/
void BorderProperties::toXml(QDomElement &e) const {
	e.setAttribute("cols",        columns_count);
	e.setAttribute("colsize",     QString("%1").arg(columns_width));
	e.setAttribute("rows",        rows_count);
	e.setAttribute("rowsize",     QString("%1").arg(rows_height));
	e.setAttribute("displaycols", display_columns ? "true" : "false");
	e.setAttribute("displayrows", display_rows    ? "true" : "false");
}

/**
	Importe les dimensions a partir des attributs XML de l'element e
	@param e Element XML dont les attributs seront lus
*/
void BorderProperties::fromXml(QDomElement &e) {
	if (e.hasAttribute("cols"))        columns_count   = e.attribute("cols").toInt();
	if (e.hasAttribute("colsize"))     columns_width   = e.attribute("colsize").toInt();
	if (e.hasAttribute("rows"))        rows_count      = e.attribute("rows").toInt();
	if (e.hasAttribute("rowsize"))     rows_height     = e.attribute("rowsize").toInt();
	if (e.hasAttribute("displaycols")) display_columns = e.attribute("displaycols") == "true";
	if (e.hasAttribute("displayrows")) display_rows    = e.attribute("displayrows") == "true";
}

/**
	Exporte les dimensions dans une configuration.
	@param settings Parametres a ecrire
	@param prefix prefixe a ajouter devant les noms des parametres
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
	Importe les dimensions depuis une configuration.
	@param settings Parametres a lire
	@param prefix prefixe a ajouter devant les noms des parametres
*/
void BorderProperties::fromSettings(QSettings &settings, const QString &prefix) {
	columns_count   = settings.value(prefix + "cols",            columns_count).toInt();
	columns_width   = qRound(settings.value(prefix + "colsize",  columns_width).toDouble());
	display_columns = settings.value(prefix + "displaycols",     display_columns).toBool();
	
	rows_count      = settings.value(prefix + "rows",            rows_count).toInt();
	rows_height     = qRound(settings.value(prefix + "rowsize",  rows_height).toDouble());
	display_rows    = settings.value(prefix + "displayrows",     display_rows).toBool();
}
