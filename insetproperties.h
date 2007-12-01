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
	InsetProperties();
	virtual ~InsetProperties();
	enum DateManagement {
		UseDateValue, ///< utilise l'attribut date
		CurrentDate   ///< utilise la date courante
	};
	
	bool operator==(const InsetProperties &);
	bool operator!=(const InsetProperties &);
	QDate finalDate() const ;
	
	// attributs
	QString title;            ///< Titre affiche par le cartouche
	QString author;           ///< Auteur affiche par le cartouche
	QDate date;               ///< Date affichee par le cartouche ; si la date est nulle, le champ reste vide
	QString filename;         ///< Nom de fichier affiche par le cartouche
	QString folio;            ///< Folio affiche par le cartouche
	DateManagement useDate;   ///< Indique s'il faut utiliser ou non l'attribut date
};
#endif
