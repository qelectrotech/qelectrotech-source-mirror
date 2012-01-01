/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef TITLEBLOCK_PROPERTIES_H
#define TITLEBLOCK_PROPERTIES_H
#include <QtCore>
#include <QtXml>
#include "diagramcontext.h"
/**
	Cette classe est un conteneur pour les proprietes d'un cartouche de schema
	: titre, auteur, date, nom de fichier et folio
*/
class TitleBlockProperties {
	public:
	TitleBlockProperties();
	virtual ~TitleBlockProperties();
	/// Specifie comment gerer la date
	enum DateManagement {
		UseDateValue, ///< utilise l'attribut date
		CurrentDate   ///< utilise la date courante
	};
	
	bool operator==(const TitleBlockProperties &);
	bool operator!=(const TitleBlockProperties &);
	
	void toXml(QDomElement &) const;
	void fromXml(const QDomElement &);
	void toSettings(QSettings &, const QString & = QString()) const;
	void fromSettings(QSettings &, const QString & = QString());
	
	QDate finalDate() const ;
	
	// attributs
	QString title;            ///< Titre affiche par le cartouche
	QString author;           ///< Auteur affiche par le cartouche
	QDate date;               ///< Date affichee par le cartouche ; si la date est nulle, le champ reste vide
	QString filename;         ///< Nom de fichier affiche par le cartouche
	QString folio;            ///< Folio affiche par le cartouche
	DateManagement useDate;   ///< Indique s'il faut utiliser ou non l'attribut date
	QString template_name;    ///< Name of the template used to render the title block - an empty string means "the default template provided by the application"
	DiagramContext context;   ///< Container for the additional, user-defined fields
	
	private:
	QString exportDate() const;
	void setDateFromString(const QString &);
};
#endif
