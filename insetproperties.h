#ifndef INSET_PROPERTIES_H
#define INSET_PROPERTIES_H
#include <QDate>
#include <QString>
/**
	Cette classe est un conteneur pour les proprietes d'un cartouche de schema
	: titre, auteur, date, nom de fichier et folio
*/
class InsetProperties {
	public:
	/// Constructeur
	InsetProperties() {
	}
	/// Destructeur
	virtual ~InsetProperties() {
	}
	/**
		@param ip autre conteneur InsetProperties
		@return true si ip et ce conteneur sont identiques, false sinon
	*/
	bool operator==(const InsetProperties &ip) {
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
	bool operator!=(const InsetProperties &ip) {
		return(!(*this == ip));
	}
	// attributs
	QString title;
	QString author;
	QDate date;
	QString filename;
	QString folio;
};
#endif
