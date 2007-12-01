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
