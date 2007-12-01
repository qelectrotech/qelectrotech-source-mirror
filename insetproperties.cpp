/*
	Copyright 2006-2007 Xavier Guerrin
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

/// Constructeur
InsetProperties::InsetProperties() {
}

/// Destructeur
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
	@return La date a utiliser
*/
QDate InsetProperties::finalDate() const {
	if (useDate == UseDateValue) {
		return(date);
	} else {
		return(QDate::currentDate());
	}
}
